/**
  ******************************************************************************
  * File Name          : ringbuf.c
  * Description        : This file contain functions for ringbuf
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #include "hal_timer.h"

#include "packet_resend.h"
#include "ak_apps_config.h"
#include "pktresnd_list.h"
#include "spi_transfer.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

static int i32CountPacketBackup = 0;

static int g_iPacketResendRate = 0;
static int g_iPacketBackup = 0;
static int g_iPacketTimeout = 0;

/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
int packet_resend_get_resendrate(void)
{
    int ret = 0;
    ret = g_iPacketResendRate;
    g_iPacketResendRate = 0;
    return ret;
}
int packet_resend_get_numbertimeout(void)
{
    int ret = 0;
    ret = g_iPacketTimeout;
    g_iPacketTimeout = 0;
    return ret;
}
int packet_resend_get_numberbackup(void)
{
    int ret = 0;
    ret = g_iPacketBackup;
    g_iPacketBackup = 0;
    return ret;
}
int packet_resend_get_numberavailbalebackup(void)
{
    return i32CountPacketBackup;
}

/*----------------------------------------------------------------------------*/

void * packet_resend_init(void)
{
    return pktlist_init_head();
}

void packet_resend_deinit(void *p_handle)
{
    if(p_handle != NULL)
    {
        pktlist_deinit_head(p_handle);
    }
}

int packet_resend_push(void *p_handle, char *p_data, int length)
{
    int i32Ret = 0;
    char *p_packet_spi = NULL;
    PACKET_RESEND_t *p_packet_resend = NULL;

    if((p_handle == NULL) || (p_data == NULL) || (length != 1024))
    {
        return -1;
    }

    if(i32CountPacketBackup>= CFG_RESEND_MAX_PACKET_SAVE)
    {
        printf("%s Reach RESEND_MAX_PACKET_SAVE\n", __func__);
        // FIXME: delete final node and backup current node 
        return -3;
    }
    else
    {
        // printf("\n%s Count: %d, ", __func__, i32CountPacketBackup);
    }

    p_packet_resend = pktlist_malloc_packet_resend(&i32Ret);
    if(p_packet_resend == NULL)
    {
        printf("%s allocate failed - ret(%d)\n", __func__, i32Ret);
        return -2;
    }

    // printf("%s p_packet_resend->pkt_data:%p\n", __func__, p_packet_resend->pkt_data);
    memcpy(p_packet_resend->pkt_data, p_data, length);
    p_packet_resend->type = *(p_packet_resend->pkt_data + 8); // the first element
    p_packet_resend->pid = (*(p_packet_resend->pkt_data + PKTRSND_PID_1_OFFSET) & 0xFF) << 24;
    p_packet_resend->pid |= (*(p_packet_resend->pkt_data + PKTRSND_PID_2_OFFSET) & 0xFF) << 16;
    p_packet_resend->pid |= (*(p_packet_resend->pkt_data + PKTRSND_PID_3_OFFSET) & 0xFF) << 8;
    p_packet_resend->pid |= (*(p_packet_resend->pkt_data + PKTRSND_PID_4_OFFSET) & 0xFF) << 0;
    p_packet_resend->timesend = get_tick_count();

/*
    printf("p_packet_resend: %p, PID: %u, type: %d, ts: %u", \
                            p_packet_resend, \
                            p_packet_resend->pid, \
                            p_packet_resend->type, \
                            p_packet_resend->timesend);
*/  
    // printf("b%d_", p_packet_resend->pid);
    // insert packet to linkedlist
    i32Ret = pktlist_add_node(p_packet_resend, (struct list_head *)p_handle);
    // printf(", ret:%d\n", i32Ret);
    i32CountPacketBackup++;
    g_iPacketBackup++;
    if(i32CountPacketBackup%10 == 0)
    {
        printf("\n");
    }
    return i32Ret;
}


// /*
// * get data and resend
// */
// int packet_resend_pop(void *p_handle, char *p_data, int length)
// {

// }

void packet_resend_show_ack(char *p_data_ack, int length)
{
    int i = 0;
    char str[2048] = {0x00, };
    memset(str, 0x00, sizeof(str));
    for(i = 0; i < length; i++)
    {
        sprintf(str + strlen(str), "%02x_", *(p_data_ack + i));
    }
    printf(">>>%s\n", str);
}

