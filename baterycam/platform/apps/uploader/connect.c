#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h> // on linux
// #include <sys/socket.h> // on linux
// #include <sys/stat.h> // on linux

#ifndef HTTP_TEST_ON_LINUX
    #if 0
    #include <FreeRTOS.h>
    #include <task.h>
    #include <queue.h>
    #include <semphr.h>
    #include "ip_over_spi.h"
    #endif
#endif

#include <time.h>
#include "json.h"
#include "json_handle.h"
#include "connect.h"
#include "dbg_uploader.h"

#include "http.h"
#include "buffer.h"
#include "ak_apps_config.h"
#include "ak_video.h"
#include "doorbell_config.h"
#include "upload_main.h"

extern DOORBELL_SNAPSHOT_DATA_t g_sDataSnapshot4Uploader;
extern char g_macaddress[];
/*
TODO: improve:
Need to implement boundary!!!
*/

/******************************************************************************/
/*                              DEFINITIONS                                   */
/******************************************************************************/
char g_acFileNameSnap[256];
char g_acFileNameClip[256];

/******************************************************************************/
/*                              FUNCTION                                      */
/******************************************************************************/
/*
* We don't use make_connection, because init_connection will open and connect 
* to server.
*/
#ifdef UPLOADER_RUN_ON_AK_PLAT

int init_connection(char *hostname, char *port, int protocol)
{
    int sockfd, status;
// #if(UPLOADER_USE_SSL == 1)
//     sockfd = socket_api(hostname, port, 1);
// #else
//     sockfd = socket_api(hostname, port, 0);
// #endif
    sockfd = socket_api(hostname, port, protocol);
    jump_unless(sockfd > 0);
    return sockfd;
error:
    return -1;
}
#else // for LINUX Platform
int
init_connection(char *hostname, char *port, struct addrinfo **res)
{    
    struct addrinfo hints;    
    memset(&hints, 0, sizeof(hints));    
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_STREAM;    
    return getaddrinfo(hostname, port, &hints, res);
}

int make_connection(struct addrinfo *res)
{
    int sockfd, status;

    sockfd = socket_api(res->ai_family, res->ai_socktype, res->ai_protocol);
    jump_unless(sockfd > 0);

    status = connect_api(sockfd, res->ai_addr, res->ai_addrlen);
    jump_unless(status == 0);

    return sockfd;

error:
    return -1;
}
#endif

char *
build_request(char *hostname, char *request_path)
{
    char *request = NULL;
    Buffer *request_buffer = buffer_alloc(BUF_SIZE);

    buffer_appendf(request_buffer, "GET %s HTTP/1.0\r\n", request_path);
    buffer_appendf(request_buffer, "Host: %s\r\n", hostname);
    buffer_appendf(request_buffer, "Connection: close\r\n\r\n");

    request = buffer_to_s(request_buffer);
    buffer_free(request_buffer);

    return request;
}

/*
* We use sockfd as id of tcp_open
*/
int
make_request(int sockfd, char *hostname, char *request_path)
{
    char *request           = build_request(hostname, request_path);
    size_t bytes_sent       = 0;
    size_t total_bytes_sent = 0;
    size_t bytes_to_send    = strlen(request);
    int i = 0;
    while (1) {
        bytes_sent = send_api(sockfd, request, strlen(request), 0);
        total_bytes_sent += bytes_sent;
        debug("Bytes sent: %ld", bytes_sent);
        if (total_bytes_sent >= bytes_to_send) {
            break;
        }
    }
    free(request);
    return total_bytes_sent;
}

int
fetch_response(int sockfd, Buffer **response, int recv_size)
{
    int bytes_received;
    int status = 0;
    int l_timeout = 0;
    int l_total_byte = 0;
    int l_sleep_count = 0;
    char data[2*HTTP_CHUNK_SIZE]={0};
    printf("Rx...");
    
    while (1) {
        bytes_received = recv_api(sockfd, data, HTTP_CHUNK_SIZE, 0 );
        printf("\r\nReceive %d bytes\r\n", bytes_received);
        if (bytes_received <= 0) {
	        if (l_total_byte>0)
	        	return 0;
	        else{
		        if (l_sleep_count>=20)
		        	return -1;
	        }
        }
        if (bytes_received > 0) {
	        l_total_byte += bytes_received;
	        printf("Append\r\n");
            status = buffer_append(*response, data, bytes_received);
            if (status != 0) {
	            printf("Append fail\r\n");
                fprintf(stderr, "Failed to append to buffer.\n");
                return -1;
            }
            printf("Append done: --%s--\r\n",data);
        }

        // port_sleep_ms(100);
        ak_sleep_ms(100);
		l_sleep_count ++;
    }
    debug("Finished receiving data.");
    return status;
}


char *
build_header_post_request(char *hostname, char *request_path,
                                char *sFileName, unsigned int u32LenFile)
{
    char *request = NULL;
    Buffer *request_buffer = buffer_alloc(BUF_SIZE);

    buffer_appendf(request_buffer, "POST %s HTTP/1.0\r\n", request_path);
    buffer_appendf(request_buffer, "Host: %s\r\n", hostname);
    buffer_appendf(request_buffer, "Connection: close\r\n");
    buffer_appendf(request_buffer, "Content-Length: %u\r\n", u32LenFile);

    buffer_appendf(request_buffer, "Cache-Control: no-cache\r\n");
    buffer_appendf(request_buffer, "User-Agent: GMCC\r\n");
    
    buffer_appendf(request_buffer, "Content-Type: multipart/form-data, boundary=----WebKitFormBoundaryXCCDljpXzSKJi0QL\r\n");
    buffer_appendf(request_buffer, "Accept: */*\r\n");
    buffer_appendf(request_buffer, "Accept-Encoding: gzip, deflate\r\n");
    buffer_appendf(request_buffer, "Accept-Language: en-US,en;q=0.8\r\n\r\n");

    
    buffer_appendf(request_buffer, "------WebKitFormBoundaryXCCDljpXzSKJi0QL\r\n");
#ifdef SERVER_LOCAL
    buffer_appendf(request_buffer, "Content-Disposition: form-data; name=\"fileToUpload\"; filename=\"%s\"\r\n", sFileName);
#else
    buffer_appendf(request_buffer, "Content-Disposition: form-data; name=\"snap\"; filename=\"%s\"\r\n", sFileName);
#endif
    //buffer_appendf(request_buffer, "Content-Disposition: form-data; name=\"fileToUpload\"; filename=\"123456.jpg\"\r\n");
    buffer_appendf(request_buffer, "Content-Type: image/jpeg\r\n\r\n");

    request = buffer_to_s(request_buffer);
    buffer_free(request_buffer);

    return request;
}

char *
build_tail_post_request(void)
{
    char *request = NULL;
    int ret = 0;
    Buffer *request_buffer = buffer_alloc(BUF_SIZE);
    buffer_appendf(request_buffer, "------WebKitFormBoundaryXCCDljpXzSKJi0QL\r\n");
    buffer_appendf(request_buffer, "Content-Disposition: form-data; name=\"registration_id\"\r\n\r\n");
    buffer_appendf(request_buffer, "00000274DAEA813043PAVAIX\r\n");
    ret = buffer_appendf(request_buffer, "------WebKitFormBoundaryXCCDljpXzSKJi0QL--\r\n\r\n");
    request = buffer_to_s(request_buffer);
    buffer_free(request_buffer);
    return request;
}

/*
* GET method
*/
int
get_method_request(int id, char *hostname, char *request_path)
{
    char *request           = build_request(hostname, request_path);
    size_t bytes_sent       = 0;
    size_t total_bytes_sent = 0;
    size_t bytes_to_send    = strlen(request);
    
    while (1) {
#ifdef HTTP_TEST_ON_LINUX
        bytes_sent = send_api(id, request, strlen(request), 0);
#else
        bytes_sent = tcp_write(id, request, bytes_to_send);
#endif
        total_bytes_sent += bytes_sent;
        if (total_bytes_sent >= bytes_to_send) {
            break;
        }
    }
    free(request);
    return total_bytes_sent;
}
/*---------------------------------------------------------------------------*/
/* function for event add                                                    */
/*---------------------------------------------------------------------------*/
extern char g_UDID[];
extern char g_devicetoken[];
extern char g_key_hex[];
extern char g_rnd_hex[];
extern char g_sip[];
extern char g_sp[];

