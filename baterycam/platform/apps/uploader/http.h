/**
  ******************************************************************************
  * File Name          : http.h
  * Description        : This file contains define for http post 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  *
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HTTP_POST_H__
#define __HTTP_POST_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
#define API_URL g_api_url
#define API_PORT g_api_port

#define  HTTP_POST_LINE_FEED                "\r\n"

typedef struct post_multipart{
    char boundary[64];
    char request_path[128];
    char host_name[128];
    
}POST_MULTIPART_t;

#define URL_CINATIC_EVENT_ADD     "http://dev-api.cinatic.com:8080/events/add"
#define URL_AMAZON_SERVER        "http://cinatic-resources.s3.amazonaws.com:80"
#define URL_CINATIC_SUCCESS       "http://dev-api.cinatic.com:80/events/upload_success"
       

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/

int file_post(void);

int http_post_add_event_api(void);




#ifdef __cplusplus
}
#endif

#endif /* __HTTP_POST_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/


