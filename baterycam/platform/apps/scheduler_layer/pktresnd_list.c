/**
  ******************************************************************************
  * File Name          : pktresnd_list.c
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
#include "pktresnd_list.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/

#define __list_for_each             list_for_each

typedef struct pkt_list
{
    PACKET_RESEND_t     *packet_resend; // data
    struct list_head    pkt_list;
}PACKET_LIST_t;

/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/
void *pktlist_init_head(void)
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

void pktlist_deinit_head(struct list_head *p_handle)
{
    if(p_handle != NULL)
    {
        pktlist_delete_all(p_handle);
        free(p_handle);
    }
}


/*
*   allocate memory for structure data, PACKET_RESEND_t
*/
PACKET_RESEND_t *pktlist_malloc_packet_resend(int *p_error)
{
    PACKET_RESEND_t *p_packet_resend = NULL;
    char *p_packet_spi = NULL;
    p_packet_resend = (PACKET_RESEND_t *)malloc(sizeof(PACKET_RESEND_t));
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

int pktlist_free_packet_resend(PACKET_RESEND_t *p_packet_resend)
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
int pktlist_add_node(PACKET_RESEND_t *p_packet_resend, struct list_head *p_head)
{
    int i32Ret = 0;
    PACKET_LIST_t *pPacketList = NULL;

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
    pPacketList = (PACKET_LIST_t *)malloc(sizeof(PACKET_LIST_t));
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
void pktlist_display(struct list_head *p_head)
{
    struct list_head *iter;
    PACKET_LIST_t *pPacketList = NULL;
    if(p_head == NULL)
    {
        return;
    }

    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, PACKET_LIST_t, pkt_list);
        printf("%u ", pPacketList->packet_resend->pid);
    }
    printf("\n");
}

/*
*   Delete all list
*/
void pktlist_delete_all(struct list_head *p_head)
{
    struct list_head *iter;
    PACKET_LIST_t *pPacketList = NULL;
    if(p_head == NULL)
    {
        return;
    }

  redo:
    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, PACKET_LIST_t, pkt_list);
        list_del(&pPacketList->pkt_list);
        free(pPacketList);
        goto redo;
    }
}

int pktlist_find_first_pid_and_delete(unsigned int pid,
                        struct list_head *p_head,
                        PACKET_RESEND_t **pp_packet_resend)
{
    int i32Ret = 0;
    struct list_head *iter;
    PACKET_LIST_t *pPacketList = NULL;
    PACKET_RESEND_t *ppktrsnd = NULL;

    if(pp_packet_resend == NULL)
    {
        return -1;
    }
    ppktrsnd = *pp_packet_resend;

    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, PACKET_LIST_t, pkt_list);
        if(pPacketList->packet_resend->pid == pid) {
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
        else if(pPacketList->packet_resend->pid > pid)
        {
            // if PID in queue is greater than PID ACK, break it
            return 0;
        }
    }
    return i32Ret;
}

int pktlist_compare_timestamp_and_delete(unsigned long ts,
                        struct list_head *p_head,
                        PACKET_RESEND_t **pp_packet_resend)
{
    int i32Ret = 0;
    struct list_head *iter;
    PACKET_LIST_t *pPacketList = NULL;
    PACKET_RESEND_t *ppktrsnd = NULL;

    if(pp_packet_resend == NULL)
    {
        return -1;
    }
    ppktrsnd = *pp_packet_resend;

    __list_for_each(iter, p_head) {
        pPacketList = list_entry(iter, PACKET_LIST_t, pkt_list);
        // printf("#%d", pPacketList->packet_resend->pid);
        // FIX ME: need to check validate ts before compare
        if((ts - pPacketList->packet_resend->timesend) >= CFG_RESEND_MAX_TIMEOUT) 
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
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return i32Ret;
}


// PACKET_RESEND_t * pktlist_find_first_pid(unsigned int pid, struct list_head *p_head,
//                                 void **pp_pos_list)
// {
//     struct list_head *iter;
//     PACKET_LIST_t *pPacketList = NULL;

//     __list_for_each(iter, p_head) {
//         pPacketList = list_entry(iter, PACKET_LIST_t, pkt_list);
//         if(pPacketList->packet_resend->pid == pid) {
//             return pPacketList->packet_resend;
//         }
//     }
//     return NULL;
// }

// int pktlist_delete_node(struct list_head *p_head)
// {
//     int i32Ret = 0;

//     return i32Ret;
// }

// int find_first_and_delete(int arg, struct list_head *head)
// {
//     struct list_head *iter;
//     struct foo *objPtr;

//     __list_for_each(iter, head) {
//         objPtr = list_entry(iter, struct foo, list_member);
//         if(objPtr->info == arg) {
//             list_del(&objPtr->list_member);
//             free(objPtr);
//             return 1;
//         }
//     }

//     return 0;
// }

// main() 
// {
//     LIST_HEAD(fooHead);
//     LIST_HEAD(barHead);
    
//     add_node(10, &fooHead);
//     add_node(20, &fooHead);
//     add_node(25, &fooHead);
//     add_node(30, &fooHead);
    
//     display(&fooHead);
//     find_first_and_delete(20, &fooHead);
//     display(&fooHead);
//     delete_all(&fooHead);
//     display(&fooHead);
// }


/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/