#if(UPLOADER_USE_SSL == 0)
#define EVENT_STR "{\"device_id\":\"%s\",\"event_type\":6,\"st_data\":{\"key\":\"%s\",\"rn\":\"%s\",\"sip\":\"%s\",\"sp\":%s},\"device_token\":\"%s\",\"is_ssl\":false}"
#else
#define EVENT_STR "{\"device_id\":\"%s\",\"event_type\":6,\"st_data\":{\"key\":\"%s\",\"rn\":\"%s\",\"sip\":\"%s\",\"sp\":%s},\"device_token\":\"%s\",\"is_ssl\":true}"
#endif

char *build_post_request_event_add(char *hostname)
{
    int iRet = 0;
    char *request = NULL;
    char l_json_string[300]="";
    Buffer *request_buffer = buffer_alloc(BUF_SIZE);

    char post_event_url[200];
    
    memset(post_event_url,0x00, sizeof(post_event_url));
    //snprintf(post_event_url, sizeof(post_event_url),"/v1/devices/events?device_id=%s&event_type=1&device_token=%s&is_ssl=false",g_UDID,g_devicetoken);
    snprintf(post_event_url, sizeof(post_event_url),"/v1/devices/events?");
    snprintf(l_json_string,sizeof(l_json_string),EVENT_STR,g_UDID,g_key_hex,g_rnd_hex,g_sip,g_sp,g_devicetoken);
    printf("%s post_event_url: %s\r\n", __func__, post_event_url);
    printf("%s BUF_SIZE: %d\r\n", __func__, BUF_SIZE);
    printf("%s request_buffer: %p\r\n", __func__, request_buffer);
    printf("json_string: %s\r\n",l_json_string);

    iRet = buffer_appendf(request_buffer, "POST %s HTTP/1.1\r\n", post_event_url);
    if(iRet < 0){free(request_buffer); return NULL;}
    iRet = buffer_appendf(request_buffer, "Host: %s\r\n", hostname);
    if(iRet < 0){free(request_buffer); return NULL;}
    iRet = buffer_appendf(request_buffer, "Connection: close\r\n");
    if(iRet < 0){free(request_buffer); return NULL;}
    iRet = buffer_appendf(request_buffer, "Content-Length: %d\r\n", strlen(l_json_string));
    if(iRet < 0){free(request_buffer); return NULL;}
    //buffer_appendf(request_buffer, "Cache-Control: no-cache\r\n");
    //buffer_appendf(request_buffer, "User-Agent: GMCC\r\n");
    iRet = buffer_appendf(request_buffer, "Content-Type: application/json\r\n");
    if(iRet < 0){free(request_buffer); return NULL;}
    iRet = buffer_appendf(request_buffer, "Accept: */*\r\n");
    if(iRet < 0){free(request_buffer); return NULL;}
    iRet = buffer_appendf(request_buffer, "Accept-Encoding: gzip, deflate\r\n");
    if(iRet < 0){free(request_buffer); return NULL;}
    iRet = buffer_appendf(request_buffer, "Accept-Language: en-US,en;q=0.8\r\n\r\n");
    if(iRet < 0){free(request_buffer); return NULL;}
    iRet = buffer_appendf(request_buffer, "%s", l_json_string);
    if(iRet < 0){free(request_buffer); return NULL;}

    request = buffer_to_s(request_buffer);
    buffer_free(request_buffer);

    printf("request %s\r\n", request);
    printf("--------------------exit.\r\n");
    return request;
}

int make_request_event_add(int sockfd, char *hostname)
{
    char *request = NULL;
    size_t bytes_sent       = 0;
    size_t total_bytes_sent = 0;
    size_t bytes_to_send    = 0;
    int current_chunk_length;

    request = build_post_request_event_add(hostname);
    bytes_to_send = strlen(request);

    //printf("***Request:\r\n");
    //printf("%s\r\n", request);
    printf("***Tx: ");
    while (1) {
		if ((bytes_to_send-total_bytes_sent)>HTTP_CHUNK_SIZE_NKB)
			current_chunk_length = HTTP_CHUNK_SIZE_NKB;
		else
			current_chunk_length = bytes_to_send-total_bytes_sent;
		bytes_sent = send_api(sockfd, request+total_bytes_sent, current_chunk_length, 0);
        total_bytes_sent += bytes_sent;
        printf(" %ld ", bytes_sent);
        if (total_bytes_sent >= bytes_to_send) {
            // send 0 to stop
            bytes_sent = send_api(sockfd, request+total_bytes_sent, 0, 0);
            break;
        }
    }
    free(request);
    return total_bytes_sent;
}

/*
* Function Name  : con_find_json_response
* Description    : find a string json in response
* Input          : char* point to response
* Return         :  NULL if parameter is invalid
*                   char* json if success
*/
char *con_find_json_response(char *response)
{
    int ret = 0;
    char *json_response = NULL;
    JsonNode *node = NULL;
    JsonNode *find_node = NULL;
    if(!response){
        debug("Argument null!");
        return NULL;
    }
    
    json_response = strstr(response, "{");
    if(!json_response){
        debug("json_response null!");
        return NULL;
    }
    else{
        return json_response;
    }
}

/*---------------------------------------------------------------------------*/
/* fucntion for upload snapshot */
/*---------------------------------------------------------------------------*/
/* Build request for upload snapshot and clip */
/*
POST / HTTP/1.1
Host: cinatic-resources.s3.amazonaws.com
Connection: keep-alive
Content-Length: 185274
Cache-Control: max-age=0
Origin: null
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryEbgGqPhzuTzSsDZx
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*_/_*;q=0.8
Accept-Encoding: gzip, deflate
Accept-Language: en-US,en;q=0.8

------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="key"

00000274DAEA813043PAVAIX/${filename}
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="acl"

private
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="Content-Type"

image/jpeg
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="x-amz-meta-uuid"

0921cfc0-e121-11e6-b342-79487726f07f
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="x-amz-server-side-encryption"

AES256
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="X-Amz-Credential"

AKIAIH5TTUQA5CLP5EKQ/20170123/us-east-1/s3/aws4_request
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="X-Amz-Algorithm"

AWS4-HMAC-SHA256
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="X-Amz-Date"

20170123T000000Z
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="x-amz-meta-tag"


------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="Policy"

eyAiZXhwaXJhdGlvbiI6ICIyMDE3LTAxLTI0VDA0OjA0OjI4LjAwMFoiLCJjb25kaXRpb25zIjogW3siYnVja2V0IjogImNpbmF0aWMtcmVzb3VyY2VzIn0sWyJzdGFydHMtd2l0aCIsICIka2V5IiwgIjAwMDAwMjc0REFFQTgxMzA0M1BBVkFJWC8iXSx7ImFjbCI6ICJwcml2YXRlIn0sWyJzdGFydHMtd2l0aCIsICIkQ29udGVudC1UeXBlIiwgImltYWdlL2pwZWciXSx7IngtYW16LW1ldGEtdXVpZCI6ICIwOTIxY2ZjMC1lMTIxLTExZTYtYjM0Mi03OTQ4NzcyNmYwN2YifSx7IngtYW16LXNlcnZlci1zaWRlLWVuY3J5cHRpb24iOiAiQUVTMjU2In0sWyJzdGFydHMtd2l0aCIsICIkeC1hbXotbWV0YS10YWciLCAiIl0seyJ4LWFtei1jcmVkZW50aWFsIjogIkFLSUFJSDVUVFVRQTVDTFA1RUtRLzIwMTcwMTIzL3VzLWVhc3QtMS9zMy9hd3M0X3JlcXVlc3QifSx7IngtYW16LWFsZ29yaXRobSI6ICJBV1M0LUhNQUMtU0hBMjU2In0seyJ4LWFtei1kYXRlIjogIjIwMTcwMTIzVDAwMDAwMFoiIH1dfQ==
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="X-Amz-Signature"

bd356fbae5f97348f191ca11b2a674b1351ecb4b1f8beecb4dc7494aad1e4f07
------WebKitFormBoundaryEbgGqPhzuTzSsDZx
Content-Disposition: form-data; name="file"; filename="MITSUBISHI.jpg"
Content-Type: image/jpeg
*/
/*
Generate boundary
*/

