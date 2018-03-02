/**
  ******************************************************************************
  * File Name          : p2psdstream_list.c
  * Description        : This file contain functions for linkedlist packet resend
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  ******************************************************************************
  */

/*---------------------------------------------------------------------------*/
/*                           INCLUDED FILES                                  */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "p2psdstream_list.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/

#define __list_for_each             list_for_each

typedef struct pkt_list
{
    P2P_PACKET_RESEND_t     *packet_resend; // data
    struct list_head    pkt_list;
}P2P_PACKET_LIST_t;

/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/
void *p2p_pktlist_init_head(void)
{
    struct list_head *p_handle = NULL;
    p_handle = (struct list_head *)malloc(sizeof(struct list_head));
    if(p_handle == NULL)
    {
        printf("%s Init head failed!\n", __func__);
        return NULL;
    }

    p_handle->next = p_handle;
    p_handle->prev = p_handle;

    printf("%s p_handle: %p, next: %p, prev: %p\n", __func__, p_handle, p_handle->next, p_handle->prev);
    return (void *)p_handle;
}

void p2p_pktlist_deinit_head(struct list_head *p_handle)
{
    if(p_handle != NULL)
    {
        p2p_pktlist_delete_all(p_handle);
        free(p_handle);
    }
}


/*
*   allocate memory for structure data, P2P_PACKET_RESEND_t
*/
P2P_PACKET_RESEND_t *p2p_pktlist_malloc_packet_resend(int *p_error)
{
    P2P_PACKET_RESEND_t *p_packet_resend = NULL;
    char *p_packet_spi = NULL;
    p_packet_resend = (P2P_PACKET_RESEND_t *)malloc(sizeof(P2P_PACKET_RESEND_t));
    if(p_packet_resend != NULL)
    {
        /* Allocate memory for spi packet */
        p_packet_spi = (char *)malloc(MAX_SPI_PACKET_LEN);
        if(p_packet_spi == NULL)
        {
            // error: cannot allocate memory
            // need to free packet resend
            free(p_packet_resend);
            p_packet_resend = NULL;
            if(p_error != NULL)
            {
                *p_error = -2;
            }
        }
        else
        {
            p_packet_resend->pkt_data = p_packet_spi;
            if(p_error != NULL)
            {
                *p_error = 0;
            }
        }
    }
    else
    {
        if(p_error != NULL)
        {
            *p_error = -1;
        }
    }
    return p_packet_resend;
}

int p2p_pktlist_free_packet_resend(P2P_PACKET_RESEND_t *p_packet_resend)
{
    int i32Ret = 0;
    if(p_packet_resend != NULL)
    {
        if(p_packet_resend->pkt_data != NULL)
        {
            // free packet spi
            free(p_packet_resend->pkt_data);
            // free packet resend
            free(p_packet_resend);
        }
    }
    return i32Ret;
}

/*
*   p_packet_resend must be allocated memory before call this function
*/
int p2p_pktlist_add_node(P2P_PACKET_RESEND_t *p_packet_resend, struct list_head *p_head)
{
    int i32Ret = 0;
    P2P_PACKET_LIST_t *pPacketList = NULL;

    // check parameter
    if((p_packet_resend == NULL) || (p_head == NULL))
    {
        return -1;
    }
    else
    {
        //debug
        // printf("%s p_packet_resend: %p, p_head: %p\n", __func__, p_packet_resend, p_head);
    }

    // allocate memory for a new packet list
    pPacketList = (P2P_PACKET_LIST_t *)malloc(sizeof(P2P_PACKET_LIST_t));
    if(pPacketList == NULL)
    {
        return -2;
    }

    pPacketList->packet_resend = p_packet_resend;
    INIT_LIST_HEAD(&pPacketList->pkt_list);
    // list_add(&pPacketList->pkt_list, p_head);
    list_add_tail(&pPacketList->pkt_list, p_head);

    // printf("%s pPacketList->packet_resend: %p\n", __func__, pPacketList->packet_resend);

    return i32Ret;
}

