
// #include <sys/types.h> // on linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "upload_main.h"
#include "ak_video.h"
#include "http.h"
#include "file_on_mem.h"
#include "ak_apps_config.h"
#include "config_model.h"

#ifdef SERVER_CEPHS3
char g_UDID[64]="000002F4B85ECD1AB0IAPACN";
char g_devicetoken[128]="e59c901fe895e0f80bf624239f84dc4a";
char g_api_url[MAX_URL_LEN+1]="api.db1-staging.l-kloud.com";
// char CON_MAC_ADDRESS[]="F4B85ECD1AB0";
char g_macaddress[13]="20914894336F";
#endif

#ifdef SERVER_S3
char g_UDID[64]="000002F4B85ECD1AB0IAPACN";
char g_devicetoken[128]="e8b61e8966dda3f2d08e0f9f4ab70079";
char g_api_url[MAX_URL_LEN+1]="api.cinatic.com";
// char CON_MAC_ADDRESS[]="F4B85ECD1AB0";
char g_macaddress[13]="F4B85ECD1AB0";
#endif

// S3_ALL_IN_ONE
#ifdef SERVER_HTTP
char g_UDID[64]="000002209148943C96VNANXK";
char g_devicetoken[128]="e59c901fe895e0f80bf624239f84dc4a";
char g_api_url[MAX_URL_LEN+1]="api.db1-staging.l-kloud.com";
// char CON_MAC_ADDRESS[]="209148943C96";
char g_macaddress[13]="209148943C96";

char g_acFileNameAllInOne[MAX_FILENAME_LEN+1];
extern DOORBELL_SNAPSHOT_DATA_t g_sDataSnapshot4Uploader;

#endif

char g_key_hex[65]={0};
char g_rnd_hex[65]={0};
char g_sip[16]={0};
char g_sp[8]={0};

#if(UPLOADER_USE_SSL == 0)
char g_api_port[16]="80";
#else
char g_api_port[16]="443";
#endif

char g_local_jpg_file[MAX_FILENAME_LEN+1]="s3test";
//char g_local_flv_file[MAX_FILENAME_LEN+1]="321";
char g_local_flv_file[MAX_FILENAME_LEN+1]="";

// S3_ALL_IN_ONE
#define EVENT_STR "{\"device_id\":\"%s\",\"event_type\":1,\"st_data\":{\"key\":\"%s\",\"rn\":\"%s\",\"sip\":\"%s\",\"sp\":%d},\"device_token\":\"%s\",\"is_ssl\":false}"
#define STREAM_KEY "mykey"
#define STREAM_RND "myrnd"
#define STREAM_IP "1.1.1.1"
#define STREAM_PORT 9094

int filesize_snapshot=0;


extern int g_iDoneSnapshot;
extern int g_isDoneUpload;

extern char g_eventid[];
extern char g_date[];

extern int g_play_dingdong;
static int g_first_time_upload = 0;
extern char g_event_info[];

// extern TIME_DEBUG_UPLOAD_t g_sTimeDebugUpload;

// S3_ALL_IN_ONE
#ifdef SERVER_HTTP
extern char g_acFileNameSnap[256];

#if 0
#define http_file_multipart_0 "POST /v1/devices/events/upload/snap?device_id=%s&event_type=6&device_token=%s&file_type=1&file_name=%s&file_size=%d&st_key=%s&st_ip=%s&st_port=%d&st_rn=%s HTTP/1.1\r\n"\
                  "HOST: api.db1-staging.l-kloud.com\r\n"\
                  "Connection: keep-alive\r\n"\
                  "Content-Length: %d\r\n"\
                  "User-Agent: CCAK\r\n"\
                  "Cache-Control: no-cache\r\n"\
                  "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryRBARCG8356kyJ5aA\r\n"\
                  "Accept: */*\r\n"\
                  "Accept-Encoding: gzip, deflate\r\n"\
                  "Accept-Language: en-US,en;q=0.8\r\n"\
                  "\r\n%s"
#else
#define http_file_multipart_0 "POST /v1/devices/events/upload/snap?device_id=%s&event_type=6&device_token=%s&file_type=1&file_name=%s&file_size=%d&st_key=%s&st_ip=%s&st_port=%s&st_rn=%s HTTP/1.1\r\n"\
                  "HOST: %s\r\n"\
                  "Connection: keep-alive\r\n"\
                  "Content-Length: %d\r\n"\
                  "User-Agent: CCAK\r\n"\
                  "Cache-Control: no-cache\r\n"\
                  "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryRBARCG8356kyJ5aA\r\n"\
                  "Accept: */*\r\n"\
                  "Accept-Encoding: gzip, deflate\r\n"\
                  "Accept-Language: en-US,en;q=0.8\r\n"\
                  "\r\n%s"

#define http_file_multipart_new_flow "POST /v1/devices/events/upload/complete?device_id=%s&event_id=%s&update_date=%s&device_token=%s&file_type=%d&file_name=%s&file_size=%d HTTP/1.1\r\n"\
                  "HOST: %s\r\n"\
                  "Connection: keep-alive\r\n"\
                  "Content-Length: %d\r\n"\
                  "User-Agent: CCAK\r\n"\
                  "Cache-Control: no-cache\r\n"\
                  "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryRBARCG8356kyJ5aA\r\n"\
                  "Accept: */*\r\n"\
                  "Accept-Encoding: gzip, deflate\r\n"\
                  "Accept-Language: en-US,en;q=0.8\r\n"\
                  "\r\n%s"