static char *g_astrInfoUploadSnap[CON_SIZE_KEY_SNAP] =
{
#ifdef SERVER_CEPHS3
    KEY_AWSACCESSKEYID,
#endif
    KEY_KEY_PATH,   //1
    KEY_ACL,        //2
    KEY_X_AMZ_META_UUID,    //3
    KEY_X_AMZ_SERVER_SIDE_ENCRYPT,  //4
    KEY_X_AMZ_CREDENTIAL, //5
    KEY_X_AMZ_ALGORITHM,  //6
    KEY_X_AMZ_DATE, //7
    // dont use meta tag
    KEY_SNAPSHOT_POLICY, // 8
    KEY_SNAPSHOT_X_AMZ_SIGNATURE // 9
};

static char *g_astrInfoUploadClip[CON_SIZE_KEY_CLIP] =
{
#ifdef SERVER_CEPHS3
    KEY_AWSACCESSKEYID,
#endif
    KEY_KEY_PATH,   //1
    KEY_ACL,        //2
    KEY_X_AMZ_META_UUID,    //3
    KEY_X_AMZ_SERVER_SIDE_ENCRYPT,  //4
    KEY_X_AMZ_CREDENTIAL, //5
    KEY_X_AMZ_ALGORITHM,  //6
    KEY_X_AMZ_DATE, //7
    // dont use meta tag
    KEY_CLIP_POLICY, // 8
    KEY_CLIP_X_AMZ_SIGNATURE // 9
};


/*
* Upload success info
*/
static char *g_astrHttpSttSuccess[HTTP_STT_NUMBER] =
{
    HTTP_STT_CODE_SUCCESS_200_OK,              
    HTTP_STT_CODE_SUCCESS_201_CREATED,         
    HTTP_STT_CODE_SUCCESS_202_ACCEPTED,        
    HTTP_STT_CODE_SUCCESS_203_NON_AUTH,        
    HTTP_STT_CODE_SUCCESS_204_NO_CONTENT,      
    HTTP_STT_CODE_SUCCESS_205_RESET_CONTENT,   
    HTTP_STT_CODE_SUCCESS_206_PARTIAL_CONTENT, 
    HTTP_STT_CODE_SUCCESS_207_MULTI_STATUS,    
    HTTP_STT_CODE_SUCCESS_208_ALREADY_REPORTED,
    HTTP_STT_CODE_SUCCESS_226_IM_USED         
};

//static CON_UPLOAD_SUCCESS_INFO_t g_asUploadSuccessSnapshot;
//static CON_UPLOAD_SUCCESS_INFO_t g_asUploadSuccessClip;
static CON_UPLOAD_SUCCESS_INFO_t g_asUploadSuccess;

static char *gen_boundary(void)
{
    char *boundary = (char *)malloc(BOUNDARY_SIZE_STR);
    if(!boundary){
        return NULL;
    }
    memset(boundary, 0x00, BOUNDARY_SIZE_STR);
    memcpy(boundary, BOUNDARY_HARD_CODE, strlen(BOUNDARY_HARD_CODE));
    return boundary;
}

static void free_boundary(char *boundary)
{
    if(boundary){
        free(boundary);
    }
}


/*
* Get name file snap and clip 
*/
// static int get_file_name_motion(char *file_name)
// {
//     int ret = 0;
// #ifdef UPLOADER_RUN_ON_AK_PLAT
//     char fn[256] = {0x00};
//     char mac_addr[16] = {0x00};
//     memset(fn, 0x00, 256);
//     ret = snapshot_get_file_name(fn);
//     if(ret == 0)
//     {
//         memcpy(file_name, fn, strlen(fn));
//     }

// #else
//     time_t sLocalTime;
//     struct tm *timeinfo;
//     char mac_addr[16] = {0x00};

//     if(!file_name){
//         return -1;
//     }
//     memset(mac_addr, 0x00, sizeof(mac_addr));
//     if(get_mac_address(mac_addr) < 0){
//         return -1;
//     }
//     time(&sLocalTime);
//     timeinfo = (struct tm *)localtime(&sLocalTime);
//     sprintf(file_name, "%s_%04d%02d%02d%02d%02d%02d",
//                         mac_addr,
//                         timeinfo->tm_year + 1900,
//                         timeinfo->tm_mon + 1,
//                         timeinfo->tm_mday,
//                         timeinfo->tm_hour,
//                         timeinfo->tm_min,
//                         timeinfo->tm_sec);
// #endif
//     return ret;
// }

static int get_file_size_snap(char *path)
{
    struct stat st;
    stat(path, &st);
    return st.st_size;
}
/*
* Build header for upload snapshot
*/
static int set_request_property_header(Buffer **ppRequest, CON_HEADER_SNAP_t *psHeader)
{
    int ret = 0;

    debug("Entered!");

    if(!ppRequest || !psHeader){
        debug("request null!");
        return -1;
    }
    /* Build post */
    printf("Build post %s %s\r\n", psHeader->req_path, psHeader->host_name);
    ret = buffer_appendf(*ppRequest, POSTFIELD_POST_X, psHeader->req_path);
    CHECK_ERROR(ret);
    /* Build host */
    ret = buffer_appendf(*ppRequest, POSTFIELD_HOST_X, psHeader->host_name);
    CHECK_ERROR(ret);
    /* Build connection */
    ret = buffer_appendf(*ppRequest, POSTFIELD_CONNECTION_X, psHeader->connect_type);
    CHECK_ERROR(ret);
    /* Build content length */
    ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_LENGTH_X, psHeader->total_length);
    CHECK_ERROR(ret);

    /* Build cache control */
    ret = buffer_appendf(*ppRequest, POSTFILED_STR_CACHE_CONTROL);
    CHECK_ERROR(ret);
    /* Build origin */
    ret = buffer_appendf(*ppRequest, POSTFILED_STR_ORIGIN);
    CHECK_ERROR(ret);
     /* Build upgrade */
    ret = buffer_appendf(*ppRequest, POSTFILED_STR_UPGRADE);
    CHECK_ERROR(ret);

    /* Build user-agent */
    ret = buffer_appendf(*ppRequest, POSTFIELD_USER_AGENT_X, psHeader->usr_agent);
    CHECK_ERROR(ret);
    /* Build multpart with boundary */
    ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_TYPE_FORM_DATA_X, psHeader->boundary);
    CHECK_ERROR(ret);
    /* Build accept */
    ret = buffer_appendf(*ppRequest, POSTFIELD_ACCEPT);
    CHECK_ERROR(ret);
    /* Build accept coding */
    ret = buffer_appendf(*ppRequest, POSTFIELD_ACCEPT_CODING);
    CHECK_ERROR(ret);
    /* Build accept language */
    ret = buffer_appendf(*ppRequest, POSTFIELD_ACCEPT_LANGUAGE);
    CHECK_ERROR(ret);
    return ret;
} 
/*
* Build body with info sv cinatic
*/
static int set_request_property_body_snapshot(Buffer **ppRequest, char *response, char *boundary, char* file_ext)
{
    int ret = 0;
    int ret_buf_append = 0;
    int i = 0;
    JsonNode *json_info = NULL;
    char info[1024] = {0x00};
    if(!ppRequest || !response || !boundary){
        debug("Argument null!");
        return -1;
    }
    json_info = jh_get_json_node_coredata(response);
    if(!json_info){
        debug("Json Info null!");
        return -2;
    }
    
    for(i = 0; i < CON_SIZE_KEY_SNAP; i++){
        memset(info, 0x00, sizeof(info));
        debug("parse item %d", i);
        if (file_ext[0]=='j')
            ret = jh_find_data_by_key(json_info, g_astrInfoUploadSnap[i], info);
        else
            ret = jh_find_data_by_key(json_info, g_astrInfoUploadClip[i], info);
        debug("parse item %d done", i);
        if(ret < 0){
            continue;
        }
        else{
            /* Append boundary */
            ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_BOUNDARY, boundary);
            CHECK_ERROR(ret_buf_append);
            /* KEY */
            /*
            #if 0
            if(i == 7){ // policy
                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, POSTFIELD_POLICY);
            }
            else{
                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, g_astrInfoUploadSnap[i]);
            }
            #endif*/
            // for signature snapshot
            if (file_ext[0]=='j'){
	            if(strcmp(g_astrInfoUploadSnap[i], KEY_SNAPSHOT_X_AMZ_SIGNATURE) == 0){
	                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, POSTFIELD_SIGNATURE);
	            }
	            else if(strcmp(g_astrInfoUploadSnap[i], KEY_SNAPSHOT_POLICY) == 0){
	                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, POSTFIELD_POLICY);
	            }
	            else{
	                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, g_astrInfoUploadSnap[i]);
	            }
            } else {
	            if(strcmp(g_astrInfoUploadClip[i], KEY_CLIP_X_AMZ_SIGNATURE) == 0){
	                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, POSTFIELD_SIGNATURE);
	            }
	            else if(strcmp(g_astrInfoUploadClip[i], KEY_CLIP_POLICY) == 0){
	                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, POSTFIELD_POLICY);
	            }
	            else{
	                ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, g_astrInfoUploadClip[i]);
	            }
            }            
            
            
            
            
            CHECK_ERROR(ret_buf_append);
            /* VALUE */
            ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_VALUE, info);
            CHECK_ERROR(ret_buf_append);
        }
    }
    
    // append meta tag
    /* Append boundary */
    #ifndef SERVER_CEPHS3
    debug("append %s\r\n", POSTFIELD_BOUNDARY);
    ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_BOUNDARY, boundary);
    CHECK_ERROR(ret_buf_append);
    debug("append %s\r\n", POSTFIELD_CONTENT_DISPOSITION_X);
    ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, POSTFIELD_META_TAG);
    CHECK_ERROR(ret_buf_append);
    /* VALUE */
    debug("append %s\r\n", POSTFIELD_CONTENT_DISPOSITION_VALUE);
    ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_VALUE, "");
    CHECK_ERROR(ret_buf_append);
    #endif
    
    // append content type
    /* Append boundary */
    debug("append %s\r\n", POSTFIELD_BOUNDARY);
    ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_BOUNDARY, boundary);
    CHECK_ERROR(ret_buf_append);
    debug("append %s\r\n", POSTFIELD_CONTENT_DISPOSITION_X);
    ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, POSTFIELD_NAME_CONTENT_TYPE_SNAP);
    CHECK_ERROR(ret_buf_append);
    debug("append %s\r\n", POSTFIELD_CONTENT_DISPOSITION_VALUE);
    /* VALUE */
    if (file_ext[0]=='j')
        ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_VALUE, POSTFIELD_VALUE_CONTENT_TYPE_SNAP);
    else
        ret_buf_append = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_VALUE, POSTFIELD_VALUE_CONTENT_TYPE_CLIP);
    debug("return %d\r\n", ret_buf_append);
    CHECK_ERROR(ret_buf_append);
    debug("return %d\r\n", ret_buf_append);
    return 0;
}