/*
*   Display list
*/
void p2p_pktlist_display(struct list_head *p_head)
{
    struct list_head *iter;
    P2P_PACKET_LIST_t *pPacketList = NULL;
    if(p_head == NULL)
    {
        return;
    }

    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, P2P_PACKET_LIST_t, pkt_list);
        printf("%u ", pPacketList->packet_resend->pid);
    }
    printf("\n");
}

/*
*   Delete all list
*/
void p2p_pktlist_delete_all(struct list_head *p_head)
{
    struct list_head *iter;
    P2P_PACKET_LIST_t *pPacketList = NULL;
    if(p_head == NULL)
    {
        return;
    }

  redo:
    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, P2P_PACKET_LIST_t, pkt_list);
        list_del(&pPacketList->pkt_list);
        free(pPacketList);
        goto redo;
    }
}

int p2p_pktlist_find_first_pid_and_delete(unsigned int pid,
                        struct list_head *p_head,
                        P2P_PACKET_RESEND_t **pp_packet_resend)
{
    int i32Ret = 0;
    struct list_head *iter;
    P2P_PACKET_LIST_t *pPacketList = NULL;
    P2P_PACKET_RESEND_t *ppktrsnd = NULL;

    if(pp_packet_resend == NULL)
    {
        return -1;
    }
    ppktrsnd = *pp_packet_resend;

    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, P2P_PACKET_LIST_t, pkt_list);
        if(pPacketList->packet_resend->pid == pid) {
            // printf("p->PID: %u (p:%u)\n", pPacketList->packet_resend->pid, pid);
            ppktrsnd->pid = pPacketList->packet_resend->pid;
            ppktrsnd->timesend = pPacketList->packet_resend->timesend;
            ppktrsnd->type = pPacketList->packet_resend->type;
            memcpy(ppktrsnd->pkt_data,
                    pPacketList->packet_resend->pkt_data,
                    MAX_SPI_PACKET_LEN);
            // delete node
            free(pPacketList->packet_resend->pkt_data);
            list_del(&pPacketList->pkt_list);
            free(pPacketList);
            return 1;
        }
        else if(pPacketList->packet_resend->pid < pid)
        {
            free(pPacketList->packet_resend->pkt_data);
            list_del(&pPacketList->pkt_list);
            return 0;
            // free(pPacketList);
        }
        else if(pPacketList->packet_resend->pid > pid)
        {
            // if PID in queue is greater than PID ACK, break it
            return 0;
        }
    }
    return i32Ret;
}

int p2p_pktlist_compare_timestamp_and_delete(unsigned long ts,
                        struct list_head *p_head,
                        P2P_PACKET_RESEND_t **pp_packet_resend)
{
    int i32Ret = 0;
    struct list_head *iter;
    P2P_PACKET_LIST_t *pPacketList = NULL;
    P2P_PACKET_RESEND_t *ppktrsnd = NULL;
    unsigned long timeout = 0;

    if(pp_packet_resend == NULL)
    {
        return -1;
    }
    ppktrsnd = *pp_packet_resend;

    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, P2P_PACKET_LIST_t, pkt_list);
        timeout = ts - pPacketList->packet_resend->timesend;
        // if((ts - pPacketList->packet_resend->timesend) >= P2P_SD_STREAM_MAX_TIMEOUT) 
        if( timeout >= P2P_SD_STREAM_PACKET_MAX_TIMEOUT) 
        {
            ppktrsnd->pid = pPacketList->packet_resend->pid;
            ppktrsnd->timesend = pPacketList->packet_resend->timesend;
            ppktrsnd->type = pPacketList->packet_resend->type;
            memcpy(ppktrsnd->pkt_data,
                    pPacketList->packet_resend->pkt_data,
                    MAX_SPI_PACKET_LEN);
            // delete node
            free(pPacketList->packet_resend->pkt_data);
            list_del(&pPacketList->pkt_list);
            free(pPacketList);
            printf("(%u:%u)\n", ppktrsnd->pid, timeout);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return i32Ret;
}


/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/