#endif



#define http_file_multipart_1_new_flow "------WebKitFormBoundaryRBARCG8356kyJ5aA\r\n"\
                  "Content-Disposition: form-data; name=\"mp_file\"; filename=\"%s\"\r\n"\
                  "Content-Type: image/jpeg\r\n"\
                  "\r\n"
#define http_file_multipart_1_new_flow_flv "------WebKitFormBoundaryRBARCG8356kyJ5aA\r\n"\
                  "Content-Disposition: form-data; name=\"mp_file\"; filename=\"%s\"\r\n"\
                  "Content-Type: video/x-flv\r\n"\
                  "\r\n"
#define http_file_multipart_1 "------WebKitFormBoundaryRBARCG8356kyJ5aA\r\n"\
                  "Content-Disposition: form-data; name=\"snap\"; filename=\"%s\"\r\n"\
                  "Content-Type: image/jpeg\r\n"\
                  "\r\n"

                  
#define http_file_multipart_2 "\r\n------WebKitFormBoundaryRBARCG8356kyJ5aA--\r\n"

//Post json header
#define http_file_sd_complete_header "POST /v1/devices/events/sd_complete?device_id=%s&event_id=%s&update_date=%s&device_token=%s&file_type=%d&file_name=%s.flv&file_size=%d HTTP/1.1\r\n"\
                  "HOST: %s\r\n"\
                  "Connection: keep-alive\r\n"\
                  "Content-Length: 0\r\n"\
                  "User-Agent: CCAK\r\n"\
                  "Cache-Control: no-cache\r\n"\
                  "Content-Type: application/x-www-form-urlencoded\r\n"\
                  "Accept: */*\r\n"\
                  "Accept-Encoding: gzip, deflate\r\n"\
                  "Accept-Language: en-US,en;q=0.8\r\n"\
                  "\r\n"




