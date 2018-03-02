/**
  ******************************************************************************
  * File Name          : p2p_sdcard_streaming.c
  * Description        : 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdlib.h>
#include "packet.h"
#include "p2p_sdcard_streaming.h"
#include "ak_apps_config.h"
#include "p2psdstream_resend.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
extern ak_sem_t  sem_SpiCmdStartTransfer;
extern int g_iP2pSdStreamStart;

extern char g_SpiCmdFileStream[256];


static int g_total_packet_in_queue = 0; // it should be 336
static int g_packet_per_second = 0; // 100 packets/s

static long g_lSizeFileStream = 0;

typedef struct mnt_pid_ack
{
    char *data;
    int pid;
    unsigned long t2s;
    char ack;
}MNT_PID_ACK_t;


/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/

long p2p_get_size_file(void *fp)
{
    //Thuan add -06/10/2017
    FILE * pFile;
    long lSize = -1;
  
    pFile = (FILE *)fp;
    if (pFile==NULL) 
        ak_print_error(" \n -------------Error opening file-------------------- \n");
    else
    {
        fseek (pFile, 0, SEEK_END);   // non-portable
        lSize = ftell (pFile);
        fseek(pFile, 0, SEEK_SET);
        ak_print_error(" \n -------- Size of file: %ld bytes --------- \n",lSize);
    }
    return lSize;
}

static int p2psdstream_get_data_sd(void *fp, char *pData, int iLength)
{
    int iReadLen = 0;
    if(fp == NULL || pData == NULL || iLength <= 0)
    {
        ak_print_error("Parameter is invalid\r\n");
        iReadLen = -1;
    }
    else
    {
        iReadLen = fread(pData, sizeof(char), iLength, fp);
    }
    return iReadLen;
}

//Thuan add - 09/10/2017
static void *openfile_sdcard()
{   
    void *pSDCard_file;
    
    //sprintf(g_SpiCmdFileStream, "a:/thuantest.txt");

    pSDCard_file = fopen(g_SpiCmdFileStream, "r");
    
    if (pSDCard_file == NULL)
    {
        ak_print_error_ex("open file '%s' failed\r\n");
    }
    else
    {
        ak_print_error_ex("OK pSDCard_file: %p\r\n", pSDCard_file);
    }

    return pSDCard_file;
}

//Thuan edit - 16/10/2017
static int p2p_prepare_data_and_wrapper_ps(char *pDataOut, int *length)
{
    int iRet = 0;
    char *pDataIn = NULL;
    int iReadLen = 0;
    int iPacketCounter = 0;
    void *fp;
    
    //Thuan add - 16/10/2017
    fp = openfile_sdcard();
    if (fp == NULL)
    {
        ak_print_error_ex("Open file fail, pSDCard_file = NULL ! \r\n");
        return -1;
    }
   
    // get size of file
    g_lSizeFileStream = p2p_get_size_file(fp);
    
    if (g_lSizeFileStream <= 0)
    {
        ak_print_error_ex("file error, file size = %d  \r\n",g_lSizeFileStream);
        fclose(fp);
        return -1;
    }

    ak_print_normal_ex("ATrung: file size: %d\r\n", g_lSizeFileStream);

    pDataIn = (char *)malloc(sizeof(char) * g_lSizeFileStream);
    if(pDataIn == NULL)
    {
        ak_print_error_ex("Cannot allocate data for streaming\r\n");
        iRet = -1;
        fclose(fp);
    }
    else
    {
        iReadLen = fread(pDataIn, sizeof(char), g_lSizeFileStream, fp);
        if(iReadLen != g_lSizeFileStream)
        {
            ak_print_error_ex("Read data failed, readlen=%d/%d\r\n", iReadLen, g_lSizeFileStream);
            iRet = -2;
        }
        else
        {
            iRet = packet_create_p2p_sd_streaming(41, &iPacketCounter, pDataIn, \
                                            g_lSizeFileStream, pDataOut, length);
            if(iRet != 0)
            {
                ak_print_error_ex("Create p2p packet failed! (%d)\r\n", iRet);
            }
            else
            {
                ak_print_error_ex("packet_create_p2p_sd_streaming OK!\r\n");
            }
        }

        free(pDataIn);
        fclose(fp);
    }
    return iRet;
}

int p2p_send_done_uploading(void)
{
    int iRet = 0;
    char *pMsg = NULL;
    pMsg = (char *)malloc(1024);
    *(pMsg + 0) = SPI_CMD_FILE_TRANSFER;
    *(pMsg + 1) = SPICMD_CONTROL_TRANSFER_DONE;
    iRet = spi_p2psdstream_send(pMsg, MAX_SPI_PACKET_LEN);
    if(iRet != MAX_SPI_PACKET_LEN)
    {
        ak_sleep_ms(200);
        iRet = spi_p2psdstream_send(pMsg, MAX_SPI_PACKET_LEN);
    }
    free(pMsg);
    return 0;
}

int p2p_send_dummy(void)
{
    int iRet = 0;
    char *pMsg = NULL;
    pMsg = (char *)malloc(1024);
    memset(pMsg, 0x00, 1024);
    iRet = spi_p2psdstream_send(pMsg, MAX_SPI_PACKET_LEN);
    if(iRet != MAX_SPI_PACKET_LEN)
    {
        ak_sleep_ms(20);
        iRet = spi_p2psdstream_send(pMsg, MAX_SPI_PACKET_LEN);
    }
    free(pMsg);
    return 0;
}

void dump_packet_send(char *pMsg, int length)
{
    int i = 0;
    printf("Dump:");
    for(i = 0; i < length; i++)
    {
        printf("%02X ", *(pMsg + i));
    }
    printf("\r\n");
}

int g_SendCurrent = 0;

/*-------------------------------------------------------------------------------*/