/*
* process packet ACK
* [4byte Smallest PID][ACK Data 336]
*/
int packet_resend_process_ack(void *p_handle, char *p_data_ack, int length)
{
    int i32Ret = 0;
    int i = 0;
    unsigned int uiSmallestPID = 0;
    unsigned int uiPID = 0;
    PACKET_RESEND_t *p_pktrsnd = NULL;
    unsigned long ts = 0;

    if(p_data_ack == NULL)
    {
        return -1;
    }

    uiSmallestPID = (*(p_data_ack + 0) & 0xFF) << 24;
    uiSmallestPID |= (*(p_data_ack + 1) & 0xFF) << 16;
    uiSmallestPID |= (*(p_data_ack + 2) & 0xFF) << 8;
    uiSmallestPID |= (*(p_data_ack + 3) & 0xFF) << 0;

    // printf("%s uiSmallestPID: %u\n", __func__, uiSmallestPID);
    // packet_resend_show_ack(p_data_ack, length);

    p_pktrsnd = pktlist_malloc_packet_resend(&i32Ret);
    if(p_pktrsnd == NULL)
    {
        printf("%s Cannot allocate memory (%d)\n", __func__, i32Ret);
        return -2;
    }
    // search and check PID
    // if we receive PID ACK, remove that packet from list
    // if we don't receive PID ACK, pop it and resend

    p_data_ack += 4;
    for(i = 0; i < (length - 4); i++)
    {
        uiPID = uiSmallestPID + i;
        // Receive ACK
        if((*(p_data_ack + i) == 0) || (*(p_data_ack + i) == 1))
        {
            // find pid, copy data and delete it
            i32Ret = pktlist_find_first_pid_and_delete(uiPID, \
                                (struct list_head *)p_handle, &p_pktrsnd);
            if(i32Ret == 1)
            {
                i32CountPacketBackup--;
                // g_iPacketBackup--;
                // If NAK resend it
                if(*(p_data_ack + i) == 0)
                {
                    i32Ret = send_spi_data(p_pktrsnd->pkt_data, MAX_SPI_PACKET_LEN);
                    #ifndef UPLOADER_DEBUG
                    printf(">%d ", p_pktrsnd->pid);
                    #endif
                    g_iPacketResendRate++;
                }
                else
                {
                    ts =  get_tick_count() - p_pktrsnd->timesend;
                    #ifndef UPLOADER_DEBUG
                    printf("..%d:%u ", p_pktrsnd->pid, ts);
                    #endif
                }
            }
        }
        else
        {
            printf("ACK invalid: %d!\n", *(p_data_ack + i));
        }
    }
    pktlist_free_packet_resend(p_pktrsnd);
    return i32Ret;
}

/*
* process packet timeout
*/
int packet_resend_process_timeout(void *p_handle)
{
    int i32Ret = 0;
    unsigned long cur_ts = 0;
    PACKET_RESEND_t *p_pktrsnd = NULL;

    p_pktrsnd = pktlist_malloc_packet_resend(&i32Ret);
    if(p_pktrsnd == NULL)
    {
        printf("%s Cannot allocate memory (%d)\n", __func__, i32Ret);
        return -2;
    }

    while(1)
    {
        cur_ts = get_tick_count();
        // search and compare timestamp
        // if we timestamp - timesend >= CFG_RESEND_MAX_TIMEOUT, 
        // pop it and resend
        i32Ret = pktlist_compare_timestamp_and_delete(cur_ts, \
                                    (struct list_head *)p_handle, &p_pktrsnd);
        if(i32Ret == 1)
        {
            #ifndef UPLOADER_DEBUG
            printf("!%d ", p_pktrsnd->pid);
            #endif
            i32Ret = send_spi_data(p_pktrsnd->pkt_data, MAX_SPI_PACKET_LEN);
            i32CountPacketBackup--;   
            // g_iPacketBackup--;
            g_iPacketTimeout++; 
        }
        else
        {
            // printf("Don't find packet timeout\n");
            break;
        }
    }
    pktlist_free_packet_resend(p_pktrsnd);
    return 0;
}



/*
***********************************************************************
*                           END OF FILES                              *
***********************************************************************
*/