int main_all_in_one(void){
	// char str_http_header0[1024];
	// char str_http_header1[1024];
	char *str_http_header0 = NULL;
	char *str_http_header1 = NULL;
	char *str_http_header2=http_file_multipart_2;
	int ret;
	char fullfilename[128];
	struct addrinfo *res = NULL;
    int status = -1;
    int sockfd = -1;
    int send_len=-1;
    int i,block_len;
    char l_data[1024];
	// FILE *fp = NULL;
	int content_len = 0;
	unsigned long ulTime = get_tick_count();
	
	ak_print_error_ex("==========Start all in one OK: %u===========\r\n", ulTime);
	str_http_header0 = (char *)malloc(1024);
	str_http_header1 = (char *)malloc(1024);
	if(!str_http_header0 || !str_http_header1){
		ak_print_error_ex("Malloc failed!\r\n");
		return -1;
	}
	memset(str_http_header0, 0x00, 1024);
	memset(str_http_header1, 0x00, 1024);

	// S3_ALL_IN_ONE
	ret = get_system_info();
    if(ret != 0)
    {
        ak_print_error_ex("Uploader get info system failed! (%d)\r\n", ret);
        return -1;
	}

	memset(fullfilename, 0x00, sizeof(fullfilename));
	// snprintf(fullfilename,sizeof(fullfilename),"%s.%s",g_local_jpg_file,"jpg");
	if (gen_motion_filename(g_acFileNameAllInOne, MAX_FILENAME_LEN) == 0)
	{
		snprintf(fullfilename, sizeof(fullfilename), "%s.%s", g_acFileNameAllInOne, "jpg");
		// ak_print_normal("%s filename: %s\r\n", __func__, fullfilename);
	}
	else{
		ak_print_error_ex("Length of g_acFileNameSnap is too long!\r\n");
		return -3;
	}

	filesize_snapshot = fsize_api("S3_ALL_IN_ONE");
	// snprintf(str_http_header1, sizeof(str_http_header1), http_file_multipart_1, fullfilename);
	snprintf(str_http_header1, 1024, http_file_multipart_1, fullfilename);
	// content_len = strlen(http_file_multipart_1)+filesize_snapshot+strlen(http_file_multipart_2);
	content_len = strlen(str_http_header1) + filesize_snapshot + strlen(str_http_header2);

	ak_print_error_ex("Snapshot file:%s, filelen=%d\r\n, contentlen=%d\r\n", fullfilename, filesize_snapshot,content_len);
	
	ak_print_error_ex("sizeof(str_http_header0): %d\r\n", sizeof(str_http_header0));
	// snprintf(str_http_header0,sizeof(str_http_header0),http_file_multipart_0,g_UDID,g_devicetoken,fullfilename,filesize_snapshot,STREAM_KEY,STREAM_IP,STREAM_PORT,STREAM_RND,content_len,str_http_header1);
	// snprintf(str_http_header0, 1024,http_file_multipart_0,g_UDID,g_devicetoken,fullfilename,filesize_snapshot,STREAM_KEY,STREAM_IP,STREAM_PORT,STREAM_RND,content_len,str_http_header1);
	snprintf(str_http_header0, 1024,http_file_multipart_0,g_UDID,g_devicetoken,fullfilename,filesize_snapshot, g_key_hex, g_sip, g_sp, g_rnd_hex, g_api_url, content_len,str_http_header1);
	
	/* debug */
	printf("str_http_header0: %s\r\n", str_http_header0);
	printf("g_api_url: %s\r\n", API_URL);
	printf("g_api_port: %s\r\n", API_PORT);

	//*************GET S3 Upload LINK*********************//
	// S3_ALL_IN_ONE
	#ifndef UPLOADER_RUN_ON_AK_PLAT
		// DNS resolve
		status = init_connection(g_api_url, g_api_port, &res);
		if ((status!=0) || (res==NULL)){
			if (res!=NULL)
				freeaddrinfo(res);
			printf("ERROR cannot init connect\r\n");
		}

		// Connect to IP
		sockfd = make_connection(res);
		if (sockfd < 0){
			printf("ERROR: make connection failed\r\n");
			return -1;
		}
	#else

		#if(UPLOADER_USE_SSL == 1)
			status = init_connection(API_URL, API_PORT, 1);
		#else
			status = init_connection(API_URL, API_PORT, 0);
		#endif

		sockfd = status;
		if (sockfd < 0){
			ak_print_error_ex("ERROR: make connection failed sockfd %d status %d\r\n", \
					sockfd, status);
			return -1;
		}
		ak_print_error_ex("==========open socket OK: %u===========\r\n", get_tick_count() - ulTime);
	#endif

	send_len = strlen(str_http_header0);
	ak_print_error_ex("Send %d bytes header:\r\n",send_len);
	ak_print_error_ex("str_http_header0: %s\r\n", str_http_header0);

	i=0;
	while(i<send_len){
		if ((send_len-i)>=HTTP_CHUNK_SIZE_NKB)
			block_len=HTTP_CHUNK_SIZE_NKB;
		else
			block_len=send_len-i;
		ret = send_api(sockfd, str_http_header0 + i, block_len, 0);
		if (ret<block_len){
			close_api(sockfd);
			ak_print_error_ex("ERROR: send fail\r\n");
			return 0;
		}
		i+=block_len;
	}
	ak_print_error_ex("Send str_http_header0 OK!\r\n");

	send_len=filesize_snapshot;
	i=0;
	while(i<send_len){
		if ((send_len-i)>=HTTP_CHUNK_SIZE_NKB)
			block_len=HTTP_CHUNK_SIZE_NKB;
		else
			block_len=send_len-i;
	
		// memcpy(l_data, g_sDataSnapshot4Uploader.data + i, block_len);
		// ret = send_api(sockfd, l_data, block_len, 0);
		ret = send_api(sockfd, g_sDataSnapshot4Uploader.data + i, block_len, 0);
		printf("Send%d\r\n",ret);
		if (ret!=block_len){
			
			close_api(sockfd);
			ak_print_error_ex("ERROR: send fail\r\n");
			return 0;
		}
		i+=block_len;
	}

	send_len=strlen(str_http_header2);
	i=0;
	ak_print_error_ex("str_http_header2: %s\r\n",str_http_header2);
	while(i<send_len){
		if ((send_len-i)>=HTTP_CHUNK_SIZE_NKB)
			block_len=HTTP_CHUNK_SIZE_NKB;
		else
			block_len=send_len-i;
		ret = send_api(sockfd, str_http_header2+i, block_len, 0);
		if (ret!=block_len){
			close_api(sockfd);
			ak_print_error_ex("ERROR: send fail\r\n");
			return 0;
		}
		i+=block_len;
	}

	/* send 0 byte data to change to recv data */
	ret = send_api(sockfd, str_http_header2+i, 0, 0);
	ak_print_error_ex("==========send done: %u (ret %d)===========\r\n", get_tick_count() - ulTime, ret);

	i=0;
	memset(l_data,0x00,sizeof(l_data));
	ret=-1;
	while(i<30){
		ret = recv_api(sockfd, l_data, 1000, 0 );
		if (ret>0){
			ak_print_error_ex("Recv data %d break", ret);
			break;
		}
		i++;
		port_sleep_ms(100);
	}
	close_api(sockfd);
	if (ret>0)
		printf("Response:%s\r\n",l_data);
	else
		printf("Fail to receive response\r\n");

	ak_print_error_ex("==========recv done %u ===========\r\n", get_tick_count() - ulTime, ret);
	return 0;
}