int p2p_calc_pid_number(int iOutLength)
{
    int iRet = 0;
    iRet = iOutLength/MAX_SPI_PACKET_LEN;
    if(iOutLength%MAX_SPI_PACKET_LEN)
    {
        iRet++;
    }
    return iRet;
}



int p2p_get_new_dataload_and_send(char *pDataPool, int iFilePid)
{
    char *pSend = NULL;
    int iRet = 0;

    if(pDataPool == NULL || iFilePid < 0)
    {
        return -1;
    }
    pSend = pDataPool + iFilePid*MAX_SPI_PACKET_LEN;
    iRet = spi_p2psdstream_send(pSend, MAX_SPI_PACKET_LEN);
    if(iRet != MAX_SPI_PACKET_LEN)
    {
        ak_print_error_ex("Send error! iRet: %d\r\n", iRet);
        iRet = -1;
    }
    else
    {
        iRet = 0;
    }
    return iRet;
}

int p2p_process_ack(char *pAck, int len)
{
    int iSmall = 0;
    int i = 0;

    iSmall = ( *(pAck + 0) & 0xFF) << 24;
    iSmall |= ( *(pAck + 1) & 0xFF) << 16;
    iSmall |= ( *(pAck + 2) & 0xFF) << 8;
    iSmall |= ( *(pAck + 3) & 0xFF) << 0;

    for(i = 0; i < len; i++)
    {
        if( *(pAck + 4 + i) == 0 )
        {
            break;
        }
    }
    return (i + iSmall);
}


static int p2p_ack_clear_buffer(void)
{
    char *pACK = NULL;
    pACK = (char *)malloc(sizeof(char)*MAX_SPI_PACKET_LEN);
    
    if(pACK == NULL)
    {
        return -1;
    }

    while(spi_p2psdstream_receive(pACK, MAX_SPI_PACKET_LEN) == MAX_SPI_PACKET_LEN)
    {
        // do nothing
    }
    return 0;
}

