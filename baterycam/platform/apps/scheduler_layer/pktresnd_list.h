/**
  ******************************************************************************
  * File Name          : pktresnd_list.h
  * Description        : This file contains all structure of linkedlist packet 
  *                     resend feature 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PKTRESND_LIST_H__
#define __PKTRESND_LIST_H__
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
}PACKET_RESEND_t;

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/

void *pktlist_init_head(void);
void pktlist_deinit_head(struct list_head *p_handle);


PACKET_RESEND_t *pktlist_malloc_packet_resend(int *p_error);
int pktlist_free_packet_resend(PACKET_RESEND_t *p_packet_resend);
/*
*   p_packet_resend must be allocated memory before call this function
*/
int pktlist_add_node(PACKET_RESEND_t *p_packet_resend, struct list_head *pHead);
/*
*   Display list
*/
void pktlist_display(struct list_head *pHead);
/*
*   Delete all list
*/
void pktlist_delete_all(struct list_head *pHead);
PACKET_RESEND_t * pktlist_find_first_pid(unsigned int pid, struct list_head *p_head);
int pktlist_find_first_pid_and_delete(unsigned int pid,
                        struct list_head *p_head,
                        PACKET_RESEND_t **pp_packet_resend);
int pktlist_compare_timestamp_and_delete(unsigned long ts,
                        struct list_head *p_head,
                        PACKET_RESEND_t **pp_packet_resend);

#ifdef __cplusplus
}
#endif

#endif /* __PKTRESND_LIST_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/