/*---------------------------------------------------------------------------*/
/* New flow upload for demo Motorola 23-Nov-2017 */
/*---------------------------------------------------------------------------*/
/* I clone code and add one more parameter for url upload, "EVENT_ID" */
extern int clip_filesize;
extern char g_acClipFileName[];
int upload_all_in_one_add_eventid(char* filetype)
{
	char *str_http_header0 = NULL;
	char *str_http_header1 = NULL;
	char *str_http_header2 = http_file_multipart_2;
	int ret;
	char fullfilename[128];
	struct addrinfo *res = NULL;
    int status = -1;
    int sockfd = -1;
    int send_len=-1;
    int i,block_len;
    char l_data[1024];
	int content_len = 0;
	unsigned long ulTime = get_tick_count();
	FILE *fp=NULL;
	int upload_body = 1;
//	char *c_video=NULL;
	char l_video_buf[HTTP_CHUNK_SIZE_NKB];
	
	ak_print_error_ex("==========Start all in one OK: %u===========\r\n", ulTime);
	str_http_header0 = (char *)malloc(1024);
	str_http_header1 = (char *)malloc(1024);
	if(!str_http_header0 || !str_http_header1){
		ak_print_error_ex("Malloc failed!\r\n");
		return -1;
	}
	memset(str_http_header0, 0x00, 1024);
	memset(str_http_header1, 0x00, 1024);

	// S3_ALL_IN_ONE
	ret = get_system_info();
    if(ret != 0)
    {
        ak_print_error_ex("Uploader get info system failed! (%d)\r\n", ret);
        return -1;
	}

	memset(fullfilename, 0x00, sizeof(fullfilename));
	// snprintf(fullfilename,sizeof(fullfilename),"%s.%s",g_local_jpg_file,"jpg");
	if (gen_motion_filename(g_acFileNameAllInOne, MAX_FILENAME_LEN) == 0)
	{
		snprintf(fullfilename, sizeof(fullfilename), "%s.%s", g_acFileNameAllInOne, filetype);
	}
	else{
		ak_print_error_ex("Length of g_acFileNameSnap is too long!\r\n");
		return -3;
	}
	if (filetype[0]=='j')
		filesize_snapshot = fsize_api("S3_ALL_IN_ONE"); //Snapshot
	else{
#ifdef DOORBELL_TINKLE820
		filesize_snapshot = fsize_api(NULL); //clip
		ak_print_error_ex("Clip length %d\r\n",filesize_snapshot);
		fp = f_mem_open("", "r");
		if (fp==NULL) {
			ak_print_error_ex("Fail to get clip binary\r\n");
			return -1;
		}
#else
		// LaoHu doorbell update clipname only to server
		filesize_snapshot = clip_filesize;
		ak_print_error_ex("Clip length %d\r\n",filesize_snapshot);
		upload_body = 0;
#endif
	}
		
	//snprintf(str_http_header1, 1024, http_file_multipart_1, fullfilename);
	if (filetype[0]=='j')
		snprintf(str_http_header1, 1024, http_file_multipart_1_new_flow, fullfilename);
#ifdef DOORBELL_TINKLE820
	else
		snprintf(str_http_header1, 1024, http_file_multipart_1_new_flow_flv, fullfilename);
#endif
		
	content_len = strlen(str_http_header1) + filesize_snapshot + strlen(str_http_header2);
	ak_print_error_ex("Snapshot file:%s, filelen=%d\r\n, contentlen=%d\r\n", fullfilename, filesize_snapshot,content_len);
	ak_print_error_ex("sizeof(str_http_header0): %d\r\n", sizeof(str_http_header0));
	//snprintf(str_http_header0, 1024, http_file_multipart_new_flow, g_UDID, g_devicetoken,fullfilename,filesize_snapshot, g_key_hex, g_sip, g_sp, g_rnd_hex, g_api_url, content_len,str_http_header1);
	if (filetype[0]=='j')
		snprintf(str_http_header0, 1024, http_file_multipart_new_flow, g_UDID, g_eventid, g_date, g_devicetoken,1,fullfilename,filesize_snapshot, g_api_url, content_len,str_http_header1);
	else
#ifdef DOORBELL_TINKLE820
		snprintf(str_http_header0, 1024, http_file_multipart_new_flow, g_UDID, g_eventid, g_date, g_devicetoken,2,fullfilename,filesize_snapshot, g_api_url, content_len,str_http_header1);
#else
		snprintf(str_http_header0, 1024, http_file_sd_complete_header, g_UDID, g_eventid, g_date, g_devicetoken,2,g_acClipFileName,filesize_snapshot, g_api_url);
#endif
	/* debug */
	printf("str_http_header0: %s\r\n", str_http_header0);
	printf("g_api_url: %s\r\n", API_URL);
	printf("g_api_port: %s\r\n", API_PORT);

	//*************GET S3 Upload LINK*********************//
	// S3_ALL_IN_ONE
	#ifndef UPLOADER_RUN_ON_AK_PLAT
		// DNS resolve
		status = init_connection(g_api_url, g_api_port, &res);
		if ((status!=0) || (res==NULL)){
			if (res!=NULL)
				freeaddrinfo(res);
			printf("ERROR cannot init connect\r\n");
		}

		// Connect to IP
		sockfd = make_connection(res);
		if (sockfd < 0){
			printf("ERROR: make connection failed\r\n");
			return -1;
		}
	#else

		#if(UPLOADER_USE_SSL == 1)
			status = init_connection(API_URL, API_PORT, 1);
		#else
			status = init_connection(API_URL, API_PORT, 0);
		#endif

		sockfd = status;
		if (sockfd < 0){
			ak_print_error_ex("ERROR: make connection failed sockfd %d status %d\r\n", \
					sockfd, status);
			return -1;
		}
		ak_print_error_ex("==========open socket OK: %u===========\r\n", get_tick_count() - ulTime);
	#endif

	send_len = strlen(str_http_header0);
	ak_print_error_ex("Send %d bytes header:\r\n",send_len);
	ak_print_error_ex("str_http_header0: %s\r\n", str_http_header0);

	i=0;
	while(i<send_len){
		if ((send_len-i)>=HTTP_CHUNK_SIZE_NKB)
			block_len=HTTP_CHUNK_SIZE_NKB;
		else
			block_len=send_len-i;
		ret = send_api(sockfd, str_http_header0 + i, block_len, 0);
		if (ret<block_len){
			close_api(sockfd);
			ak_print_error_ex("ERROR: send fail\r\n");
			return 0;
		}
		i+=block_len;
	}
	ak_print_error_ex("Send str_http_header0 OK!\r\n");

	send_len=filesize_snapshot;
	i=0;

	if (upload_body){
		block_len=HTTP_CHUNK_SIZE_NKB;
		while((i<send_len)&&(block_len==HTTP_CHUNK_SIZE_NKB)){
			if (filetype[0]=='j'){
				if ((send_len-i)>=HTTP_CHUNK_SIZE_NKB)
					block_len=HTTP_CHUNK_SIZE_NKB;
				else
					block_len=send_len-i;
			} else
				block_len = f_mem_read(l_video_buf, 1, HTTP_CHUNK_SIZE_NKB, fp);
		
			// memcpy(l_data, g_sDataSnapshot4Uploader.data + i, block_len);
			// ret = send_api(sockfd, l_data, block_len, 0);
			if (filetype[0]=='j')
				ret = send_api(sockfd, g_sDataSnapshot4Uploader.data + i, block_len, 0);
			else{
				ret = send_api(sockfd, l_video_buf, block_len, 0);
				//ret = send_api(sockfd, c_video + i, block_len, 0);
			}
			printf("Send%d\r\n",ret);
			if (ret!=block_len){
				
				close_api(sockfd);
				ak_print_error_ex("ERROR: send fail\r\n");
				return 0;
			}
			i+=block_len;
		}
		f_mem_close(fp); // close video clip file
		send_len=strlen(str_http_header2);
		i=0;
		ak_print_error_ex("str_http_header2: %s\r\n",str_http_header2);
		while(i<send_len){
			if ((send_len-i)>=HTTP_CHUNK_SIZE_NKB)
				block_len=HTTP_CHUNK_SIZE_NKB;
			else
				block_len=send_len-i;
			ret = send_api(sockfd, str_http_header2+i, block_len, 0);
			if (ret!=block_len){
				close_api(sockfd);
				ak_print_error_ex("ERROR: send fail\r\n");
				return 0;
			}
			i+=block_len;
		}
	}

	/* send 0 byte data to change to recv data */
	ret = send_api(sockfd, str_http_header2+i, 0, 0);
	ak_print_error_ex("==========send done: %u (ret %d)===========\r\n", get_tick_count() - ulTime, ret);

	i=0;
	memset(l_data,0x00,sizeof(l_data));
	ret=-1;
	while(i<30){
		ret = recv_api(sockfd, l_data, 1000, 0 );
		if (ret>0){
			ak_print_error_ex("Recv data %d break", ret);
			break;
		}
		i++;
		port_sleep_ms(100);
	}
	close_api(sockfd);
	if (ret>0)
		printf("Response:%s\r\n",l_data);
	else
		printf("Fail to receive response\r\n");

	ak_print_error_ex("==========recv done %u ===========\r\n", get_tick_count() - ulTime, ret);
	return 0;
}