static int p2p_process_ack_many_times(MNT_PID_ACK_t *pMnt, int iNumPid, int *piSmallPidToSend)
{
    int iRet = MAX_SPI_PACKET_LEN;
    int i = 0;
    char *p = NULL;
    int iLenAck = 0;
    int iSmallPidAck = 0;
    int iEnoughAck = 0;
    char *pACK = NULL;
    char *pACKBK = NULL;
    int first_find_pid_nak = 0;
    int iHaveAck = 0;
    int cntlog = 0;
    
    pACK = (char *)malloc(sizeof(char)*MAX_SPI_PACKET_LEN);
    pACKBK = (char *)malloc(sizeof(char)*MAX_SPI_PACKET_LEN);
    if(pACK == NULL || pMnt == NULL || pACKBK == NULL || piSmallPidToSend == NULL)
    {
        if(pACK) free(pACK);
        if(pACKBK) free(pACKBK);
        return -1;
    }

    // read ack many times and get final packet ack
    while(iRet == MAX_SPI_PACKET_LEN)
    {
        iRet = spi_p2psdstream_receive(pACK, MAX_SPI_PACKET_LEN);
        if(iRet == MAX_SPI_PACKET_LEN){
            memcpy(pACKBK, pACK, MAX_SPI_PACKET_LEN);
            iHaveAck = 1;
        }
    }

    if(iHaveAck == 0)
    {
        printf(".\r\n");
        iRet = 0;
    }
    else
    {
        // parse smallest PID and length data
        iLenAck = ( *(pACKBK + 1) & 0xFF) << 8;
        iLenAck |= ( *(pACKBK + 2) & 0xFF) << 0;
        iSmallPidAck = ( *(pACKBK + 3) & 0xFF) << 24;
        iSmallPidAck |= ( *(pACKBK + 4) & 0xFF) << 16;
        iSmallPidAck |= ( *(pACKBK + 5) & 0xFF) << 8;
        iSmallPidAck |= ( *(pACKBK + 6) & 0xFF) << 0;
        // printf("===%s iSmallPidAck(len %d): %d/%d===\r\n", __func__, iLenAck, iSmallPidAck, iNumPid);
        printf("===(len %d) ACK %d/%d===\r\n", iLenAck, iSmallPidAck, iNumPid);
        //set pid from 0 to iSmallPidAck is 1, default app received to here
        if(iSmallPidAck <= iNumPid){ // must be <=.
            for(i = 0; i < iSmallPidAck; i++)
            {
                if( (pMnt+i)->ack == 0){
                    (pMnt+i)->ack = 1;
                }
            }
        }
        *piSmallPidToSend = iSmallPidAck;
        if(iSmallPidAck == iNumPid)
        {
            // nothing to do, return
        }
        else
        {
            p = pACKBK + 7;
            for(i = 0; i < (iLenAck - 4); i++)
            {
                if((iSmallPidAck + i) < iNumPid){
                    if(*(p + i) == 1) //ACK
                    {
                        // avoid exceeding boundary of pointer pMnt
                        // printf("A%d ", iSmallPidAck + i);
                        (pMnt+iSmallPidAck+i)->ack = 1;
                        // cntlog++;
                        // if(cntlog == 15)
                        // {
                        //     cntlog = 0;
                        //     printf("\r\n");
                        // }
                    }
                    else // NAK
                    {
                        // printf("PidNak%d\n", iSmallPidAck + i);
                        if(first_find_pid_nak == 0)
                        {
                            *piSmallPidToSend += i;
                            first_find_pid_nak = 1;
                            printf("SmallACK_calc: %d, iSmallPidAck:%d, i:%d\r\n", *piSmallPidToSend, iSmallPidAck, i);
                        }
                    }
                }
                else{
                    iEnoughAck = 1;
                    // printf("-------------Received enough ACK-------------\r\n");
                    break;
                }
            }
            
            // if(iEnoughAck)
            // {
            //     // read all buffer to fflush it
            //     while(spi_p2psdstream_receive(pACK, MAX_SPI_PACKET_LEN) == MAX_SPI_PACKET_LEN)
            //     {
            //         // do nothing
            //     }
            // }

            iRet = *piSmallPidToSend;
        }//if(iSmallPidAck == iNumPid)
    }

    p2p_ack_clear_buffer();

    free(pACK);
    free(pACKBK);
    return iRet;
}