/*
* Build file name and type of file upload
*/
static int set_request_property_file_snapshot(Buffer **ppRequest, char *boundary, char* file_ext)
{
    int ret = 0;
    char file_name[128] = {0x00};
    if(!ppRequest || !boundary){
        return -1;
    }
    //filename
    memset(file_name, 0x00, sizeof(file_name));
    if (strlen(g_acFileNameSnap)<sizeof(file_name))
    	memcpy(file_name,g_acFileNameSnap, strlen(g_acFileNameSnap));
    else
    	return -3;

    ret = buffer_appendf(*ppRequest, POSTFIELD_BOUNDARY, boundary);
    CHECK_ERROR(ret);
    if (file_ext[0]=='j')
    	ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_FILE, file_name);
    else
    	ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_FILE_CLIP, file_name);
    CHECK_ERROR(ret);
    if (file_ext[0]=='j')
    	ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_TYPE_SNAPSHOT);
    else
    	ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_TYPE_CLIP);
    CHECK_ERROR(ret);
    return ret;
}

// static int gen_motion_filename(char* g_generated_filename, int g_generated_filename_len)
// {
//     int ret = 0;
//     char file_name[128] = {0x00};

//     // printf("%s entered!\r\n", __func__);
//     //filename
//     memset(file_name, 0x00, 128);
//     if(get_file_name_motion(file_name) < 0)
//     {
//         printf("Get file_name system failed!\r\n");
//         printf("----------------------------\r\n");
//         return -2;
//     }

//     // printf("%s memset g_generated_filename!\r\n", __func__);
//     memset(g_generated_filename, 0x00, HTTP_UPLOAD_MAX_FILENAME_LEN);

//     if (strlen(file_name) < g_generated_filename_len){
//         memcpy(g_generated_filename, file_name, strlen(file_name));
//     }
//     else{
//         debug("Length filename is so long!\r\n");
//         return -3;
//     }
//     printf("file name snapshot: %s\r\n", g_generated_filename);
//     return ret;
// }

/*
* Build tail for upload snapshot
*/
static int set_request_property_tail(Buffer **ppRequest, CON_TAIL_SNAP_t *psTailSnap)
{
    int ret = 0;
    if(!ppRequest || !psTailSnap){
        debug("request null!");
        return -1;
    }
    /* Boundary */
    ret = buffer_appendf(*ppRequest, "\r\n"POSTFIELD_BOUNDARY, psTailSnap->boundary);
    CHECK_ERROR(ret);
    /* Build submit */
    ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_X, FIELD_SUBMIT);
    CHECK_ERROR(ret);
    ret = buffer_appendf(*ppRequest, POSTFIELD_CONTENT_DISPOSITION_VALUE, psTailSnap->submit);
    CHECK_ERROR(ret);
    /* Boundary end */
    ret = buffer_appendf(*ppRequest, POSTFIELD_BOUNDARY_END, psTailSnap->boundary);
    CHECK_ERROR(ret);
    return ret;
}

int g_body_field1_length=0;
static char *prebuild_header_get_length_snapshot(char *req_path, char *host_name, char *response, char* file_ext)
{
    char *request = NULL;
    Buffer *request_buffer = buffer_alloc(POST_REQUEST_HEADER_SIZE);
    char *boundary = NULL;
    int ret_val = 0;
    int temp=0;
    //char connect_type[] = "Close";
    char usr_agent[] = "cincamera";
    char submit[] = "Upload to Amazon S3";

    CON_HEADER_SNAP_t header_info;
    CON_TAIL_SNAP_t tail_info;

//    debug("Entered!");
    /* Generate boundary */
    boundary = gen_boundary();
//    debug("boundary: %s", boundary);

	debug("req_path %s host_name %s!\r\n",req_path,host_name);
    //hardcode test
    header_info.req_path = req_path;
    header_info.host_name = host_name;

    //header_info.connect_type = connect_type;
    header_info.connect_type = (char *)malloc(strlen(CON_CONNECTION_TYPE) + 10);
    memset(header_info.connect_type, 0x00, strlen(CON_CONNECTION_TYPE) + 10);
    memcpy(header_info.connect_type, CON_CONNECTION_TYPE, strlen(CON_CONNECTION_TYPE));

    header_info.usr_agent = usr_agent;
    header_info.boundary = boundary;
    header_info.total_length = 0;

    tail_info.boundary = boundary;
    tail_info.submit = submit;

    /* Init struct header */
    ret_val = set_request_property_header(&request_buffer, &header_info);
    if(ret_val < 0){
        //return NULL;
        goto ERROR;
    }
    temp = strlen(buffer_to_s(request_buffer));
//    debug("PrebuildSNAP: Header!");
    ret_val = set_request_property_body_snapshot(&request_buffer, response, boundary, file_ext);
    debug("PrebuildSNAP: Body header %d!",ret_val);
    if(ret_val < 0){
        // return NULL;
        goto ERROR;
    }
    debug("PrebuildSNAP: Body header!");
    ret_val = set_request_property_file_snapshot(&request_buffer, boundary, file_ext);
    if(ret_val < 0){
        // return NULL;
        goto ERROR;
    }
    g_body_field1_length = strlen(buffer_to_s(request_buffer)) - temp;
//    debug("PrebuildSNAP: Finish header!");
    // ret_val = set_request_property_tail(&request_buffer, &tail_info);
    // if(ret_val < 0){
    //     // return NULL;
    //     goto ERROR;
    // }

    request = buffer_to_s(request_buffer);

    buffer_free(request_buffer);
    free_boundary(boundary);

    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
    return request;

ERROR:
    printf("\r\n%s ERROR return !\r\n", __func__);
    buffer_free(request_buffer);
    free_boundary(boundary);
    return NULL;
}

