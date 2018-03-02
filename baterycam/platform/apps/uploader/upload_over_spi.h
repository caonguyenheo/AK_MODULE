/**
  ******************************************************************************
  * File Name          : upload_over_spi.h
  * Description        :
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UPLOAD_OVER_SPI_H__
#define __UPLOAD_OVER_SPI_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                            INCLUDED FILES                                 */
/*---------------------------------------------------------------------------*/

#include <stdint.h> 
#include "ak_apps_config.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/
#define UPLOAD_CTRL_BYTE_OFFSET             (1) // 1 bytes
#define UPLOAD_LEN_BYTE_OFFSET              (2) // 2 bytes
#define UPLOAD_SSL_BYTE_OFFSET              (UPLOAD_LEN_BYTE_OFFSET + 2)    // 1byte
#define UPLOAD_PORT_BYTE_OFFSET             (UPLOAD_SSL_BYTE_OFFSET + 1)
#define UPLOAD_URL_BYTE_OFFSET              (UPLOAD_PORT_BYTE_OFFSET + 2)

#define UPLOAD_POSITION_BYTE_OFFSET         (UPLOAD_LEN_BYTE_OFFSET + 2) // 4 bytes
#define UPLOAD_DATA_BYTE_OFFSET             (UPLOAD_POSITION_BYTE_OFFSET + 4)

#define UPLOAD_RESPONSE_POSITION_BYTE_OFFSET         (2) // 4 bytes

#define UPLOAD_PROTOCOL_HTTP                (0)
#define UPLOAD_PROTOCOL_HTTPS               (1)

/* define byte control for upload */
#define UPLOAD_CTRL_START_SOCKET            0x01
#define UPLOAD_CTRL_SOCKET_READY            0x02
#define UPLOAD_CTRL_UPLOADING               0x03
#define UPLOAD_CTRL_DATA_ERROR              0x04
#define UPLOAD_CTRL_RESERVED                0x05
#define UPLOAD_CTRL_RESPONSE                0x06
#define UPLOAD_CTRL_CLOSE_SOCKET            0x07

typedef enum UP_STATE
{
    E_UP_STATE_INIT = 0,
    E_UP_STATE_START_SOCKET,
    E_UP_STATE_UPLOADING,
    E_UP_STATE_RECV_RESPONSE,
    E_UP_STATE_CLOSE_SOCKET,
    E_UP_MAX_STATE
}up_eState_t;

typedef enum UP_EVENT
{
    E_UP_EVENT_SOCKET_OPEN = 0,
    E_UP_EVENT_SOCKET_SEND,
    E_UP_EVENT_SOCKET_RECV,
    E_UP_EVENT_SOCKET_CLOSE,
    E_UP_MAX_EVENT
}up_eEvent_t;


typedef int (*upload_func)(char *pRecv, char *pResp);

/*---------------------------------------------------------------------------*/
/*                            GLOBAL PROTOTYPES                              */
/*---------------------------------------------------------------------------*/

int upload_socket_api(char *hostname, char *port, int protocol);
ssize_t upload_send_api(int sockfd, const void *buf, size_t len, int flags);
ssize_t upload_recv_api(int sockfd, void *buf, size_t len, int flags);
int upload_close_api(int fd);

void uploader_flush_buffer_after_uploading(void);

#ifdef __cplusplus
}
#endif

#endif /* __UPLOAD_OVER_SPI_H__ */

/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/

