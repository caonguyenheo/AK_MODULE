/**
  ******************************************************************************
  * File Name          : upload_main.h
  * Description        : 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UPLOAD_MAIN_H__
#define __UPLOAD_MAIN_H__
#ifdef __cplusplus
 extern "C" {
#endif

#include <sys/types.h>          /* See NOTES */
#include "ak_apps_config.h"

// #include <sys/socket.h>

//Thuan edited - 31/10/2017
#if 1
    #define HTTP_CHUNK_SIZE 1000
#endif

#if 1
    #define HTTP_NUMBER_OF_KB                   25
    #define HTTP_CHUNK_SIZE_NKB                 (HTTP_NUMBER_OF_KB*1000) //send 5 packets (1KB/packet) every calling the function upload_send_api()
#endif

#define MAX_FILENAME_LEN 255
#define MAX_URL_LEN 255
// #define SERVER_S3
// #define SERVER_CEPHS3
#define SERVER_HTTP

// int socket_api(int domain, int type, int protocol);

int socket_api(char *hostname, char *port, int protocol);
ssize_t send_api(int sockfd, const void *buf, size_t len, int flags);
// int connect_api(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t recv_api(int sockfd, void *buf, size_t len, int flags);
int close_api(int fd);

void *uploader_task(void *argv);

void *uploader_task_new_push(void *argv);



#ifndef UPLOADER_RUN_ON_AK_PLAT
    size_t fread_api( void * ptr, size_t size, size_t count, FILE * stream );
    FILE * fopen ( const char * filename, const char * mode );
    int fclose_api(FILE *fp);
#endif

int gen_motion_filename(char* g_generated_filename, int g_generated_filename_len);
void port_sleep_ms(int l_duration_ms);

void port_sleep_ms(int l_duration_ms);

int fsize_api(const char *filename);
int main_all_in_one(void);

#ifdef __cplusplus
}
#endif
#endif //__UPLOAD_MAIN_H__
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/