char *build_request_upload_snapshot(char *req_path, char *host_name, char *response, int *len_stream, char* l_filename, char* l_file_ext, char* g_generated_filename, int g_generated_filename_len)
{
    char *request = NULL;
    char *request_tail = NULL;
    char *pre_build = NULL;
    Buffer *request_buffer = buffer_alloc(POST_REQUEST_HEADER_SIZE);
    Buffer *request_buffer_tail = buffer_alloc(POST_REQUEST_HEADER_SIZE);
    char *boundary = NULL;
    int ret_val = 0;
    //char connect_type[16] = "keep-alive";
    char usr_agent[16] = "cincamera";
    char submit[64] = "Upload to Amazon S3";
    int length_pre_build = 0;
    int file_size = 0;
    char str_total_len[32] = {0x00};
    char l_full_filename[128]={0};
    
    CON_HEADER_SNAP_t header_info;
    CON_TAIL_SNAP_t tail_info;
    int length_req = 0;
    int length_tail = 0;
/*----------------*/
    char *data_to_send;
/* Hardcode test */
    FILE *fp = NULL;
    const int INT_SIZE_DATA = (2*1024);
    char data[INT_SIZE_DATA];
    int byte_read = 0;
    int cnt_read = 0;
/*****************/
    debug("get_filename\r\n");
    if (gen_motion_filename(g_generated_filename, g_generated_filename_len)<0)
        return -1;
    debug("get_filename done: %s\r\n", g_generated_filename);
    memcpy(g_acFileNameSnap,g_generated_filename,strlen(g_generated_filename));
    memcpy(g_acFileNameClip,g_generated_filename,strlen(g_generated_filename));
    debug("l_filename: %s, l_file_ext: %s\r\n", l_filename, l_file_ext);
    snprintf(l_full_filename,sizeof(l_full_filename),"%s.%s",l_filename,l_file_ext);
    debug("l_full_filename %s\r\n",l_full_filename);

    /* Generate boundary */
    boundary = gen_boundary();
    debug("boundary: %s", boundary);

    /* Pre build header and get total length */
    pre_build = prebuild_header_get_length_snapshot(req_path, host_name, response, l_file_ext);

    if(!pre_build){
        // return NULL;
        goto ERROR;
    }
    debug("pre_build OK");
    length_pre_build = strlen(pre_build);

    printf("----------------------------------\r\nlength_pre_build: %d<<<\r\n", length_pre_build);
#ifndef  UPLOADER_RUN_ON_AK_PLAT
    file_size = get_file_size_snap(l_full_filename) + 2; // for /r/n end of file
#else
    file_size = (int)snapshot_get_size_data() + 2;

    printf("***** file_size: %d*****\r\n", file_size);
#endif
    //hardcode test
    header_info.req_path = req_path;
    header_info.host_name = host_name;
    //header_info.connect_type = connect_type;
    header_info.connect_type = (char *)malloc(strlen(CON_CONNECTION_TYPE) + 10 );
    if(header_info.connect_type == NULL)
    {
        printf("\r\nERROR allocate memory for header_info.connect_type failed! \r\n");
        goto ERROR;
    }
    memset(header_info.connect_type, 0x00, strlen(CON_CONNECTION_TYPE) + 10);
    memcpy(header_info.connect_type, CON_CONNECTION_TYPE, strlen(CON_CONNECTION_TYPE));
    header_info.usr_agent = usr_agent;
    header_info.boundary = boundary;

    //FIX ME:
    tail_info.boundary = boundary;
    tail_info.submit = submit;

    ret_val = set_request_property_tail(&request_buffer_tail, &tail_info);
    if(ret_val < 0){
        // return NULL;
        goto ERROR;
    }
    request_tail = buffer_to_s(request_buffer_tail);
    if(!request_tail){
        goto ERROR;
    }
    // FIXME: append 0x0D 0x0A to fix issue "The request body terminated unexpectedly"
    length_tail = strlen(request_tail)-2;
    debug("Tail: %s", request_tail);
    // *(request_tail + length_tail) = 0x0D;
    // *(request_tail + length_tail + 1) = 0x0A;
    // length_tail += 2;
    //--------------------------------------------------------------------------
    printf("-------------update length of tail!-----------------\r\n");

    memset(str_total_len, 0x00, sizeof(str_total_len));
    sprintf(str_total_len, "%d", length_pre_build + file_size + length_tail);
    //header_info.total_length = length_pre_build + file_size + length_tail + strlen(str_total_len) + 5;
    //*len_stream = header_info.total_length;
    //printf("\r\n%s Total_length: %d",  __func__, header_info.total_length);
    //printf("\r\n%s length_pre_build: %d, file_size: %d, length_tail: %d",  __func__, length_pre_build, file_size, length_tail);
    header_info.total_length = g_body_field1_length + file_size + length_tail;
    debug("Total_length: %d %d %d %d", header_info.total_length, g_body_field1_length, file_size, length_tail);
	/* Init struct header */
    ret_val = set_request_property_header(&request_buffer, &header_info);
    if(ret_val < 0){
        // return NULL;
        goto ERROR;
    }

    debug("Header!");
    ret_val = set_request_property_body_snapshot(&request_buffer, response, boundary, l_file_ext);
    debug("Body header %d!",ret_val);
    if(ret_val < 0){
        // return NULL;
        goto ERROR;
    }

    ret_val = set_request_property_file_snapshot(&request_buffer, boundary, l_file_ext);
    if(ret_val < 0){
        // return NULL;
        goto ERROR;
    }
    debug("Finish header! byte_used: %d ", request_buffer->bytes_used);

    request = buffer_to_s(request_buffer);
    if(!request){
        debug("Request null!");
        goto ERROR;
    }
    length_req = strlen(request);
    debug("length_req: %d", length_req);
    *len_stream = length_req + file_size + length_tail;
    debug("len_stream: %d %d %d", length_req,file_size,length_tail);
    data_to_send = malloc(*len_stream+10);
    if(!data_to_send){
        printf("SNAPSHOT: Cannot allocate memory for data_to_send");
        goto ERROR;
    }
    memcpy(data_to_send, request, length_req);
    printf("\r\nHeader: %s\r\n",data_to_send);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    debug("upload %s\n",l_full_filename);
    fp = fopen_api(l_full_filename, "r");
    if(!fp){
        debug("open file failed!");
    }
    else{
        debug("Read file! request:%p, file_size: %d", request, file_size);
        cnt_read = 0;
        while(cnt_read < (file_size-2)){
            memset(data, 0x00, INT_SIZE_DATA);
            byte_read = fread_api(data, sizeof(char), INT_SIZE_DATA, fp);
            memcpy((data_to_send + length_req + cnt_read), data, byte_read);
            cnt_read += byte_read;
            debug("Have read %d bytes, %d/%d", byte_read, cnt_read, file_size);
            if(byte_read < 0){
                debug("Read snapshot file failed!");
                goto ERROR;
            }
        }
        debug("End of file! cnt_read: %d, file_size: %d", cnt_read, file_size);
        *(data_to_send + length_req + cnt_read) = '\r';
        *(data_to_send + length_req + cnt_read + 1) = '\n';
        //length_req += (cnt_read +2);

        debug("Finish reading file %d %d %d!",length_req,cnt_read + 2,length_tail);
        memcpy(data_to_send + length_req + cnt_read + 2, request_tail, length_tail);
        debug("Done copy request_tail %s!",request_tail);
                 
        //fclose(fp);
        
        debug("Close file fp!");
    }
#else
    // Platform anyka
    cnt_read = 0;    
    memcpy((data_to_send + length_req + cnt_read), \
                    g_sDataSnapshot4Uploader.data, \
                    file_size - 2);
    cnt_read += g_sDataSnapshot4Uploader.length;
    // cnt_read = cnt_read - 2;
    printf("\r\n---------------------------------------\r\n");
    *(data_to_send + length_req + cnt_read) = '\r';
    *(data_to_send + length_req + cnt_read + 1) = '\n';
    memcpy(data_to_send + length_req + cnt_read + 2, request_tail, length_tail);
    printf("\r\n%s Done copy request_tail!\r\n",  __func__);
#endif

    // ret_val = set_request_property_tail(&request_buffer_tail, &tail_info);
    // if(ret_val < 0){
    //     // return NULL;
    //     goto ERROR;
    // }

    
    buffer_free(request_buffer);
    buffer_free(request_buffer_tail);
    free_boundary(boundary);

    // ak_sleep_ms(300); // insert delay to printf log over UART
    printf("%s Length of stream: %d\r\n", __func__, strlen(data_to_send));
    printf("-----------------------------------------------\r\n");
    return data_to_send; // pointer

ERROR:
    buffer_free(request_buffer);
    buffer_free(request_buffer_tail);
    free_boundary(boundary);
    #ifndef UPLOADER_RUN_ON_AK_PLAT
    if(fp){
        fclose_api(fp);
    }
    #endif
    return NULL;
}

