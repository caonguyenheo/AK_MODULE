/**
  ******************************************************************************
  * File Name          : fwupgrade.h
  * Description        : 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 CINATIC LTD COMPANY
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FWUPGRADE_H__
#define __FWUPGRADE_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 


/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

#define FWUPGRADE_BUF_SIZE	            (4*1024)
#define FWUPGRADE_SIZE_BUF_READ         (4*1024*1024)
        

#define error_check(x)                  if(!x) { \
                                            ak_print_error("Parameter null!\n"); \
                                            return -1;}

/* structure process packet spi from CC32xx */
typedef struct fwupg_header
{
    unsigned char type;         // 1 byte
    unsigned char control;      // 1 byte
    unsigned short len_packet;  // 2 byte
    unsigned int len_binary;    // 4 byte
    unsigned int position;      // 4 byte
}FWUPG_HEADER_t __attribute__((aligned(16))); 

typedef struct fwupg_info
{
    FWUPG_HEADER_t header;
    unsigned char *data;
}FWUPG_INFO_t __attribute__((aligned(16))); 

typedef struct fwupg_hashing_packet
{
    unsigned char type;         // 1 byte
    unsigned char control;      // 1 byte
    unsigned short len_packet;  // 2 byte
    unsigned char *data;
}FWUPG_HASHING_PACKET_t __attribute__((aligned(16)));

/* structure process data binary */
/* length of data binary */
/* hashing string. md5 or sha1 sha256 */
typedef struct fwupg_binary
{
    unsigned long   length_bin;
    unsigned long   current_length;
    unsigned char *start_point;
    unsigned char *end_point;
    unsigned char *work_point;      // position in AK side
    unsigned char *position_point; // position in CC3200 side
    unsigned char *hash_key;
    unsigned int    length_key;
}FWUPG_BINARY_t;

typedef enum fw_type_notify
{
    E_FW_TYPE_NOTIFY_INIT = 0,
    E_FW_TYPE_NOTIFY_STOP_UPGRADING,
    E_FW_TYPE_NOTIFY_DONE_UPGRADING,
    E_FW_TYPE_NOTIFY_MAX
}fw_etype_notify_t;

typedef enum FW_STATE
{
    E_FW_STATE_INIT = 0,
    E_FW_STATE_RECV_BIN,
    E_FW_STATE_VERIFY,
    // E_FW_STATE_FINISHED_BURNING,
    E_FW_MAX_STATE
}fw_eState_t;

typedef enum FW_EVENT
{
    E_FW_EVENT_RECV_PACKET = 0,
    E_FW_MAX_EVENT
}fw_eEvent_t;

typedef int (*action)(char *pRecv, char *pResp);


/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
void fwupgrade_main(void);
void *fwupgrade_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __FWUPGRADE_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

