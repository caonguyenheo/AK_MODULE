#ifdef HTTP_TEST_ON_LINUX

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif

#include <stdlib.h>
#include <stdio.h>

#include "url.h"
#include "connect.h"

#include "dbg.h"
#include <buffer.h>
#include "http.h"

#include "myjsmn.h"
#include "url_parser.h"

#include "upload_main.h"
#include "dbg_uploader.h"
#include "ak_apps_config.h"

#include "ak_video.h"

// -------------End Top Level------------------------------

static int g_iCntPacket = 0;

int upload_snapshot(char *data, int byte_to_send, char *amz);
int http_send_event_upload_success_snapshot(char *file_ext, char *l_url, char *port);
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
extern char g_UDID[64];;
extern char g_devicetoken[128];
extern char g_key_hex[65];
extern char g_rnd_hex[65];
extern char g_sip[16];
extern char g_sp[8];

char g_eventid[128];
char g_date[32];
char g_generated_filename[MAX_FILENAME_LEN+1];
char success_action_redirect[512];

extern char g_api_url[];
extern char g_api_port[];
// #define API_URL g_api_url
// #define API_PORT g_api_port

extern char g_local_jpg_file[];
extern char g_local_flv_file[];
#define SNAPSHOT_SAVED_JPG g_local_jpg_file
#define SNAPSHOT_SAVED_FLV g_local_flv_file

extern char g_macaddress[13];

/*******************************************************************************
* @brief: Convert char string to hex string 
* @param[in]:  input   - input buffer
* @param[out]: output  - output buffer
* @param[in]:  inp_len - length of input buffer
* @ret  :      0 if success, others for failure 
*/
static int
char_to_hex(char *input, char *output, int inp_len)
{
  int i;
  char buf_ptr[3] = {0};

  if(NULL == input || 0 >= inp_len) {
    return -1;
  }
  for(i = 0; i < inp_len; i++) {
    snprintf(buf_ptr, sizeof buf_ptr, "%02X", input[i]);
    strncpy(output + i*2, buf_ptr, strlen(buf_ptr));
  }
  return 0;
}