int p2p_sd_streaming_main_new(void)
{
    int iRet = 0;
    char *pDataSend = NULL;
    char *pSendCurrent = NULL;
    char *pAck = NULL;
    int iLenAck = 0;
    int iLenSend = 0;
    int cntlog = 0;
    int iFilePIDACK = 0;

    int iLengthToSend = 0;
    int iCountLoad = 0;
    int iNumberOfPid = 0;
    int iCurrPidDoesnotACK = 0;
    int iPidSend = 0;
    MNT_PID_ACK_t *pMntPidAck = NULL, *pMnt = NULL;
    int i = 0;
    int iCntSendLoad = 0;
    unsigned long ts_curr = 0;
    unsigned long timeoutack = 0;
    unsigned long ulTOSmallPacket = 0;
    int iSmallPidSend = 0;

    int iSendSize = 0;
    void *p2p_resend_handle = NULL;

    pDataSend = (char *)malloc(sizeof(char)*1024*1024);
    if(pDataSend == NULL)
    {
        ak_print_error_ex("Cannot malloc data for pDataSend\r\n");
        return -1;
    }

    /* prepare data and wrapper ps */
    iRet = p2p_prepare_data_and_wrapper_ps(pDataSend, &iLengthToSend);
    ak_print_error_ex("prepare_data (%d) iLengthToSend: %d\r\n", iRet, iLengthToSend);
    if (iRet < 0)
    {
        goto pDataSend_free;
    }

    iNumberOfPid = p2p_calc_pid_number(iLengthToSend);
    
    pMntPidAck = (MNT_PID_ACK_t *)malloc(sizeof(MNT_PID_ACK_t) * iNumberOfPid);
    if(pMntPidAck == NULL)
    {
        ak_print_error_ex("Cannot malloc buffer for pMntPidAck\r\n");
        //return -1;
        goto pDataSend_free;
    }
    for(i = 0; i < iNumberOfPid; i++)
    {
        pMnt = pMntPidAck + i;
        pMnt->data = pDataSend + i*MAX_SPI_PACKET_LEN;
        pMnt->pid = i;
        pMnt->t2s = 0;///get_tick_count();
        pMnt->ack = 0;
    }

    iCountLoad = 0;
    iCurrPidDoesnotACK = 0; // pid current which doesn't have ACK
    
    for(;;)
    {
        /*-------------------------------------------------------------------------*/
        /*-------------------------------Send data---------------------------------*/
        // iPidSend = iCurrPidDoesnotACK;
        /* get new packet with wanna PID 200ms */
        /* which packet will be sent? 
            1. no ack
            2. timeout 1.3s
        */
        while(iCountLoad < 8)
        {
            ts_curr = get_tick_count();
            // check ack before send it
            pMnt = pMntPidAck + iPidSend;
            if(pMnt->ack == 0)
            {
                // check timeout or time to send before
                /*
                    t2s == 0 -> it must be sent first
                    t2s != 0 and > 1.3 it must be re-sent
                */
                if(pMnt->t2s == 0) // send first
                {
                    iRet = p2p_get_new_dataload_and_send(pDataSend, iPidSend);
                    if(iRet == 0)
                    {
                        printf("%d ", iPidSend);
                        if(iPidSend == iNumberOfPid)
                        {
                            printf("=======End of file======================\r\n");
                            break;
                        }
                        iCountLoad++;
                        pMnt = pMntPidAck + iPidSend;
                        pMnt->t2s = get_tick_count();
                        // iPidSend++;
                    }
                    ak_sleep_ms(15);
                }
                else // resend, need to check timeout 1300 ms
                {
                    ts_curr = ts_curr - pMnt->t2s;
                    if(ts_curr >= P2P_SD_STREAM_PACKET_MAX_TIMEOUT)
                    {
                        iRet = p2p_get_new_dataload_and_send(pDataSend, iPidSend);
                        if(iRet == 0)
                        {
                            printf("%d(%u) ", iPidSend, ts_curr);
                            if(iPidSend == (iNumberOfPid - 1))
                            {
                                printf("====End of file iPidSend:%d iNumberOfPid:%d\r\n", iPidSend, iNumberOfPid);
                                break;
                            }
                            iCountLoad++;
                            pMnt = pMntPidAck + iPidSend;
                            pMnt->t2s = get_tick_count();
                            // iPidSend++;
                        }
                        ak_sleep_ms(15);
                    }
                    // else{
                    //     iPidSend++;
                    // }
                }
            }

            iPidSend++;
            if(iPidSend >= iNumberOfPid)
            {
                // when should task send dummy? There is no packet to send
                if(iCountLoad == 0)
                {
                    printf("+++ "); // send summy to get ACK
                    p2p_send_dummy();
                    cntlog++;
                    if(cntlog == 15)
                    {
                        cntlog = 0;
                        //printf("\r\n");
                    }
                    iCountLoad++;
                }
                // printf("---->>Loop begin send from smallpid: %d\r\n", iSmallPidSend);
                iPidSend = iSmallPidSend;
                break;
            }
            // ak_sleep_ms(5);
        } //End while

        iCountLoad = 0;
        
        /*-------------------------------------------------------------------------*/
        /*-------------------------------Process ACK-------------------------------*/
        iRet = p2p_process_ack_many_times(pMntPidAck, iNumberOfPid, &iSmallPidSend);
        if(iRet > 0)
        {
            if(iSmallPidSend < iNumberOfPid ){
                iPidSend = iSmallPidSend;
                printf("set iPidSend =%d\r\n", iPidSend);
            }
            else{
                //send ok
                printf("Done send last PID\r\n");
                g_iP2pSdStreamStart = 0;
            }
            timeoutack = get_tick_count();
            // printf("TimeAck:%u iSmallPidSend:%d\r\n", timeoutack, iSmallPidSend);
        }
        else
        {
            if((get_tick_count() - timeoutack) >= P2P_SD_STREAM_ACK_MAX_TIMEOUT && (timeoutack != 0) )
            {
                g_iP2pSdStreamStart = 0;
                printf("Time out ACK: %u\r\n", get_tick_count() - timeoutack);
            }
            else
            {
                // printf(">>>iSmallPidSend:%d\r\n", iSmallPidSend);
            }
            if(iSmallPidSend == iNumberOfPid)
            {
                printf("last PID %d = %d\r\n", iSmallPidSend, iNumberOfPid);
                g_iP2pSdStreamStart = 0;
            }
        }

        /* check timeout the smalest PID packet */
        pMnt = pMntPidAck + iSmallPidSend;
        ulTOSmallPacket = pMnt->t2s;
        if((get_tick_count() - ulTOSmallPacket) >= P2P_SD_STREAM_PACKET_MAX_TIMEOUT)
        {
            iPidSend = iSmallPidSend;
            printf("Time out small Pid, need to send from %d\r\n", iPidSend);
        }


        if(g_iP2pSdStreamStart == 0)
        {
            printf("STOP transfer!\r\n");
            p2p_send_done_uploading();
            break;
        }

        ak_sleep_ms(10);
    }
    //end for loop

    printf("+++++++++++++++++++P2p sd stream exit+++++++++++++++++++++++++++\r\n");
    p2p_send_done_uploading();

    //free memory-no error
    if(pMntPidAck)
    {
        free(pMntPidAck);
    }
    
    if(pDataSend)
    {
        free(pDataSend);
    }
    
    return iRet;//exit the function here when no error

    /* -------------------------------------------------------------------------*/
    //free memory-when error

pDataSend_free:
    if(pDataSend)
    {
        free(pDataSend);
    }

    ak_print_error_ex("p2p_sd_streaming fail! \r\n");
    return -1; //exit the function here when there is any error
}

void *p2p_sdcard_streaming_task(void *arg)
{
    ENUM_SYS_MODE_t sys_get_mode;
    int iRet = 0;
    int iCount1 = 0;

    for(;;)
    {
        /* wait here */
        // ak_thread_sem_wait(&sem_SpiCmdStartTransfer);
        while(g_iP2pSdStreamStart == 0)
        {
            ak_sleep_ms(100);
        }
        if(p2p_ack_clear_buffer() != 0)
        {
            ak_print_error_ex("Clear buffer ACK failed! try again!\r\n");
            p2p_ack_clear_buffer();
        }

        ak_print_normal_ex(" \n ------------------ START main thread 's processing here------------ \r\n");
        //Thuan edit - 16/10/2017
        iRet = p2p_sd_streaming_main_new();

        g_iP2pSdStreamStart = 0;

        ak_sleep_ms(10);
        if(E_SYS_MODE_FTEST == sys_get_mode || \
            E_SYS_MODE_FWUPGRADE == sys_get_mode)
        {
            ak_print_normal_ex("Break 2 exit\r\n");
            break;
        }
    }
    ak_thread_exit();
    return NULL;
}


/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/