/*
* parse respone after uploading
*/
int uploading_parse_repsonse(char *response, int *http_status_code, char *location)
{
    char *pPosHead = NULL;
    char *pPosLocation = NULL;
    char *pMsg = NULL;
    char *pHttpStt = NULL;
    char *pCR = NULL;
    char strFirstLine[256] = {0x00};
    int  i = 0;
    int len = 0;

    if(!response){
        return -1;
    }
    
    printf(" response %s\r\n",response);
    printf(" STR_UPLOAD_SNAP_OK_HEAD %s\r\n",STR_UPLOAD_SNAP_OK_HEAD);
    
    pPosHead = strstr(response, STR_UPLOAD_SNAP_OK_HEAD);

    if(!pPosHead){
        debug("Don't undestand response");
        return -1;
    }
    else{
        // find http error code
        pMsg = response + strlen(STR_UPLOAD_SNAP_OK_HEAD) + 1;  
        pCR = strstr(pMsg, "\r");
        memset(strFirstLine, 0x00, sizeof(strFirstLine));
        memcpy(strFirstLine, pMsg, (int)pCR - (int)pMsg);
        for(i = 0; i < HTTP_STT_NUMBER; i++){
            // pHttpStt = NULL;
            pHttpStt = strstr(strFirstLine, g_astrHttpSttSuccess[i]);
            if(pHttpStt){
                break;
            }
        }
        if(pHttpStt){
            debug("pHttpStt: %p, pMsg: %p", pHttpStt, pMsg);
            //if(pHttpStt == pMsg){
                debug("Upload success: HTTP Status code:%s", g_astrHttpSttSuccess[i]);
                if(http_status_code){
                    *http_status_code = 200 + i;
                }
                if(location){
                    pPosLocation = strstr(response, STR_UPLOAD_SNAP_OK_LOCATION);
                    if(pPosLocation){
                        pMsg = pPosLocation + strlen(STR_UPLOAD_SNAP_OK_LOCATION) + 1;
                        pCR = strstr(pMsg, "\r");
                        if(pCR){
                            len = (int)pCR - (int)pMsg;
                            if(len > 0){
                                memcpy(location, pMsg, len);
                            }
                        }
                    }
                }
            //}
            return 0;
        }
    }
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Functions for upload clip                                                 */
/*---------------------------------------------------------------------------*/

/*
POST / HTTP/1.1
Host: cinatic-resources.s3.amazonaws.com
Connection: keep-alive
Content-Length: 2552839
Cache-Control: max-age=0
Origin: null
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryfVvASBTEqFLuupX6
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*_/_*;q=0.8
Accept-Encoding: gzip, deflate
Accept-Language: en-US,en;q=0.8

------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="key"

00000274DAEA813043PAVAIX/8ccbd740-e9f4-11e6-b342-79487726f07f/${filename}
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="acl"

private
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="Content-Type"

application/octet-stream
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="x-amz-meta-uuid"

8ccbd740-e9f4-11e6-b342-79487726f07f
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="x-amz-server-side-encryption"

AES256
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="X-Amz-Credential"

AKIAIH5TTUQA5CLP5EKQ/20170203/us-east-1/s3/aws4_request
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="X-Amz-Algorithm"

AWS4-HMAC-SHA256
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="X-Amz-Date"

20170203T000000Z
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="x-amz-meta-tag"


------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="Policy"

eyAiZXhwaXJhdGlvbiI6ICIyMDE3LTAyLTA0VDA5OjM4OjQyLjAwMFoiLCJjb25kaXRpb25zIjogW3siYnVja2V0IjogImNpbmF0aWMtcmVzb3VyY2VzIn0sWyJzdGFydHMtd2l0aCIsICIka2V5IiwgIjAwMDAwMjc0REFFQTgxMzA0M1BBVkFJWCJdLHsiYWNsIjogInByaXZhdGUifSxbInN0YXJ0cy13aXRoIiwgIiRDb250ZW50LVR5cGUiLCAiYXBwbGljYXRpb24vb2N0ZXQtc3RyZWFtIl0seyJ4LWFtei1tZXRhLXV1aWQiOiAiOGNjYmQ3NDAtZTlmNC0xMWU2LWIzNDItNzk0ODc3MjZmMDdmIn0seyJ4LWFtei1zZXJ2ZXItc2lkZS1lbmNyeXB0aW9uIjogIkFFUzI1NiJ9LFsic3RhcnRzLXdpdGgiLCAiJHgtYW16LW1ldGEtdGFnIiwgIiJdLHsieC1hbXotY3JlZGVudGlhbCI6ICJBS0lBSUg1VFRVUUE1Q0xQNUVLUS8yMDE3MDIwMy91cy1lYXN0LTEvczMvYXdzNF9yZXF1ZXN0In0seyJ4LWFtei1hbGdvcml0aG0iOiAiQVdTNC1ITUFDLVNIQTI1NiJ9LHsieC1hbXotZGF0ZSI6ICIyMDE3MDIwM1QwMDAwMDBaIiB9XX0=
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="X-Amz-Signature"

50761350678135e516049f318da543635c42c12dc33ff5ce26f0dd410c6f9d03
------WebKitFormBoundaryfVvASBTEqFLuupX6
Content-Disposition: form-data; name="file"; filename="74DAEA813043_20170202161839.flv"
Content-Type: video/x-flv
*/

/*---------------------------------------------------------------------------*/
/* Functions for event upload success                                        */
/*---------------------------------------------------------------------------*/
/*
*   Parse info
*/
static int parse_success_info(char *info, CON_UPLOAD_SUCCESS_INFO_t *success_info)
{
    int ret = 0;
    int i = 0;
    char *input = info;
    char *pos = NULL;
    char *pos_and = NULL;
    int len_cpy = 0;
    int len_pattern = 0;

    if(!info || !success_info){
        debug("parse_success_info Argument null!");
        return -1;
    }

    if(success_info->event_id == NULL || \
        success_info->udid == NULL || \
        success_info->update_date == NULL || \
        success_info->token == NULL){
        debug("Need to allocate memory for success_info struct!");
        return -2;
    }

    // event id
    pos = strstr(input, PATTERN_STR_EVENT_ID);
    pos_and = strstr(input, END_PATTERN_PARSE_SUCCESS_INFO);
    if(!pos || !pos_and){
        debug("Cannot find & or event_id string!");
        return -3;
    }
    else{
        len_pattern = strlen(PATTERN_STR_EVENT_ID);
        len_cpy = (int)pos_and - (int)pos - len_pattern;
        // debug("len EV_ID: %d, pos: %p, pos_and: %p, len_cpy: %d", len_pattern, pos, pos_and, len_cpy);
        memcpy(success_info->event_id, pos + len_pattern, len_cpy);
        input = (char *)((int)pos_and + 1);
    }
    debug("EventID: %s", success_info->event_id);
    // update_date
    pos = strstr(input, PATTERN_STR_UPDATE_DATE);
    pos_and = strstr(input, END_PATTERN_PARSE_SUCCESS_INFO);
    if(!pos || !pos_and){
        debug("Cannot find & or update date string!");
        return -3;
    }
    else{
        len_pattern = strlen(PATTERN_STR_UPDATE_DATE);
        len_cpy = (int)pos_and - (int)pos - len_pattern;
        memcpy(success_info->update_date, pos + len_pattern, len_cpy);
        input = (char *)((int)pos_and + 1);
    }
    debug("Update date: %s", success_info->update_date);
    // token
    pos = strstr(input, PATTERN_STR_TOKEN);
    pos_and = strstr(input, END_PATTERN_PARSE_SUCCESS_INFO);
    if(!pos || !pos_and){
        debug("Cannot find & or token string!");
        return -3;
    }
    else{
        len_pattern = strlen(PATTERN_STR_TOKEN);
        len_cpy = (int)pos_and - (int)pos - len_pattern;
        memcpy(success_info->token, pos + len_pattern, len_cpy);
        input = (char *)((int)pos_and + 1);
    }
    debug("Token: %s", success_info->token);
    // debug("input: %s", input);
    // udid
    pos = strstr(input, PATTERN_STR_REG_ID);
    if(!pos){
        debug("Cannot find registration id string!");
        return -3;
    }
    else{
        len_pattern = strlen(PATTERN_STR_REG_ID);
        len_cpy = (int)strlen(input) - len_pattern;
        // debug("len UDID: %d, pos: %p, len_cpy: %d", len_pattern, pos, len_cpy);
        memcpy(success_info->udid, pos + len_pattern, len_cpy);
    }
    debug("UDID: %s", success_info->udid);
    return ret;
}

/*
* Init struct success info
*/
static int initialize_success_info_structure(CON_UPLOAD_SUCCESS_INFO_t *success_info)
{
    if(!success_info){
        debug("success_info null");
        return -1;
    }
    success_info->file_type = 0;

    success_info->event_id = malloc(CON_SIZE_STRING_ELEMENT_INFO);
    if(success_info->event_id == NULL){
        return -2;
    }
    memset(success_info->event_id, 0x00, CON_SIZE_STRING_ELEMENT_INFO);
    
    success_info->udid = malloc(CON_SIZE_STRING_ELEMENT_INFO);
    if(success_info->udid == NULL){
        return -3;
    }
    memset(success_info->udid, 0x00, CON_SIZE_STRING_ELEMENT_INFO);

    success_info->update_date = malloc(CON_SIZE_STRING_ELEMENT_INFO);
    if(success_info->update_date == NULL){
        return -2;
    }
    memset(success_info->update_date, 0x00, CON_SIZE_STRING_ELEMENT_INFO);

    success_info->file_name = malloc(CON_SIZE_STRING_ELEMENT_INFO);
    if(success_info->file_name == NULL){
        return -2;
    }
    memset(success_info->file_name, 0x00, CON_SIZE_STRING_ELEMENT_INFO);

    success_info->token = malloc(CON_SIZE_STRING_ELEMENT_INFO);
    if(success_info->token == NULL){
        return -2;
    }
    memset(success_info->token, 0x00, CON_SIZE_STRING_ELEMENT_INFO);
    return 0;
}

static int deinit_success_info_structure(CON_UPLOAD_SUCCESS_INFO_t *success_info)
{
    int ret = 0;
    if(success_info){
        if(success_info->event_id){
            free(success_info->event_id);
            success_info->event_id = NULL;
        }
        if(success_info->udid){
            free(success_info->udid);
            success_info->udid = NULL;
        }
        if(success_info->update_date){
            free(success_info->update_date);
            success_info->update_date = NULL;
        }
        if(success_info->file_name){
            free(success_info->file_name);
            success_info->file_name = NULL;
        }
        if(success_info->token){
            free(success_info->token);
            success_info->token = NULL;
        }
    }
    return ret;
}

/*
* After getting response form http POST event add and upload snapshot, 
* this function is needed to call
* This will parse and store [success_action_redirect]
*/
int parse_info_success_upload(char *response, int type)
{
    int ret = 0;
    JsonNode *json_info = NULL;
    char info[1024] = {0x00};

    /* Check invalid parameter */
    if(type < 0 || type > 4){
        debug("file_type invalid!");
        return -1;
    }

    /* Check global variable file_name */
    if(strlen(g_acFileNameSnap) <= 0){
        debug("File name snapshot is not set yet!");
        return -1;
    }

    /* parse json from response string */
    json_info = jh_get_json_node_coredata(response);
    if(!json_info){
        debug("Json Info null!");
        return -2;
    }

    /* parse information of upload successful with success_action_redirect key */
    memset(info, 0x00, sizeof(info));
    ret = jh_find_data_by_key(json_info, KEY_SUCCESS_ACTION_REDIRECT, info);
    if(ret < 0){
        debug("Parse json to find success_action_redirect FAILED!");
        ret = -3;
    }
    else{
        // parse info
        debug("info: %s", info);
        //TODO: free memory g_asUploadSuccess
        if(initialize_success_info_structure(&g_asUploadSuccess) == 0){
            ret = parse_success_info(info, &g_asUploadSuccess);
            if(ret < 0){
                ret = -5;
                debug("parse_success_info return failed!");
            }
            else{
                /* copy file name */
                if(type == 1){
                    sprintf(g_asUploadSuccess.file_name, "%s"CON_FILE_EXTENTION_SNAPSHOT, g_acFileNameSnap);
                }
                else if(type == 2){
                    sprintf(g_asUploadSuccess.file_name, "%s"CON_FILE_EXTENTION_CLIP, g_acFileNameClip);
                }
                g_asUploadSuccess.file_type = type;
            }
        }
        else{
            debug("initialize_success_info_structure failed!");
            ret = -4;
        }
    }
    return ret;
}

/*
* Build request get for event upload successful
* Snapshot: type = 1, clip type = 2
*/
char * build_body_url_get_method_upload_success(CON_UPLOAD_SUCCESS_INFO_t *success_info)
{
    char *body_url = NULL;
    int len_url = 0;
    int len_ev_id = 0, len_date = 0, len_type = 0, len_file_name = 0;
    int len_token = 0, len_udid = 0;
    int shift = 0;

    if(!success_info){
        return NULL;
    }
    else{
        // check element structure
        if(success_info->event_id == NULL || \
            success_info->udid == NULL || \
            success_info->update_date == NULL || \
            success_info->file_name == NULL || \
            success_info->token == NULL){
            return NULL;
        }
    }
    // Calculate length of url
    /* +1 for '&' */
    len_ev_id = strlen(success_info->event_id);
    len_udid = strlen(success_info->udid);
    len_date = strlen(success_info->update_date);
    len_type = 1; // a character for file type
    len_file_name = strlen(success_info->file_name);
    len_token = strlen(success_info->token);

    len_url = strlen(PATTERN_STR_EVENT_ID) + len_ev_id + 1 + \
            strlen(PATTERN_STR_REG_ID) + len_udid + 1 + \
            strlen(PATTERN_STR_UPDATE_DATE) + len_date + 1 + \
            strlen(PATTERN_STR_FILE_TYPE) + len_type + 1 + \
            strlen(PATTERN_STR_FILE_NAME) + len_file_name + 1 + \
            strlen(PATTERN_STR_TOKEN) + len_token + 64;

    body_url = (char *)malloc(len_url);
    if(!body_url){
        debug("cannot allocate memory for body url");
        return NULL;
    }
    memset(body_url, 0x00, len_url);

    sprintf(body_url, "%s%s&%s%s&%s%s&%s%d&%s%s&%s%s",
                PATTERN_STR_EVENT_ID, success_info->event_id, \
                PATTERN_STR_REG_ID, success_info->udid, \
                PATTERN_STR_UPDATE_DATE, success_info->update_date, \
                PATTERN_STR_FILE_TYPE, success_info->file_type, \
                PATTERN_STR_FILE_NAME, success_info->file_name, \
                PATTERN_STR_TOKEN, success_info->token);
    debug("body_url:\n%s\n", body_url);
    return body_url;
}


/*
* Build request get method
*/
static char *build_request_get_method_upload_success(char *body_url, 
                                        char *hostname, char *request_path, char *port)
{
    char *request = NULL;
    Buffer *request_buffer = buffer_alloc(2*1024);

    if(!body_url || !hostname || !request_path){
        return NULL;
    }

    buffer_appendf(request_buffer, "GET %s?%s HTTP/1.0\r\n", request_path, body_url);
    buffer_appendf(request_buffer, "Host: %s:%s\r\n", hostname, (port==NULL)?"80":port);
    buffer_appendf(request_buffer, "Connection: keep-alive\r\n");
    buffer_appendf(request_buffer, "Accept: application/json\r\n");
    buffer_appendf(request_buffer, "User-Agent: cincamera\r\n");
    buffer_appendf(request_buffer, "Referer: http://dev-api.cinatic.com/docs/\r\n");
    buffer_appendf(request_buffer, "Accept-Encoding: gzip, deflate, sdch\r\n");
    buffer_appendf(request_buffer, "Accept-Language: en-US,en;q=0.8\r\n\r\n");

    request = buffer_to_s(request_buffer);
    buffer_free(request_buffer);

    return request;
}


/*
* Build request get method
*/
#define L_BUF_SIZE 2048
static int build_request_get_method(char *body_url, int body_url_len,
                                        char *hostname, char *request_path)
{
    char local_buffer1[L_BUF_SIZE]={0};
    char local_buffer2[L_BUF_SIZE]={0};
    
    snprintf(local_buffer2,L_BUF_SIZE, "%sGET %s HTTP/1.1\r\n", local_buffer1, request_path);
    memcpy(local_buffer1,local_buffer2,L_BUF_SIZE);
    snprintf(local_buffer2,L_BUF_SIZE, "%sHost: %s\r\n", local_buffer1, hostname);
    memcpy(local_buffer1,local_buffer2,L_BUF_SIZE);
    snprintf(local_buffer2,L_BUF_SIZE, "%sConnection: keep-alive\r\n", local_buffer1);
    memcpy(local_buffer1,local_buffer2,L_BUF_SIZE);
    snprintf(local_buffer2,L_BUF_SIZE, "%sAccept: application/json\r\n", local_buffer1);
    memcpy(local_buffer1,local_buffer2,L_BUF_SIZE);
    snprintf(local_buffer2,L_BUF_SIZE, "%sUser-Agent: cincamera\r\n", local_buffer1);
    memcpy(local_buffer1,local_buffer2,L_BUF_SIZE);
    //sprintf(local_buffer,sizeof(local_buffer), "%sReferer: http://dev-api.cinatic.com/docs/\r\n", local_buffer1);
    snprintf(local_buffer2,L_BUF_SIZE, "%sAccept-Encoding: gzip, deflate, sdch\r\n", local_buffer1);
    memcpy(local_buffer1,local_buffer2,L_BUF_SIZE);
    snprintf(local_buffer2,L_BUF_SIZE, "%sAccept-Language: en-US,en;q=0.8\r\n\r\n", local_buffer1);
    
    snprintf(body_url,body_url_len, "%s", local_buffer2);
    return 0;
}



/*
* make request
*/
static int build_request_get_method(char *body_url, int body_url_len, char *hostname, char *request_path);

int make_request_event_upload_snap(int sockfd, char *hostname, char *request_path, char *port)
{
    char request[2048]={0};
    char *body_url = NULL;
    size_t bytes_sent       = 0;
    size_t total_bytes_sent = 0;
    size_t bytes_to_send    = 0;
    int ret = 0;

    debug("Test point 1!");
    if((sockfd < 0) || !hostname || !request_path || !port){
        debug("Argument null!");
        return -1;
    }

    // make body_url
    /*body_url = build_body_url_get_method_upload_success(&g_asUploadSuccess);
    if(!body_url){
        debug("Build URL failed!");
        return -1;
    }*/
    //request = build_request_get_method_upload_success(body_url, hostname, request_path, port);
    printf("hostname %s \r\n",hostname);
    printf("request_path %s \r\n",request_path);
    build_request_get_method(request, sizeof(request), hostname, request_path);
    bytes_to_send = strlen(request);
    printf("\r\nrequest %s \r\n",request);

    while (1) {
#ifdef HTTP_TEST_ON_LINUX
        bytes_sent = send_api(sockfd, request + total_bytes_sent, strlen(request) - total_bytes_sent, 0);
#else
        bytes_sent = tcp_write(sockfd, request, bytes_to_send);
#endif
        if(bytes_sent < 0){
            ret = -1;
            break;
        }
        total_bytes_sent += bytes_sent;
        debug("Bytes sent: %ld", bytes_sent);
        if (total_bytes_sent >= bytes_to_send) {
            break;
        }
    }

    printf("Send data with zero length to CC3200\r\n");
    bytes_sent = send_api(sockfd, request + total_bytes_sent, 0, 0);
    if(ret == 0){
        return total_bytes_sent;
    }
    else{
        return ret;
    }
}


int http_get(int sockfd, char *hostname, char *request_path, char *port)
{
    char *request = NULL;
    char *body_url = NULL;
    size_t bytes_sent       = 0;
    size_t total_bytes_sent = 0;
    size_t bytes_to_send    = 0;
    int ret = 0;

    debug("Test point 1!");
    if((sockfd < 0) || !hostname || !request_path || !port){
        debug("Argument null!");
        return -1;
    }

    // make body_url
    body_url = build_body_url_get_method_upload_success(&g_asUploadSuccess);
    if(!body_url){
        debug("Build URL failed!");
        return -1;
    }
    request = build_request_get_method_upload_success(body_url, hostname, request_path, port);
    bytes_to_send = strlen(request);

    while (1) {
        bytes_sent = send_api(sockfd, request + total_bytes_sent, strlen(request) - total_bytes_sent, 0);
        if(bytes_sent < 0){
            ret = -1;
            break;
        }
        total_bytes_sent += bytes_sent;
        debug("Bytes sent: %ld", bytes_sent);
        if (total_bytes_sent >= bytes_to_send) {
            break;
        }
    }

    free(body_url);
    free(request);
    if(ret == 0){
        return total_bytes_sent;
    }
    else{
        return ret;
    }
}
/*
* parse respone after uploading
*/
int send_event_success_parse_repsonse(char *response)
{
    char *pPosHead = NULL;
    char *pPosLocation = NULL;
    char *pMsg = NULL;
    char *pHttpStt = NULL;
    char *pCR = NULL;
    char strFirstLine[256] = {0x00};
    int  i = 0;
    int len = 0;

    if(!response){
        return -1;
    }
    pPosHead = strstr(response, STR_UPLOAD_SNAP_OK_HEAD);
    if(!pPosHead){
        debug("Don't undestand response");
        return -1;
    }
    else{
        // find http error code
        pMsg = response + strlen(STR_UPLOAD_SNAP_OK_HEAD) + 1;  
        pCR = strstr(pMsg, "\r");
        memset(strFirstLine, 0x00, sizeof(strFirstLine));
        memcpy(strFirstLine, pMsg, (int)pCR - (int)pMsg);
        for(i = 0; i < HTTP_STT_NUMBER; i++){
            // pHttpStt = NULL;
            pHttpStt = strstr(strFirstLine, g_astrHttpSttSuccess[i]);
            if(pHttpStt){
                break;
            }
        }
        if(pHttpStt){
            debug("pHttpStt: %p, pMsg: %p", pHttpStt, pMsg);
            debug("Upload success: HTTP Status code:%s", g_astrHttpSttSuccess[i]);
            return 0;
        }
    }
    return -1;
}