/*
main function for new flow upload
*/
int main_upload_new_flow_demo(void)
{
	int iRet = 0;
	unsigned long ulTickCount = 0;
	int clip_wait_count=0;
	/* Get tick count to break down timing new flow */
	ulTickCount = get_tick_count();
	ak_print_error_ex("**************start new flow demo at: %u ms**************\r\n", ulTickCount);

	/* Add event and get parameter from server */
	iRet = http_post_add_event_api();

	ulTickCount = get_tick_count();
	//g_sTimeDebugUpload.add_event_done = ulTickCount;
	ak_print_error_ex("**************finish add event at: %u ms**************\r\n", ulTickCount);
	if(iRet != 0) // get event if from here and put it to new flow upload
	{
		ak_print_error_ex("Http post add event to server failed! Cannot get event id\r\n");
	}
	else
	{
		/* Upload snapshot with event id and update date */
		iRet = upload_all_in_one_add_eventid("jpg");

		ulTickCount = get_tick_count();
		//g_sTimeDebugUpload.upload_snapshot_done = ulTickCount;
		ak_print_error_ex("**************finish upload snapshot at: %u ms size: %d bytes (%d)**************\r\n", ulTickCount, fsize_api("S3_ALL_IN_ONE"), iRet);
		/*
		ak_print_error_ex("i: %u, snap: %u, us: %u, add: %u, ud: %u, t2add: %u, t2up: %u, total2up: %u\r\n", \
				g_sTimeDebugUpload.first_i_frame, g_sTimeDebugUpload.take_snapshot, g_sTimeDebugUpload.start_uploader, \
				g_sTimeDebugUpload.add_event_done, g_sTimeDebugUpload.upload_snapshot_done, \
				g_sTimeDebugUpload.add_event_done - g_sTimeDebugUpload.start_uploader, \
				g_sTimeDebugUpload.upload_snapshot_done - g_sTimeDebugUpload.add_event_done, \
				g_sTimeDebugUpload.upload_snapshot_done - g_sTimeDebugUpload.start_uploader);
				*/
		if(iRet != 0)
		{
			ak_print_error_ex("upload snapshot all in one failed!\r\n");
		}
		else
		{
			ak_print_error_ex("upload snapshot all in one successful!\r\n");
		}
		
		ak_print_error_ex("Wait for video clip to be done!\r\n");
		// Wait max 20s to record clip to complete
		while ((check_status_record() != RECORD_STATE_DONE) && (clip_wait_count<(20*1000/500)))
		{
			clip_wait_count++;
			port_sleep_ms(500);
		}
		if (check_status_record() != RECORD_STATE_DONE){
			ak_print_error_ex("Clip timeout!\r\n");
			return -10;
		} else
			ak_print_error_ex("Clip ready!\r\n");
		iRet = upload_all_in_one_add_eventid("flv");
		g_event_info[0]=0x00;
#if (SDCARD_ENABLE==0)
		f_mem_erase(NULL);
#endif
		if (iRet>=0)
			ak_print_error_ex("Upload clip done succesfully!\r\n");
		else
			ak_print_error_ex("Upload clip fail!\r\n");
	}

	return iRet;
}