int get_system_info(void)
{
    int iRet = 0;
    char acTemp[65];
    memset(acTemp, 0x00, 65);
    memset(g_UDID, 0x00, sizeof(g_UDID));
    memset(g_devicetoken, 0x00, sizeof(g_devicetoken));
    memset(g_macaddress, 0x00, sizeof(g_macaddress));
    memset(g_api_url, 0x00, MAX_URL_LEN + 1);
    
    memset(g_key_hex, 0x00, sizeof(g_key_hex));
    memset(g_rnd_hex, 0x00, sizeof(g_rnd_hex));
    memset(g_sip, 0x00, sizeof(g_sip));
    memset(g_sp, 0x00, sizeof(g_sp));
    //--------------------------------------------------------------------------------
    // get udid
    iRet = doorbell_cfg_get_udid(acTemp);
    if(iRet == 0)
    {
        memcpy(g_UDID, acTemp, strlen(acTemp));
        printf("%s g_UDID: %s, acTemp: %s\n", __func__, g_UDID, acTemp);
    }
    else
    {
        printf("%s udid failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    // get token
    memset(acTemp, 0x00, 65);
    iRet |= doorbell_cfg_get_token(acTemp);
    if(iRet == 0)
    {
        memcpy(g_devicetoken, acTemp, strlen(acTemp));
        printf("%s g_devicetoken: %s, acTemp: %s\n", __func__, g_devicetoken, acTemp);
    }
    else
    {
        printf("%s token failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    // get mac
    memset(acTemp, 0x00, 65);
    iRet |= doorbell_cfg_get_macaddress(acTemp);
    if(iRet == 0)
    {
        memcpy(g_macaddress, acTemp, 12);
        g_macaddress[12] = '\0';
        printf("%s g_macaddress: %s, acTemp: %s\n", __func__, g_macaddress, acTemp);
    }
    else
    {
        printf("%s macaddress failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    // get url
    memset(acTemp, 0x00, 65);
    iRet |= doorbell_cfg_get_url(acTemp);
    if(iRet == 0)
    {
        memcpy(g_api_url, acTemp, strlen(acTemp));
        printf("%s g_api_url: %s, acTemp: %s\n", __func__, g_api_url, acTemp);
    }
    else
    {
        printf("%s g_api_url failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    // get key
    memset(acTemp, 0x00, 65);
    iRet |= doorbell_cfg_get_aeskey(acTemp);
    if(iRet == 0)
    {
        //memcpy(g_key_hex, acTemp, strlen(acTemp));
        char_to_hex(acTemp,g_key_hex,strlen(acTemp));
        printf("%s g_key_hex: %s, acTemp: %s\n", __func__, g_key_hex, acTemp);
    }
    else
    {
        printf("%s g_key_hex failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    // get rnd
    memset(acTemp, 0x00, 65);
    iRet |= doorbell_cfg_get_rnd(acTemp);
    if(iRet == 0)
    {
        //memcpy(g_rnd_hex, acTemp, strlen(acTemp));
        char_to_hex(acTemp,g_rnd_hex,strlen(acTemp));
        printf("%s g_rnd_hex: %s, acTemp: %s\n", __func__, g_rnd_hex, acTemp);
    }
    else
    {
        printf("%s g_rnd_hex failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    // get sip
    memset(acTemp, 0x00, 65);
    iRet |= doorbell_cfg_get_sip(acTemp);
    if(iRet == 0)
    {
        memcpy(g_sip, acTemp, strlen(acTemp));
        printf("%s g_sip: %s, acTemp: %s\n", __func__, g_sip, acTemp);
    }
    else
    {
        printf("%s g_sip failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    // get sp
    memset(acTemp, 0x00, 65);
    iRet |= doorbell_cfg_get_sp(acTemp);
    if(iRet == 0)
    {
        memcpy(g_sp, acTemp, strlen(acTemp));
        printf("%s g_sp: %s, acTemp: %s\n", __func__, g_sp, acTemp);
    }
    else
    {
        printf("%s g_sp failed!\n", __func__);
    }
    //--------------------------------------------------------------------------------
    if(strlen(g_UDID) == 0 || strlen(g_devicetoken) == 0 \
        || strlen(g_macaddress) == 0 || strlen(g_api_url) == 0)
    {
        iRet = -1;
    }
    return iRet;
}


/*
main flow uploader in here
*/
int file_post(void)
{
    unsigned long tick = 0;
    unsigned long tick_upload_snapshot = 0;
    tick = get_tick_count();
    tick_upload_snapshot = tick;
    int flagg = 1;
    void * checkfile; 

    char amz[256]={0}, amz_clip[256]={0};
    Buffer *response = buffer_alloc(10*BUF_SIZE);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    struct addrinfo *res = NULL;
#endif

    Url *url_amz,*url_amz_clip;
    int status = 0;
    int sockfd = 0;
    char *json_response = NULL;
    char *data_upload;
    int ret = 0;
    int i;

    url_parser parsed_url;

    FILE *fp = NULL;

    //*************Get info system*********************//

    ret = get_system_info();
    if(ret != 0)
    {
        printf("Uploader get info system failed! (%d)\r\n", ret);
        return -1;
    }

    //*************GET S3 Upload LINK*********************//
    // DNS resolve
#ifndef UPLOADER_RUN_ON_AK_PLAT
    status = init_connection(API_URL, API_PORT, &res);
    if ((status!=0) || (res==NULL)){
	    if (res!=NULL)
	        freeaddrinfo(res);
    	printf("ERROR cannot init connect\r\n");
	}

    // Connect to IP
    sockfd = make_connection(res);
#else

    #if(UPLOADER_USE_SSL == 1)
    status = init_connection(API_URL, API_PORT, 1);
    #else
    status = init_connection(API_URL, API_PORT, 0);
    #endif

    sockfd = status;
#endif

    if (sockfd < 0){
        printf("%s ERROR: make connection failed sockfd %d status %d\r\n", \
                __func__, sockfd, status);
    	return -1;
	}

    // POST to server
    status = make_request_event_add(sockfd, API_URL);
    if (status <= 0){
	    close_api(sockfd);
    	printf("Sending request failed");
    	return -1;
	}
        
    // Get response
    status = fetch_response(sockfd, &response, RECV_SIZE);
    close_api(sockfd);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    freeaddrinfo(res);
#endif
    if (status < 0){
	    buffer_free(response);
    	printf("Fetching response failed");
	}


    printf("\r\nPOST Motion Event response length: %d\r\n", response->bytes_used);
    printf("Post Motion Event response: ===============\r\n%s\r\n", response->contents);
    printf("Post Motion Event response: End\r\n\r\n", response->contents);
    
    // Parse response
    json_response = strstr(response->contents, "{");
    parseJson_customized(json_response,  "snapshot_url", amz, sizeof(amz));
    parseJson_customized(json_response,  "clip_url", amz_clip, sizeof(amz_clip));
    printf("\r\nsnapshot_url %s\r\n",amz);
    printf("\r\nclip_url %s\r\n",amz_clip);
    parseJson_customized(json_response,  "success_action_redirect", success_action_redirect, sizeof(success_action_redirect));
    ret = parse_url(success_action_redirect, &parsed_url);
    ret |= getValueFromKey(&parsed_url, "event_id", g_eventid, sizeof(g_eventid));
    ret |= getValueFromKey(&parsed_url, "update_date", g_date, sizeof(g_date));
    if (ret != 0) {
        printf("parse_url fails\r\n");
        return -1;
    }
    printf("event_id: %s, update_date: %s\r\n",g_eventid,g_date);

    //*************END GET S3 Upload LINK*********************//
   
    //*************UPLOAD SNAPSHOT TO S3*********************//
    int byte_to_send = 0;
    url_amz = url_parse(amz);
 
    if (url_amz==NULL){
        printf("Invalid URL supplied: '%s'\r\n", amz);
        buffer_free(response);
        return -1;
    }

    printf("path =%s= hostname =%s= \r\n",url_amz->path,url_amz->hostname);

    // Send POST header
    data_upload = build_request_upload_snapshot(url_amz->path, url_amz->hostname, json_response, &byte_to_send, SNAPSHOT_SAVED_JPG, "jpg", g_generated_filename, MAX_FILENAME_LEN);
    printf("\r\nbyte_to_send: %d\r\n",byte_to_send);
    for(i=0;i<200;i++)
    	printf("%c",data_upload[i]);
    printf("\r\n\r\nbyte_to_send: %d\r\n",byte_to_send);

    // Send POST data which is raw image then capture response
    // TCP write/read only
    
    ret = upload_snapshot(data_upload, byte_to_send, amz);

    if(data_upload){
        free(data_upload);
    }
    if (ret!=0){
	    printf("upload_snapshot fail\r\n");
	    buffer_free(response);
	    return -1;
    }

    printf("upload_snapshot success\r\n");\


    ret = http_send_event_upload_success_snapshot("jpg",API_URL,API_PORT);
    
    printf("===========================================================\r\n");
    if(ret == 0){
        printf("Done flow upload snapshot! Time of snapshot aploading: %ld ms \r\n", get_tick_count()- tick_upload_snapshot);
    }
    else{
        printf("Send event upload success failed!\r\n");
    }
    printf("===========================================================\r\n");
    url_free(url_amz);

#if 0
    //*************UPLOAD SNAPSHOT TO S3*********************//
    
        
    if (strlen(SNAPSHOT_SAVED_FLV)<=0){
		printf("No need to upload clip\r\n");
        buffer_free(response);
        return 0;
    } 

    //*************UPLOAD CLIP TO S3*********************//
    printf("UPLOAD CLIP \r\n");
    url_amz_clip = url_parse(amz_clip);
 
    if (url_amz_clip==NULL){
        printf("Invalid URL supplied: '%s'", amz);
        return -1;
    }
    printf("path %s hostname %s \r\n",url_amz_clip->path,url_amz_clip->hostname);
    // Send POST header
    data_upload = build_request_upload_snapshot(url_amz_clip->path, url_amz_clip->hostname, json_response, &byte_to_send, SNAPSHOT_SAVED_FLV, "mp4", g_generated_filename, MAX_FILENAME_LEN);
    /*printf("Clip Header\r\n");
    for(i=0;i<3300;i++)
    	if (((data_upload[i]>31) && (data_upload[i]<127)) || (data_upload[i]==10) || (data_upload[i]==13))
    	   printf("%c",data_upload[i]);
    printf("\r\n   Byte_to_send: %d\r\n",byte_to_send);*/

    // Send POST data which is raw image then capture response
    // TCP write/read only
    ret = upload_snapshot(data_upload, byte_to_send,amz_clip);
    if(data_upload){
        free(data_upload);
    }
    if (ret!=0){
	    printf("upload clip fail\r\n");
	    buffer_free(response);
	    return -1;
    }

    printf("upload clip success\r\n");
    ret = http_send_event_upload_success_snapshot("mp4",API_URL,API_PORT);
    printf("===========================================================\r\n");
    if(ret == 0){
        printf("Done flow upload clip!\r\n");
    }
    else{
        printf("Send event upload clip success failed!\r\n");
    }
    printf("===========================================================\r\n");
    url_free(url_amz_clip);
    //*************UPLOAD SNAPSHOT TO S3*********************//
    
    buffer_free(response);
#endif  

#if (RECORDING_SDCARD == 1) 
    /* Send complete event to push info file to serve */
    //Thuan add - 17/10/2017

    flagg = 1;
    while (check_status_record() != RECORD_STATE_DONE)
    {
        if ((get_tick_count() - tick) > SYS_UPDATE_INFO_CLIP_WAIT_TIMEOUT)
        {
            flagg = 0;
            break;
        }
        ak_sleep_ms(50);
    }

    if(flagg)
    {
        ret = http_send_event_upload_success_snapshot("mp4", API_URL,API_PORT);
        printf("===========================================================\r\n");
        if(ret == 0)
        {
            printf("Done flow upload clip!\r\n");
        }
        else
        {
            printf("Send event upload clip success failed!\r\n");
        }
    }
#endif
    printf("===========================================================\r\n");

    return ret;
}
// -------------End Top Level------------------------------


int http_post_try_to_send(int sockfd, char *data, int length)
{
    int rt = 0;
    if((!data) || (length <= 0) || (sockfd < 0)){
        return -1;
    }
    rt = send_api(sockfd, data, length, 0);
    //if(rt <= 0){
    if(rt < 0){
        dzlog_error("%s Need to close socket rt=%d", __func__, rt);
    }
    return rt;
}


int upload_snapshot(char *data, int byte_to_send, char *amz)
{
//    char amz[256] = {0x00};
    Buffer *response = buffer_alloc(10*BUF_SIZE);
    struct addrinfo *res = NULL;
    Url *url_amz;
    int status = 0;
    int sockfd = 0;
    char *json_response = NULL;

    int byte_send = 0;
    int cnt_send = 0;
    int ret = 0;
    int http_stt = 0;
    char location[1024] = {0x00};
    int l_length;
    int i;
    //int byte_to_send;

    url_amz = url_parse(amz);
    error_unless(url_amz, "Invalid URL supplied: '%s'", amz);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    status = init_connection(url_amz->hostname, url_amz->port, &res);
    error_unless(status==0, "ERROR cannot init connect\r");
    sockfd = make_connection(res);
#else
    // Trung Tien: We try not to use SSL to reduce time upload in China
    // sockfd = init_connection(url_amz->hostname, "80", 0);
    #if(UPLOADER_USE_SSL == 1)
    sockfd = init_connection(url_amz->hostname, url_amz->port, 1);
    #else
    sockfd = init_connection(url_amz->hostname, url_amz->port, 0);
    #endif
    status = sockfd;
#endif    
    error_unless(sockfd > 0, "ERROR: make connection failed\n");
    // status = make_request_event_add(sockfd, url_amz->hostname, url_amz->path, url_amz->port);
    // error_unless(status > 0, "Sending request failed");
    debug("byte_to_send: %d", byte_to_send);
    while(cnt_send < byte_to_send){
	    l_length = byte_to_send - cnt_send;
	    if (l_length>HTTP_CHUNK_SIZE_NKB)
            l_length=HTTP_CHUNK_SIZE_NKB;

        byte_send = http_post_try_to_send(sockfd, data + cnt_send, l_length);

        cnt_send += byte_send;
        if (byte_send<0){
	        break;
        }
    }
	printf("-------------\r\n");
    for (i=0;i<100;i++)
    	printf("%c",(data+byte_to_send-100)[i]);
    printf("\r\n-------------");
#ifdef UPLOADER_RUN_ON_AK_PLAT
    // send last packet with length 0 to notify done
    // byte_send = http_post_try_to_send(sockfd, data, 0);
    printf("Send 0 length to CC :D\r\n");
    send_api(sockfd, data, 0, 0);
#endif
    
    debug("Done send file!");
    printf("\r\nWaiting for response after POST file\r\n");

    status = fetch_response(sockfd, &response, RECV_SIZE);
    
    if (status<0)
        printf("Fetching response failed");
    
    printf("Length: %d\r\n", response->bytes_used);
    printf("%s\r\n OK \r\n", response->contents);
    /* Check response */
    ret = uploading_parse_repsonse(response->contents, &http_stt, location);
    if(ret == 0){
        debug("Link snapshot: %s", location);
    }
    close_api(sockfd);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    freeaddrinfo(res);
#endif

    buffer_free(response);
    url_free(url_amz);

    if(ret < 0){
        return -1;
    }
    return 0;
    
error:
    if (sockfd > 0)  { close_api(sockfd); }

#ifndef UPLOADER_RUN_ON_AK_PLAT
    if (res != NULL) { freeaddrinfo(res); }
#endif
    if (url_amz) { url_free(url_amz); }
    buffer_free(response);
    return 1;
}


/*
* Send event Upload success snapshot to server cinatic
*/
int http_send_event_upload_success_snapshot(char *file_ext, char *l_url, char *port)
{
    int ret = 0;
    char url_path[512];
    char file_name[256];
    Buffer *response = buffer_alloc(BUF_SIZE);
    struct addrinfo *res = NULL;
    //Url *url;
    int status = 0;
    int sockfd = 0;
    char *request = NULL;

    //Thuan add - 18/10/2017
    char acFilePath[256] = {0};
    FILE * pvFile;
    long file_size = 0;
    //char *body_url = NULL;
    //char cinatic[1024]={0};
    
    //memset(cinatic, 0x00, sizeof(cinatic));
    //memcpy(cinatic, URL_CINATIC_SUCCESS, strlen(URL_CINATIC_SUCCESS));

    //url = url_parse(cinatic);
    //error_unless(url, "Invalid URL supplied: '%s'", cinatic);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    status = init_connection(l_url, port, &res);
    error_unless(status==0, "ERROR cannot init connect\r");
    sockfd = make_connection(res);
#else

    #if(UPLOADER_USE_SSL == 1)
    status = init_connection(l_url, port, 1);
    #else
    status = init_connection(l_url, port, 0);
    #endif

    sockfd = status;
#endif
    error_unless(sockfd > 0, "ERROR: make connection failed\n");
    
    if (file_ext[0]=='j')
    {
        snprintf(url_path,sizeof(url_path),"/v1/devices/events/complete?%s&file_type=1&file_name=%s.jpg",success_action_redirect,g_generated_filename);
    }
    else
    {
        memset(file_name, 0x00, sizeof(file_name));
        if(clip_get_file_name(file_name) == 0)
        {
            // snprintf(url_path,sizeof(url_path),"/v1/devices/events/complete?%s&file_type=2&file_name=%s.mp4",success_action_redirect,g_generated_filename);
            snprintf(url_path,sizeof(url_path),"/v1/devices/events/complete?%s&file_type=2&file_name=%s.flv",success_action_redirect,file_name);
            //Thuan add - 18/10/2017
            sprintf(acFilePath, "a:/%s/%s.flv", g_acDirName_Clip, file_name);
        }
        else
        {
            snprintf(url_path,sizeof(url_path),"/v1/devices/events/complete?%s&file_type=2&file_name=%s.mp4",success_action_redirect,g_generated_filename);
            //Thuan add - 18/10/2017
            sprintf(acFilePath, "a:/%s/%s.mp4", g_acDirName_Clip, g_generated_filename);
        }
        
        //-------------------------------------------------------------check file size before upload--------------------------------------------------------
        //Thuan add - 18/10/2017
        pvFile = fopen(acFilePath, "r");
        if (pvFile == NULL)
        {
            ak_print_error_ex("check file existing: '%s' fail \r\n", acFilePath);
            return -4;
        }
        else //open file success
        {
            file_size = p2p_get_size_file(pvFile);
            fclose(pvFile);
            pvFile = NULL;

            if (file_size <= 0)
            {
                ak_print_error_ex("File error: '%s', file size: %ld bytes \r\n", acFilePath, file_size);
                return -4;
            }
            else
            {   
                ak_print_error_ex("The valid file: '%s', file size: %ld bytes \r\n", acFilePath, file_size);
            }
        }
        //--------------------------------------------------------------end check file size----------------------------------------------------------------

    }

    printf("url_path: %s\r\n",url_path);
    /* Send GET method */
    ret = make_request_event_upload_snap(sockfd, l_url, url_path,atoi(port));
    if(ret >= 0){

        //ak_print_error_ex("\n -------------------- Begin http_send_event_upload_success_snapshot - fetch_response ------------------------- \r\n");
        status = fetch_response(sockfd, &response, 10*BUF_SIZE -1);
        printf("status %d response %s\r\n",status,response->contents);
        error_unless(status >= 0, "Fetching response failed");

        if(send_event_success_parse_repsonse(response->contents) != 0){
            ret = -1;
        }
        else{
            ret = 0;
        }
    }

    close_api(sockfd);
    freeaddrinfo(res);

    buffer_free(response);
    //url_free(url);
    return ret;
    
error:
    if (sockfd > 0)  { close_api(sockfd); }
    if (res != NULL) { freeaddrinfo(res); }
    //if (url) { url_free(url); }
    buffer_free(response);
    return 1;
}

/*---------------------------------------------------------------------------*/
/* New flow upload for demo */
/*---------------------------------------------------------------------------*/
/* API add event and get event id */
extern char g_event_info[2048];
#ifdef FIRST_EVENT_REST
int http_post_add_event_api(void)
{
	unsigned long tick = 0;
    unsigned long tick_upload_snapshot = 0;
    tick = get_tick_count();
    tick_upload_snapshot = tick;
    int flagg = 1;
    void * checkfile; 

    char amz[256]={0}, amz_clip[256]={0};
    Buffer *response = buffer_alloc(10*BUF_SIZE);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    struct addrinfo *res = NULL;
#endif

    Url *url_amz,*url_amz_clip;
    int status = 0;
    int sockfd = 0;
    char *json_response = NULL;
    char *data_upload;
    int ret = 0;
    int i;

    url_parser parsed_url;

    //*************Get info system*********************//

    ret = get_system_info();
    if(ret != 0)
    {
        printf("Uploader get info system failed! (%d)\r\n", ret);
        return -1;
    }

    //*************GET S3 Upload LINK*********************//
    // DNS resolve
#ifndef UPLOADER_RUN_ON_AK_PLAT
    status = init_connection(API_URL, API_PORT, &res);
    if ((status!=0) || (res==NULL)){
	    if (res!=NULL)
	        freeaddrinfo(res);
    	printf("ERROR cannot init connect\r\n");
	}

    // Connect to IP
    sockfd = make_connection(res);
#else

    #if(UPLOADER_USE_SSL == 1)
    status = init_connection(API_URL, API_PORT, 1);
    #else
    status = init_connection(API_URL, API_PORT, 0);
    #endif

    sockfd = status;
#endif

    if (sockfd < 0){
        printf("%s ERROR: make connection failed sockfd %d status %d\r\n", \
                __func__, sockfd, status);
    	return -1;
	}

    // POST to server
    status = make_request_event_add(sockfd, API_URL);
    if (status <= 0){
	    close_api(sockfd);
    	printf("Sending request failed");
    	return -1;
	}
        
    // Get response
    status = fetch_response(sockfd, &response, RECV_SIZE);
    close_api(sockfd);

#ifndef UPLOADER_RUN_ON_AK_PLAT
    freeaddrinfo(res);
#endif
    if (status < 0){
	    buffer_free(response);
    	printf("Fetching response failed");
	}


    printf("\r\nPOST Motion Event response length: %d\r\n", response->bytes_used);
    printf("Post Motion Event response: ===============\r\n%s\r\n", response->contents);
    printf("Post Motion Event response: End\r\n\r\n", response->contents);
    
    // Parse response
    json_response = strstr(response->contents, "{");
    parseJson_customized(json_response,  "snapshot_url", amz, sizeof(amz));
    parseJson_customized(json_response,  "clip_url", amz_clip, sizeof(amz_clip));
    printf("\r\nsnapshot_url %s\r\n",amz);
    printf("\r\nclip_url %s\r\n",amz_clip);
    parseJson_customized(json_response,  "success_action_redirect", success_action_redirect, sizeof(success_action_redirect));
    ret = parse_url(success_action_redirect, &parsed_url);
    ret |= getValueFromKey(&parsed_url, "event_id", g_eventid, sizeof(g_eventid));
    ret |= getValueFromKey(&parsed_url, "update_date", g_date, sizeof(g_date));
    if (ret != 0) {
        printf("parse_url fails\r\n");
        return -1;
    }

	/* free buffer after finish */
	buffer_free(response);

	if(strlen(g_eventid) == 0)
	{
		printf("%s cannot get event id!\r\n", __func__);
		return -1;
	}
	
	if(strlen(g_date) == 0)
	{
		printf("%s cannot get update_date!\r\n", __func__);
		return -1;
	}
	
	printf("event_id: %s, update_date: %s\r\n", g_eventid, g_date);

	return 0;	
}
#else
int http_post_add_event_api(void)
{
	if (strlen(g_event_info)>0){
	    parseJson_customized(g_event_info,  "ev_id", g_eventid, sizeof(g_eventid));
	    parseJson_customized(g_event_info,  "ev_date", g_date, sizeof(g_date));
	
		if(strlen(g_eventid) == 0)
		{
			printf("%s cannot get event id!\r\n", __func__);
			return -1;
		}
		
		if(strlen(g_date) == 0)
		{
			printf("%s cannot get update_date!\r\n", __func__);
			return -1;
		}
		
		printf("event_id: %s, update_date: %s\r\n", g_eventid, g_date);
	
		return 0;
	} else {
		printf("Event info NULL\r\n");
		return -1;
	}
}
#endif
/*---------------------------------------------------------------------------*/
/* */
/*---------------------------------------------------------------------------*/

