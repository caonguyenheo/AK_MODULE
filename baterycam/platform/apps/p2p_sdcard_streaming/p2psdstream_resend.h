/**
  ******************************************************************************
  * File Name          : p2psdstream_packet_resend.h
  * Description        : This file contains all structure of packet resend 
  *                     feature 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __P2PSDSTREAM_PACKET_RESEND_H__
#define __P2PSDSTREAM_PACKET_RESEND_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 
#include "ak_apps_config.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

// [byte 0: spi type]
// [byte 1: high byte length]
// [byte 2: low byte length]
// [byte 3: AUTH 'V']
// [byte 4: AUTH 'L']
// [byte 5: AUTH 'V']
// [byte 6: AUTH 'L']
// [byte 7: Reserve]
// [byte 8: Packet type]
// [byte 9: Timestamp]
// [byte 10: Timestamp]
// [byte 11: Timestamp]
// [byte 12: Timestamp]
// [byte 13: Block length]
// [byte 14: Block length]
// [byte 15: Block PID]
// [byte 16: Block PID]
// [byte 17: Block PID]
// [byte 18: Block PID]
// [byte 19: PID]
// [byte 20: PID]
// [byte 21: PID]
// [byte 22: PID]

// parse from SPI packet
#define P2P_PKTRSND_PACKET_TYPE_OFFSET            (8)
#define P2P_PKTRSND_PID_1_OFFSET                  (19)  // MSB
#define P2P_PKTRSND_PID_2_OFFSET                  (P2P_PKTRSND_PID_1_OFFSET+1)
#define P2P_PKTRSND_PID_3_OFFSET                  (P2P_PKTRSND_PID_2_OFFSET+1)
#define P2P_PKTRSND_PID_4_OFFSET                  (P2P_PKTRSND_PID_3_OFFSET+1)  // LSB


typedef struct pkt_rsnd_handle
{
    long int            tid;    // thread id
    unsigned long       count;  // number of packets
    void                *handle;  // head pointer
}P2P_PKTRESND_HANDLE_t;


/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
int p2p_packet_resend_get_resendrate(void);
int p2p_packet_resend_get_numbertimeout(void);
int p2p_packet_resend_get_numberbackup(void);
int p2p_packet_resend_get_numberavailbalebackup(void);

void * p2p_packet_resend_init(void);
void p2p_packet_resend_deinit(void *p_handle);

int p2p_packet_resend_push(void *p_handle, char *p_data, int length);
int p2p_packet_resend_process_ack(void *p_handle, char *p_data_ack, int length);
int p2p_packet_resend_process_timeout(void *p_handle);


#ifdef __cplusplus
}
#endif

#endif /* __P2PSDSTREAM_PACKET_RESEND_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/