/*---------------------------------------------------------------------------*/
/* End of new flow upload                                                    */
/*---------------------------------------------------------------------------*/

#endif // #ifdef SERVER_HTTP

extern char g_event_info[];
extern int g_iPosition;
void *uploader_task_new_push(void *argv)
{
	int i = 0;
	int iRet = 0;
	unsigned long ulTime = 0;
    ENUM_SYS_MODE_t sys_get_mode;

	unsigned long ulTickCount = 0;
	int clip_wait_count=0;
	
	for(;;)
	{
		if(g_first_time_upload == 0) // first time upload
		{
			/* Get tick count to break down timing new flow */
			ulTime = get_tick_count();
			// push event
			/*while(g_play_dingdong != 1)
			{
				ak_sleep_ms(10);
			}*/
			printf("uploader_task_new_push 1 %d\r\n",ulTime);
			while(strlen(g_event_info)==0)
			{
				ak_sleep_ms(50);
			}
			ulTime = get_tick_count();
			printf("uploader_task_new_push 2 %d\r\n",ulTime);
			g_iPosition=0;
			// ak_print_error_ex("**************start new flow demo at: %u ms**************\r\n", ulTickCount);
			/* Add event and get parameter from server */
			iRet = http_post_add_event_api();
			
			ulTime = get_tick_count();
			printf("uploader_task_new_push 3 %d\r\n",ulTime);
			// upload
			if(iRet != 0) // get event if from here and put it to new flow upload
			{
				ak_print_error_ex("1Http post add event to server failed! Cannot get event id\r\n");
			}
			else
			{
				// while(g_iDoneSnapshot == 0)
				ulTime = get_tick_count();
				printf("uploader_task_new_push 3.1 %d\r\n",ulTime);
				while(g_iDoneSnapshot != SNAPSHOT_STATE_DONE)
				{
					i++;
					if(i%10 == 0){
						ulTime = get_tick_count();
						printf("=1=%d\r\n",ulTime);
						i = 0;
					}
					ak_sleep_ms(50);
					sys_get_mode = eSysModeGet();
					if(E_SYS_MODE_FTEST == sys_get_mode || \
						E_SYS_MODE_FWUPGRADE == sys_get_mode)
					{
						ak_print_normal_ex("Exit1 loop wait snapshot flag!\r\n");
						break;
					}
				}

				ulTime = get_tick_count();
				printf("uploader_task_new_push 4 %d\r\n",ulTime);
			
				if(E_SYS_MODE_FTEST == sys_get_mode || \
								E_SYS_MODE_FWUPGRADE == sys_get_mode)
				{
					ak_print_normal_ex("Exit2 loop upload task\r\n");
					break;
				}
				g_isDoneUpload = UPLOADER_STATE_RUNNING;
				g_iDoneSnapshot = SNAPSHOT_STATE_WAIT; // avoid upload loop forever
	
				ulTime = get_tick_count();
				// g_sTimeDebugUpload.start_uploader = ulTime;
				printf("===================================================\r\n");
				printf("Begin first uploader new flow for demo! Set g_iDoneSnapshot = %d at %d\r\n", g_iDoneSnapshot, ulTime);
				printf("===================================================\r\n");

				/* Upload snapshot with event id and update date */
				iRet = upload_all_in_one_add_eventid("jpg");

				ulTickCount = get_tick_count();
				//g_sTimeDebugUpload.upload_snapshot_done = ulTickCount;
				ak_print_error_ex("**************1finish upload snapshot at: %u ms size: %d bytes (%d)**************\r\n", ulTickCount - ulTime, fsize_api("S3_ALL_IN_ONE"), iRet);
				if(iRet != 0)
				{
					ak_print_error_ex("1upload snapshot all in one failed!\r\n");
				}
				else
				{
					ak_print_error_ex("1upload snapshot all in one successful!\r\n");
				}
				
				printf("1file_post: iRet %d time to upload: %u ms\r\n", iRet, get_tick_count() - ulTime);
				
				ak_print_error_ex("Wait for video clip to be done!\r\n");
				// Wait max 20s to record clip to complete
				while ((check_status_record() != RECORD_STATE_DONE) && (clip_wait_count<(20*1000/500)))
				{
					clip_wait_count++;
					port_sleep_ms(500);
				}
				if (check_status_record() != RECORD_STATE_DONE){
					ak_print_error_ex("Clip timeout!\r\n");
					return -10;
				} else
					ak_print_error_ex("Clip ready!\r\n");
				iRet = upload_all_in_one_add_eventid("flv");
				g_event_info[0]=0x00;
#if (SDCARD_ENABLE==0)
				f_mem_erase(NULL);
#endif
				if (iRet>=0)
					ak_print_error_ex("Upload clip done succesfully!\r\n");
				else
					ak_print_error_ex("Upload clip fail!\r\n");

				g_isDoneUpload = UPLOADER_STATE_DONE;
				printf("1UPLOADER_STATE_DONE: time (%u)\r\n", get_tick_count());
				printf("1***********************************************************\r\n");
				printf("1***********************************************************\r\n");
				printf("\r\n\r\n\r\n");
			}

			// for first time we will start push first
			g_first_time_upload = 1;
			
		}
		else // after system init
		{
			printf("222...g_iDoneSnapshot = %d\r\n", g_iDoneSnapshot);
			// while(g_iDoneSnapshot == 0)
			while(g_iDoneSnapshot != SNAPSHOT_STATE_DONE)
			{
				i++;
				if(i%5 == 0){
					printf("=2=");
					i = 0;
				}
				ak_sleep_ms(100);
				sys_get_mode = eSysModeGet();
				if(E_SYS_MODE_FTEST == sys_get_mode || \
					E_SYS_MODE_FWUPGRADE == sys_get_mode)
				{
					ak_print_normal_ex("Exit1 loop wait snapshot flag!\r\n");
					break;
				}
			}
			if(E_SYS_MODE_FTEST == sys_get_mode || \
				E_SYS_MODE_FWUPGRADE == sys_get_mode)
			{
				ak_print_normal_ex("Exit2 loop upload task\r\n");
				break;
			}
			g_isDoneUpload = UPLOADER_STATE_RUNNING;
			g_iDoneSnapshot = SNAPSHOT_STATE_WAIT; // avoid upload loop forever

			ulTime = get_tick_count();
			// g_sTimeDebugUpload.start_uploader = ulTime;
			printf("===================================================\r\n");
			printf("Begin start uploader new flow for demo! Set g_iDoneSnapshot = %d\r\n", g_iDoneSnapshot);
			printf("===================================================\r\n");

			// g_isDoneUpload = 1;
			
			// iRet = file_post();

			//main_all_in_one();
			main_upload_new_flow_demo();

			printf("file_post: iRet %d time to upload: %u ms\r\n", iRet, get_tick_count() - ulTime);
			g_isDoneUpload = UPLOADER_STATE_DONE;
			printf("UPLOADER_STATE_DONE: time (%u)\r\n", get_tick_count());
			printf("***********************************************************\r\n");
			printf("***********************************************************\r\n");
			printf("\r\n\r\n\r\n");
			// why do we assign this varibale at here! This variable belong snapshot task!
			// g_iDoneSnapshot = 0;
		}
	}

	ak_print_normal_ex("Exit task\r\n");
	ak_thread_exit();
	return NULL;
}

