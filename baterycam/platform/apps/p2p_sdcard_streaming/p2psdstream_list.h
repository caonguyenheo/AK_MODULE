/**
  ******************************************************************************
  * File Name          : p2psdstream_list.h
  * Description        : This file contains all structure of linkedlist packet 
  *                     resend feature 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __P2PSDSTREAM_PKTRESND_LIST_H__
#define __P2PSDSTREAM_PKTRESND_LIST_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 
#include "list.h"
#include "ak_apps_config.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

typedef struct pkt_resnd
{
    unsigned int    pid;
    unsigned long   timesend; // time send
    char            type; // type of packet
    char            *pkt_data; // SPI data, 1024 byte
}P2P_PACKET_RESEND_t;

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/

void *p2p_pktlist_init_head(void);
void p2p_pktlist_deinit_head(struct list_head *p_handle);


P2P_PACKET_RESEND_t *p2p_pktlist_malloc_packet_resend(int *p_error);
int p2p_pktlist_free_packet_resend(P2P_PACKET_RESEND_t *p_packet_resend);
/*
*   p_packet_resend must be allocated memory before call this function
*/
int p2p_pktlist_add_node(P2P_PACKET_RESEND_t *p_packet_resend, struct list_head *pHead);
/*
*   Display list
*/
void p2p_pktlist_display(struct list_head *pHead);
/*
*   Delete all list
*/
void p2p_pktlist_delete_all(struct list_head *pHead);
P2P_PACKET_RESEND_t * p2p_pktlist_find_first_pid(unsigned int pid, struct list_head *p_head);
int p2p_pktlist_find_first_pid_and_delete(unsigned int pid,
                        struct list_head *p_head,
                        P2P_PACKET_RESEND_t **pp_packet_resend);
int p2p_pktlist_compare_timestamp_and_delete(unsigned long ts,
                        struct list_head *p_head,
                        P2P_PACKET_RESEND_t **pp_packet_resend);

#ifdef __cplusplus
}
#endif

#endif /* __P2PSDSTREAM_PKTRESND_LIST_H__
 */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/