void *uploader_task(void *argv)
{
	int i = 0;
	int iRet = 0;
	unsigned long ulTime = 0;
    ENUM_SYS_MODE_t sys_get_mode;

	for(;;)
	{
		if(g_first_time_upload == 0) // first time upload
		{
			// push event
			// upload
			
			g_first_time_upload = 1;
		}
		else // after system init
		{
		}
		
		printf("1111...g_iDoneSnapshot = %d\r\n", g_iDoneSnapshot);
		// while(g_iDoneSnapshot == 0)
		while(g_iDoneSnapshot != SNAPSHOT_STATE_DONE)
		{
			i++;
			if(i%30 == 0){
				printf("'=.= ");
				i = 0;
			}
			ak_sleep_ms(100);
			sys_get_mode = eSysModeGet();
			if(E_SYS_MODE_FTEST == sys_get_mode || \
				E_SYS_MODE_FWUPGRADE == sys_get_mode)
			{
				ak_print_normal_ex("Exit1 loop wait snapshot flag!\r\n");
				break;
			}
		}
		if(E_SYS_MODE_FTEST == sys_get_mode || \
			E_SYS_MODE_FWUPGRADE == sys_get_mode)
		{
			ak_print_normal_ex("Exit2 loop upload task\r\n");
			break;
		}
		g_isDoneUpload = UPLOADER_STATE_RUNNING;
		g_iDoneSnapshot = SNAPSHOT_STATE_WAIT; // avoid upload loop forever

		ulTime = get_tick_count();
		// g_sTimeDebugUpload.start_uploader = ulTime;
		printf("===================================================\r\n");
		printf("Begin start uploader new flow for demo! Set g_iDoneSnapshot = %d\r\n", g_iDoneSnapshot);
		printf("===================================================\r\n");

		// g_isDoneUpload = 1;
		
		// iRet = file_post();

		//main_all_in_one();
		main_upload_new_flow_demo();

		printf("file_post: iRet %d time to upload: %u ms\r\n", iRet, get_tick_count() - ulTime);
		g_isDoneUpload = UPLOADER_STATE_DONE;
		printf("UPLOADER_STATE_DONE: time (%u)\r\n", get_tick_count());
		printf("***********************************************************\r\n");
		printf("***********************************************************\r\n");
		printf("\r\n\r\n\r\n");
		// why do we assign this varibale at here! This variable belong snapshot task!
		// g_iDoneSnapshot = 0;
	}

	ak_print_normal_ex("Exit task\r\n");
	ak_thread_exit();
	return NULL;
}

#ifndef UPLOADER_RUN_ON_AK_PLAT

int socket_api(int domain, int type, int protocol){
	return socket(domain, type, protocol);
}
	
int connect_api(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
	return connect(sockfd, addr, addrlen);
}

ssize_t send_api(int sockfd, const void *buf, size_t len, int flags){
	usleep(1000);
	int i;
	char l_byte;
	// for (i=0;i<len;i++){
	// 	l_byte=((char*)buf)[i];
	// 	if ((l_byte>=10) && (l_byte<=127))
	// 		printf("%c",l_byte);
	// }
	// printf("send_api %d byte\r\n",len);
	return send(sockfd, buf, len, flags);
}
	
ssize_t recv_api(int sockfd, void *buf, size_t len, int flags){
	printf("recv_api %d byte\r\n",len);
	return recv(sockfd, buf, len, MSG_DONTWAIT);
}

int close_api(int fd){
	return close(fd);
}

size_t fread_api( void * ptr, size_t size, size_t count, FILE * stream ){
	return fread ( ptr, size, count, stream );
}

FILE * fopen_api ( const char * filename, const char * mode ){
	return fopen ( filename, mode );
}
	
int fclose_api(FILE *fp){
	return fclose(fp);
}
	

#else // for AK platform

int socket_api(char *hostname, char *port, int protocol){
	return upload_socket_api(hostname, port, protocol);
}

ssize_t send_api(int sockfd, const void *buf, size_t len, int flags){
	// usleep(20*1000);
	printf("send_api %d byte\r\n",len);
	return 	upload_send_api(sockfd, buf, len, flags);
}
	
ssize_t recv_api(int sockfd, void *buf, size_t len, int flags){
	printf("recv_api %d byte\r\n",len);
	return upload_recv_api(sockfd, buf, len, flags);
}

int close_api(int fd){
	return upload_close_api(fd);
}

/*
* Get MAC address
*/
// static int get_mac_address(char *mac)
// {
//     if(!mac){
//         return -1;
//     }
//     // hardcode
//     memcpy(mac, CON_MAC_ADDRESS, strlen(CON_MAC_ADDRESS));
//     return 0;
// }

/*
* Get MAC address
*/
static int get_mac_address(char *mac)
{
    int i = 0;
    if(!mac){
        return -1;
    }
    for(i = 0; i < 12; i++)
    {
        *(mac + i) = g_macaddress[i];
    }
    return 0;
}

/*
* Get name file snap and clip 
*/
static int get_file_name_motion(char *file_name)
{	
	int ret = 0;
#ifdef UPLOADER_RUN_ON_AK_PLAT
	char fn[256] = {0x00};
	char mac_addr[16] = {0x00};
	memset(fn, 0x00, 256);
	ret = snapshot_get_file_name(fn);
	// memset(mac_addr, 0x00, sizeof(mac_addr));
	// if(get_mac_address(mac_addr) < 0){
	// 	return -1;
	// }
	sprintf(file_name, "%s", fn);

#else
	time_t sLocalTime;
	struct tm *timeinfo;
	char mac_addr[16] = {0x00};

	if(!file_name){
		return -1;
	}
	memset(mac_addr, 0x00, sizeof(mac_addr));
	if(get_mac_address(mac_addr) < 0){
		return -1;
	}
	time(&sLocalTime);
	timeinfo = (struct tm *)localtime(&sLocalTime);
	sprintf(file_name, "%s_%04d%02d%02d%02d%02d%02d",
						mac_addr,
						timeinfo->tm_year + 1900,
						timeinfo->tm_mon + 1,
						timeinfo->tm_mday,
						timeinfo->tm_hour,
						timeinfo->tm_min,
						timeinfo->tm_sec);
#endif
	return ret;
}

int gen_motion_filename(char* g_generated_filename, int g_generated_filename_len)
{
    int ret = 0;
    // char file_name[MAX_FILENAME_LEN+1] = {0x00};
	char file_name[128] = {0x00};
    //filename
    memset(file_name, 0x00, sizeof(file_name));
    if(get_file_name_motion(file_name) < 0){
        printf("Get file_name system failed!");
        return -2;
    }
    memset(g_generated_filename, 0x00, g_generated_filename_len);
    if (strlen(file_name)<g_generated_filename_len)
    	memcpy(g_generated_filename, file_name, strlen(file_name));
    else
    	return -3;
    printf("file name snapshot: %s", g_generated_filename);
    return ret;
}

void port_sleep_ms(int l_duration_ms){
	// usleep(l_duration_ms*1000);
	ak_sleep_ms(l_duration_ms);
}

int fsize_api(const char *filename){
	FILE * fp;
	// int l_length;
    // fp = fopen ( filename, "r" );
    // fseek(fp, 0L, SEEK_END);
    // l_length=ftell(fp);
    // fclose(fp);
	// return l_length;
	if (filename!=NULL)
		return snapshot_get_size_data();
	else{
		fp = f_mem_open("", "r");
		return (((struct fs_on_mem_struct*)fp)->max_length);
	}
}



#endif


