/**
  ******************************************************************************
  * File Name          : upload_over_spi.c
  * Description        : 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/*---------------------------------------------------------------------------*/
/*                           INCLUDED FILES                                  */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "ak_apps_config.h"
#include "ringbuffer.h"
#include "spi_transfer.h"
#include "list.h"
#include "spi_cmd.h"
#include "ak_ini.h"
#include "upload_over_spi.h"
#include "upload_main.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/
#define USEND_PARSE_GET_POSTION(p, o)    ( ((*(p+o) & 0x000000FF) << 24) |\
                                            ((*(p+o+1) & 0x000000FF) << 16) |\
                                            ((*(p+o+2) & 0x000000FF) << 8) |\
                                            ((*(p+o+3) & 0x000000FF) << 0) )

#define USEND_PARSE_GET_LENDATA(p, o)    ( ((*(p+o) & 0x00FF) << 8) |\
                                            ((*(p+o+1) & 0x00FF) << 0) )

#define NUMSEND_ONETIME            HTTP_NUMBER_OF_KB
/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                   PUBLIC API                                              */
/*---------------------------------------------------------------------------*/
int g_iPosition = 0;
static int g_first_recv_call = 0;

typedef enum upload_state_simple
{
    E_UPLOAD_STATE_SIMPLE_INIT = 0,
    E_UPLOAD_STATE_SIMPLE_OPEN,
    E_UPLOAD_STATE_SIMPLE_SEND,
    E_UPLOAD_STATE_SIMPLE_RECV,
    E_UPLOAD_STATE_SIMPLE_CLOSE,
    E_UPLOAD_STATE_SIMPLE_MAX
}UPLOAD_STATE_SIMPLE_t;


static UPLOAD_STATE_SIMPLE_t g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_INIT;


static int dump_data_upload(char *pData, int length)
{
    int i = 0;
    char acDebug[1024];
    memset(acDebug, 0x00, 1024);
    for(i = 0; i < length; i++)
    {
        sprintf(acDebug + strlen(acDebug), "%02X ", *(pData + i));
    }
    // printf("%s\r\n", acDebug);
}

static int uploadsend_prepare_packet(char *pMsg, char *pData,
    unsigned short usLen, int iPos)
{
    int iRet = 0;
    if(pMsg == NULL || pData == NULL || g_iPosition < 0 || usLen < 0)
    {
        return -1;
    }

    *(pMsg + UPLOAD_LEN_BYTE_OFFSET) = (char)((usLen & 0xFF00) >> 8);
    *(pMsg + UPLOAD_LEN_BYTE_OFFSET + 1) = (char)(usLen & 0x00FF);

    /* position */
    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET) = (char)((iPos & 0xFF000000) >> 24);
    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 1) = (char)((iPos & 0x00FF0000) >> 16);
    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 2) = (char)((iPos & 0x0000FF00) >> 8);
    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 3) = (char)(iPos & 0x000000FF);

    memcpy(pMsg + UPLOAD_DATA_BYTE_OFFSET, pData + iPos, usLen - 4);
    return iRet;
}

// process postion
static int upload_update_position_resp(char *pResp, int iPos)
{
    int iRet = 0;
    if(pResp == NULL)
    {
        return -1;
    }
    memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
    *(pResp + 0) = SPI_CMD_MOTION_UPLOAD;
    *(pResp + UPLOAD_CTRL_BYTE_OFFSET) = UPLOAD_CTRL_DATA_ERROR;
    // byte position
    *(pResp + UPLOAD_RESPONSE_POSITION_BYTE_OFFSET) = (char)((iPos & 0xFF000000) >> 24);
    *(pResp + UPLOAD_RESPONSE_POSITION_BYTE_OFFSET + 1) = (char)((iPos & 0x00FF0000) >> 16);
    *(pResp + UPLOAD_RESPONSE_POSITION_BYTE_OFFSET + 2) = (char)((iPos & 0x0000FF00) >> 8);
    *(pResp + UPLOAD_RESPONSE_POSITION_BYTE_OFFSET + 3) = (char)(iPos & 0x000000FF);
    // ak_print_normal("%s iPos: %d 0x%08x\n", __func__, iPos, iPos);
    // dump_data_upload(pResp, 8);
    return iRet;
}


/*
* Brief: this function will check position from CC3200.
* This position will indicate the package which CC3200 
* need to receive.
* Return: < 0 if there is any errors
*        >= 0 position which CC3200 wanna receive.
*/
static int uploadsend_check_response(void)
{
    int iRet = 0;
    int iPosResp = 0;
    int iLen = 0;
    int i = 0;
    char *pResp = (char *)malloc(sizeof(char)*MAX_SPI_PACKET_LEN);
    if(pResp == NULL)
    {
        ak_print_error_ex("Cannot malloc for pResp -  uploader response buffer \r\n");
        return -1;
    }
    memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
    iRet = spi_uploader_receive(pResp, MAX_SPI_PACKET_LEN);
    // printf("%s iRet = %d ", __func__, iRet);
    if(iRet <= 0 || (*(pResp+1)!= UPLOAD_CTRL_DATA_ERROR))
    {
        iRet = -1;
    }
    else
    {
        iPosResp = ((*(pResp + 2) & 0x000000FF) << 24);
        iPosResp |= ((*(pResp + 3) & 0x000000FF) << 16);
        iPosResp |= ((*(pResp + 4) & 0x000000FF) << 8);
        iPosResp |= ((*(pResp + 5) & 0x000000FF) << 0);
        // ak_print_error_ex("iPosResp: 0x%08x (%d)\r\n", iPosResp, iPosResp);
        // printf("Check resp ");
        // dump_data_upload(pResp, 12);h
        iRet = iPosResp;
    }
    free(pResp);
    return iRet;
}


static int upload_send_one_packet(char *pSend, unsigned long *pT2s, int timeout)
{
    int iRet = 0;
    unsigned short usLenData = 0;
    int iPosSend = 0;
    int iPosWannaRecv = 0;
    unsigned long ulTimeStart = 0;
    char cCount = 0;
    // unsigned char ucDelayMs = 10;
    if((pSend == NULL) || (pT2s == NULL))
    {
        return -1;
    }

    iPosSend = USEND_PARSE_GET_POSTION(pSend, UPLOAD_POSITION_BYTE_OFFSET);
    usLenData = USEND_PARSE_GET_LENDATA(pSend, UPLOAD_LEN_BYTE_OFFSET) - 4;
    ulTimeStart = get_tick_count();
    iRet = spi_uploader_send(pSend, MAX_SPI_PACKET_LEN);
    if(usLenData == 0)
    {
        return 0; // no more data to send
    }
    if(iRet <= 0)
    {
        return -1;
    }

    for(;;)
    {
        /* check response */
        ak_sleep_ms(20);
        iPosWannaRecv = uploadsend_check_response();
        if(iPosWannaRecv >= 0)
        {
            if(iPosWannaRecv == (iPosSend + usLenData))
            {
                iRet = usLenData;
                *pT2s = get_tick_count() - ulTimeStart;
                break;
            }
        }
        cCount++;
        /* resend every 200ms */
        if(cCount % 10 == 0)
        {
            iRet = spi_uploader_send(pSend, MAX_SPI_PACKET_LEN);
        }
        /* TimeOut Max 50x20 = 1 second */
        if(cCount >= 50)
        {
            iRet = -1;
            break;
        }
    }
    return iRet;
}

/*
* Open and bind socket
*/
// int socket_api(int domain, int type, int protocol)
int upload_socket_api(char *hostname, char *port, int protocol)
{
    // return socket(domain, type, protocol);
    int iRet = 0;
    char *pMsg = NULL;
    char *pRecvMsg = NULL;
    char *pW = NULL;
    unsigned short usLen = 0;
    unsigned char ucSsl = 0;
    unsigned short usPort = 0;
    int iTimeout = 50;

    // if(g_enum_state_simple != E_UPLOAD_STATE_SIMPLE_INIT)
    // {
    //     iRet = -1;
    //     g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_INIT;
    //     printf("%s wrong state!\r\n", __func__);
    //     return -1;
    // }
    printf("%s hostname: %s, port: %s, protocol: %d\r\n", __func__, hostname, port, protocol);


    /* Check parameter */
    if((hostname == NULL) || (port == NULL))
    {
        printf("hostname or port is null!\r\n");
        iRet = -1;
    }
    else
    {
        usPort = atoi(port);
        if(usPort <= 0)
        {
            printf("Cannot change port to integer!\r\n");
            iRet = -1;
        }
    }

    if((protocol != UPLOAD_PROTOCOL_HTTP) && \
        (protocol != UPLOAD_PROTOCOL_HTTPS))
    {
        printf("protocol not support!\r\n");
        iRet = -1;
    }
    else
    {
        if(protocol == UPLOAD_PROTOCOL_HTTP){
            ucSsl = UPLOAD_PROTOCOL_HTTP;
        }
        else if(protocol == UPLOAD_PROTOCOL_HTTPS){
            ucSsl = UPLOAD_PROTOCOL_HTTPS;
        }
        else{
            printf("BIG BUG, WRONG LOGIC CHECK PROTOCOL!!!");
            iRet = -1;
        }
    }

    /* Main process */
    if(iRet == 0)
    {
        // send command SPI to open socket
        pMsg = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        if(pMsg == NULL)
        {
            printf("Cannot malloc pMsg to open socket!\r\n");
            iRet = -2;
        }
        else
        {
            memset(pMsg, 0x00, MAX_SPI_PACKET_LEN);
            /* byte type */
            *(pMsg + 0) = SPI_CMD_MOTION_UPLOAD;
            /* byte control */
            *(pMsg + UPLOAD_CTRL_BYTE_OFFSET) = UPLOAD_CTRL_START_SOCKET;
            /* byte protocol ssl */
            *(pMsg + UPLOAD_SSL_BYTE_OFFSET) = ucSsl;
            /* byte port */
            *(pMsg + UPLOAD_PORT_BYTE_OFFSET) = (char)((usPort & 0xFF00) >> 8);
            *(pMsg + UPLOAD_PORT_BYTE_OFFSET + 1) = (char)(usPort & 0x00FF);
            /* url */
            pW = pMsg + UPLOAD_URL_BYTE_OFFSET;
            memcpy(pW, hostname, strlen(hostname));
            /* byte length, will update in last */
            usLen = UPLOAD_URL_BYTE_OFFSET;
            usLen += strlen(hostname);
            *(pMsg + UPLOAD_LEN_BYTE_OFFSET) = (char)((usLen & 0xFF00) >> 8);
            *(pMsg + UPLOAD_LEN_BYTE_OFFSET + 1) = (char)(usLen & 0x00FF);

            printf("%s Type: 0x%x, control: 0x%x, Ssl: 0x%x, Len: 0x%x\r\n", \
                                    __func__, *(pMsg + 0), \
                                    *(pMsg + UPLOAD_CTRL_BYTE_OFFSET), \
                                    ucSsl, usLen);
            /* send data over spi */
            iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
            if(iRet < 0)
            {
                printf("%s Maybe ringbuffer is full! retry to send after 100ms\r\n", __func__);
                // maybe retry send
                ak_sleep_ms(100);
                iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
                if(iRet != 0)
                {
                    printf("Retry failed!\r\n");
                }
            }
            else
            {
                printf("open socket sent %d bytes\r\n", iRet);
                iRet = 0;
            }

            #if 0
            // after sending successfull
            if(iRet == 0)
            {
                pRecvMsg = (char *)malloc(sizeof(char)*MAX_SPI_PACKET_LEN);
                if(pRecvMsg == NULL)
                {
                    printf("%s pRecvMsg is NULL, return.\r\n", __func__);
                    iRet = -3;
                }
                else
                {
                    while(iTimeout)
                    {
                        iRet = spi_uploader_receive(pRecvMsg, MAX_SPI_PACKET_LEN);
                        if((iRet > 0) && \
                                (*(pRecvMsg + UPLOAD_CTRL_BYTE_OFFSET) == UPLOAD_CTRL_SOCKET_READY))
                        {
                            break;
                        }
                        iTimeout--;
                        ak_sleep_ms(100);
                    }
                    if(iTimeout == 0)
                    {
                        printf("%s timeout\r\n", __func__);
                        iRet = -4;
                    }
                    else
                    {
                        if(*(pRecvMsg + UPLOAD_CTRL_BYTE_OFFSET) == UPLOAD_CTRL_SOCKET_READY)
                        {
                            printf("%s socket ready\r\n", __func__);
                            iRet = 0;
                            g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_SEND;
                            g_iPosition = 0;
                        }
                        else
                        {
                            printf("%s open socket FAILED dump log:", __func__);
                            dump_data_upload(pRecvMsg, 12);
                            iRet = -4;
                        }
                    }
                    free(pRecvMsg);
                }
            }
            #endif
            free(pMsg);
        }
    }
    if(iRet >= 0)
    {
        printf("%s return ret: %d, g_iPosition: 0x%x\r\n", __func__, iRet, g_iPosition);
        iRet = 1;
    }
    return iRet;
}
	
static int iCntUploadCallSend = 0;

//Thuan edited - 30/10/2017
#if 0  //using the original version_author Trung Tien, every 1_KB

#if 1
ssize_t upload_send_api(int sockfd, const void *buf, size_t len, int flags)
{
    int iRet = 0;
    int i = 0;
    char *pMsg = NULL;
    char *pRecv = NULL;
    unsigned short usLen = 0;
    unsigned int uiNumOfSend = 0;
    int iPosCCWanna = 0;
    int iTimeout = 0;
    int iRetry = 5;
    int iCurrentPosition = 0;
    unsigned long ulCurrentTick = 0;


    // ak_print_normal_ex("Send len %u, state %d, pos %d\r\n", len, g_enum_state_simple, g_iPosition);
    ulCurrentTick = get_tick_count();    
    printf("---send %d time start %u---------------------\n", len, ulCurrentTick);
    if(buf == NULL)
    {
        printf("%s buffer is null", __func__);
        iRet = -1;
    }

    iCntUploadCallSend++;
    /* Main process */
    if(iRet == 0)
    {
        // send command SPI to open socket
        pMsg = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        memset(pMsg, 0x00, MAX_SPI_PACKET_LEN);
        // pRecv = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        if(pMsg == NULL)
        {
            printf("Cannot malloc pMsg or pRecv to send socket!\r\n");
            iRet = -2;
        }
        else
        {
            /* byte type */
            *(pMsg + 0) = SPI_CMD_MOTION_UPLOAD;
            /* byte control */
            *(pMsg + UPLOAD_CTRL_BYTE_OFFSET) = UPLOAD_CTRL_UPLOADING;
            /* if length of data exceed size of packet spi, we must chop it */            
            uiNumOfSend = len/(MAX_SPI_PACKET_LEN-10);
            if(len % (MAX_SPI_PACKET_LEN-10)){
                uiNumOfSend++;
            }
            /* process upgrading done*/
            if(len == 0){
                uiNumOfSend = 1;
            }
            iCurrentPosition = 0;
            for(i = 0; i < uiNumOfSend; i++)
            {
                if(i == (uiNumOfSend - 1))
                {
                    usLen = len + 4;
                }
                else
                {
                    usLen = MAX_SPI_PACKET_LEN - 10 + 4;
                }
                // iRet = uploadsend_prepare_packet(pMsg, buf, usLen, g_iPosition);

                *(pMsg + UPLOAD_LEN_BYTE_OFFSET) = (char)((usLen & 0xFF00) >> 8);
                *(pMsg + UPLOAD_LEN_BYTE_OFFSET + 1) = (char)(usLen & 0x00FF);
                
                /* position */
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET) = (char)((g_iPosition & 0xFF000000) >> 24);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 1) = (char)((g_iPosition & 0x00FF0000) >> 16);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 2) = (char)((g_iPosition & 0x0000FF00) >> 8);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 3) = (char)(g_iPosition & 0x000000FF);
                memcpy(pMsg + UPLOAD_DATA_BYTE_OFFSET, buf + iCurrentPosition, usLen - 4);

                /* send data over spi */
                iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
                // printf("%s [%d] i:%d, usLen: %u, NumberOfSend: %u, (g_pos:%d) (iCurrentPosition: %d) Dump:", \
                //         __func__, iCntUploadCallSend, i, usLen, uiNumOfSend, g_iPosition, iCurrentPosition);
                dump_data_upload(pMsg, 12);
                if(iRet <= 0)
                {
                    printf("%s Maybe ringbuffer is full!\r\n", __func__);
                    i--;
                    iRetry--;
                }
                else
                {
                    // g_iPosition += (usLen - 4);
                    /* len ==0; break, no need check response */
                    if(len == 0) // upgrading done
                    {
                        g_iPosition = 0;
                        g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_RECV;
                        ak_print_normal_ex("Upgrading done, change to state E_UPLOAD_STATE_SIMPLE_RECV (iCntUploadCallSend: %d)\r\n", iCntUploadCallSend);
                        iCntUploadCallSend = 0;
                        printf("=================================================\r\n");
                        break;
                    }
                    // ak_sleep_ms(10); // wait CC confirm
                    /* Check response from CC */
                    while(1)
                    {
                        iTimeout++;
                        ak_sleep_ms(10);
                        iPosCCWanna = uploadsend_check_response();
                        if(iPosCCWanna < 0)
                        {
                            if(iTimeout % 10 == 0)
                            {
                                // try to resend
                                ak_print_normal_ex("%s resend (time out: %d)\r\n", __func__, iTimeout);
                                iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
                            }
                        }
                        else
                        {
                            if(iPosCCWanna == (g_iPosition + (usLen - 4))) // CC3200 was received OK
                            {
                                iCurrentPosition = iPosCCWanna;
                                g_iPosition = iPosCCWanna;
                                iRet = len;
                                printf("++++++CC sent %d-%d time %u OK++++++\r\n", \
                                    iPosCCWanna, g_iPosition, get_tick_count() - ulCurrentTick);
                                break; // while
                            }
                            else // < g_iPosition
                            {
                                if(iTimeout % 30 == 0) // resend each ~300ms
                                {
                                    iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
                                }
                                ak_print_error_ex("resend pos (%d, 0x%x) != g_iPosition + ulen -4 (%d + %d)\r\n", iPosCCWanna, iPosCCWanna, g_iPosition, usLen);
                            }
                        } // if(iPosCCWanna < 0)
                        // process timeout
                        if(iTimeout == 100)
                        {
                            iRet = -4;
                            ak_print_error_ex("Timeout upload send data! g_iPosition: %d, iPosCCWanna: %d\r\n", g_iPosition, iPosCCWanna);
                            break; // while
                        }
                    } // while 1
                }
                if(iRetry == 0)
                {
                    ak_print_normal_ex("Send failed, retry == 0\r\n");
                    iRet = -3;
                    break; // for
                }
            }// for
        }
        /* End of process send api */
        if(pMsg)
        {
            free(pMsg);
        }
        if(pRecv)
        {
            free(pRecv);
        }
    }
    // ulCurrentTick = get_tick_count();
    return iRet;
}
#else
// new code, make it simple
ssize_t upload_send_api(int sockfd, const void *buf, size_t len, int flags)
{
    int iRet = 0;
    int i = 0;
    char *pMsg = NULL;
    char *pRecv = NULL;
    unsigned short usLen = 0;
    unsigned int uiNumOfSend = 0;
    int iPosCCWanna = 0;
    int iTimeout = 0;
    int iRetry = 5;
    int iCurrentPosition = 0;
    unsigned long ulCurrentTick = 0;
    unsigned long ulTimeToSend = 0;
    int iSendSize = 0;

    ulCurrentTick = get_tick_count();    
    printf("---App send %d time start %u---------------------\n", len, ulCurrentTick);
    if(buf == NULL)
    {
        printf("%s buffer is null", __func__);
        iRet = -1;
    }
    /* Main process */
    if(iRet == 0)
    {
        // send command SPI to open socket
        pMsg = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        memset(pMsg, 0x00, MAX_SPI_PACKET_LEN);
        if(pMsg == NULL)
        {
            printf("Cannot malloc pMsg to send socket!\r\n");
            iRet = -2;
        }
        else
        {
            /* if length of data exceed size of packet spi, we must chop it */            
            uiNumOfSend = len/(MAX_SPI_PACKET_LEN-10);
            if(len % (MAX_SPI_PACKET_LEN-10))
            {
                uiNumOfSend++;
            }
            /* byte type and control */
            *(pMsg + 0) = SPI_CMD_MOTION_UPLOAD;
            *(pMsg + UPLOAD_CTRL_BYTE_OFFSET) = UPLOAD_CTRL_UPLOADING;
            iCurrentPosition = 0;
            for(i = 0; i < uiNumOfSend; i++)
            {
                if(i == (uiNumOfSend-1))
                {
                    usLen = len + 4; // 4 byte position
                }
                else
                {
                    usLen = i*(MAX_SPI_PACKET_LEN-10) + 4;
                }
                /* update position and len */
                *(pMsg + UPLOAD_LEN_BYTE_OFFSET) = (char)((usLen & 0xFF00) >> 8);
                *(pMsg + UPLOAD_LEN_BYTE_OFFSET + 1) = (char)(usLen & 0x00FF);
                /* position */
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET) = (char)((g_iPosition & 0xFF000000) >> 24);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 1) = (char)((g_iPosition & 0x00FF0000) >> 16);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 2) = (char)((g_iPosition & 0x0000FF00) >> 8);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 3) = (char)(g_iPosition & 0x000000FF);
                memcpy(pMsg + UPLOAD_DATA_BYTE_OFFSET, buf + iCurrentPosition, usLen - 4);

                /* send data */
                iRet = upload_send_one_packet(pMsg, &ulTimeToSend, 0);
                if(iRet < 0) // ringbuffer is full, need to resend
                {
                    iRet = 0; // return 0 and break;
                    break;
                }
                else // send ok, receive response OK
                {
                    g_iPosition += (usLen - 4); // update position
                    iCurrentPosition += (usLen - 4); // update offset data to copy
                    iRet = iCurrentPosition;
                    printf("--------Send OK pos %i (0x%x) time:%u------------\r\n", g_iPosition, g_iPosition, ulTimeToSend);
                    if(iRet == 0)
                    {
                        g_iPosition = 0;
                    }
                }
            }// end of for(i = 0; i < uiNumOfSend; i++)
        } // end of if(pMsg == NULL)
        /* End of process send api */
        if(pMsg)
        {
            free(pMsg);
        }
    }
    return iRet;
}
#endif

#else //using the every_N_KB version, author Thuan

//----------------------------------------------------Thuan add - 30/10/2017 ----------------------------------------------------------

ssize_t upload_send_api(int sockfd, const void *buf, size_t len, int flags)
{
    int iRet = 0;
    int j, k;
    char *pMsg = NULL;
    char *pRecv = NULL;
    unsigned short usLen = 0;
    unsigned int uiNumOfSend = 0;
    int iPosCCWanna = 0;
    int iCurrentPosition = 0;
    int g_iPosition_temp = 0;
    int temp;
    int ack_found=0;


    unsigned long ulCurrentTick = 0;

    ulCurrentTick = get_tick_count();    
    //printf("\n --------------------------------send %d time start %u---------------------\n", len, ulCurrentTick);
    if(buf == NULL)
    {
        printf("%s buffer is null", __func__);
        iRet = -1;
    }
    ak_print_normal_ex("g_iPosition: %d \r\n", g_iPosition);
    iCntUploadCallSend++;
    /* Main process */
    if(iRet == 0)
    {
        // send command SPI to open socket
        //pMsg = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        char pMsg[MAX_SPI_PACKET_LEN];
        memset(pMsg, 0x00, MAX_SPI_PACKET_LEN);
        //pRecv = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        if(pMsg == NULL)
        {
            printf("Cannot malloc pMsg or pRecv to send socket!\r\n");
            iRet = -2;
        }
        else
        {
            /* byte type */
            *(pMsg + 0) = SPI_CMD_MOTION_UPLOAD;
            /* byte control */
            *(pMsg + UPLOAD_CTRL_BYTE_OFFSET) = UPLOAD_CTRL_UPLOADING;
            // Starting from a global position
            g_iPosition_temp = g_iPosition;
            if(len == 0){ // This is to send zero length packet (last packet of uploading)
                usLen = 0;
                *(pMsg + UPLOAD_LEN_BYTE_OFFSET) = (char)(((usLen+4) & 0xFF00) >> 8);
                *(pMsg + UPLOAD_LEN_BYTE_OFFSET + 1) = (char)((usLen+4) & 0x00FF);
                iCurrentPosition = g_iPosition_temp;
                /* position */
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET) = (char)((iCurrentPosition & 0xFF000000) >> 24);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 1) = (char)((iCurrentPosition & 0x00FF0000) >> 16);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 2) = (char)((iCurrentPosition & 0x0000FF00) >> 8);
                *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 3) = (char)(iCurrentPosition & 0x000000FF);
                /* send data over spi */
                iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
                
                g_iPosition = 0;
                g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_RECV;
                ak_print_normal_ex("Upgrading done, change to state E_UPLOAD_STATE_SIMPLE_RECV (iCntUploadCallSend: %d)\r\n", iCntUploadCallSend);
                iCntUploadCallSend = 0;
                //flagg == false;
                printf("=================================================\r\n");
            }

            ulCurrentTick = get_tick_count();
            // This is to send non-zero length data
            while(((get_tick_count() - ulCurrentTick)<(20*1000)) && (len != 0))
            {
                // Compute number of send for this time
                uiNumOfSend = len - (g_iPosition_temp - g_iPosition);
                if (uiNumOfSend%SPI_UPLOAD_RAW_MAXLEN)
                    uiNumOfSend=(uiNumOfSend/SPI_UPLOAD_RAW_MAXLEN)+1;
                else
                    uiNumOfSend=(uiNumOfSend/SPI_UPLOAD_RAW_MAXLEN);
                if (uiNumOfSend>NUMSEND_ONETIME)
                    uiNumOfSend = NUMSEND_ONETIME;
                //ak_print_normal_ex("\n ------- uiNumOfSend: %d \r\n", uiNumOfSend);
                for(k = 0; k < uiNumOfSend; k++)
                {
                    // Length left not yet send
                    temp = (len - (g_iPosition_temp - g_iPosition) - k*SPI_UPLOAD_RAW_MAXLEN);
                    //ak_print_normal_ex("\n temp: %d \r\n", temp);
                    // This SPI packet length
                    if (temp>SPI_UPLOAD_RAW_MAXLEN)
                        usLen = SPI_UPLOAD_RAW_MAXLEN;
                    else
                        usLen = temp;

                    *(pMsg + UPLOAD_LEN_BYTE_OFFSET) = (char)(((usLen+4) & 0xFF00) >> 8);
                    *(pMsg + UPLOAD_LEN_BYTE_OFFSET + 1) = (char)((usLen+4) & 0x00FF);
                    iCurrentPosition = g_iPosition_temp + k*SPI_UPLOAD_RAW_MAXLEN;
                    /* position */
                    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET) = (char)((iCurrentPosition & 0xFF000000) >> 24);
                    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 1) = (char)((iCurrentPosition & 0x00FF0000) >> 16);
                    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 2) = (char)((iCurrentPosition & 0x0000FF00) >> 8);
                    *(pMsg + UPLOAD_POSITION_BYTE_OFFSET + 3) = (char)(iCurrentPosition & 0x000000FF);
                    memcpy(pMsg + UPLOAD_DATA_BYTE_OFFSET, buf + iCurrentPosition - g_iPosition, usLen);
                    /* send data over spi */
                    //ak_print_normal_ex("\n g_iPosition_temp: %d \r\n", g_iPosition_temp);
                    iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
                    if(iRet <= 0)
                    {
                        // resent if fails
                        ak_print_error_ex("%s Maybe ringbuffer is full!\r\n", __func__);
                        ak_sleep_ms(20); // Wait for 20ms before next 1 time resend
                        iRet = spi_uploader_send(pMsg, MAX_SPI_PACKET_LEN);
                    }
                }//end for loop
                iRet = 0;
                ak_sleep_ms(uiNumOfSend*4);
                if(1)
                {
                    j = 0;
                    ack_found=0;
                    while (j<140) // wait max 140ms for response
                    {
                        if (ack_found==0){
                            ak_sleep_ms(5);
                            j+=5;
                        }else
                            j+=1;
                        
                        // Poll response
                        ack_found=0;
                        iPosCCWanna = uploadsend_check_response();
                        //printf("\n ------- iPosCCWanna: %d \r\n", iPosCCWanna);
                        if ((iPosCCWanna <= 0) || ((g_iPosition==0)&&(g_iPosition_temp==0)&&(iPosCCWanna>HTTP_CHUNK_SIZE_NKB))) // second case is to remove old SPI packet from previous power session
                           continue;
                        if (iPosCCWanna > 0)
                        {
                            ack_found=1;
                            if (iPosCCWanna>g_iPosition_temp)
                                g_iPosition_temp = iPosCCWanna;
                            if ((g_iPosition_temp-g_iPosition)>=len)//Last packet of data has response -> to exit func
                                break;
                            if (g_iPosition_temp==(iCurrentPosition+usLen)) //Last packet of NUMSEND_ONETIME has response -> to continue with next round
                                break;
                        }
                    }

                    if ((g_iPosition_temp-g_iPosition)>=len) // Exit while/func
                        break;
                }//end if
                printf("\n ------- g_iPosition_temp: %d, len: %d g_iPosition: %d ------- \r\n", g_iPosition_temp, len, g_iPosition);
            }
            ak_print_error_ex("\n --------------------------------Send %d/%d bytes finish -------------------------------  \r\n ", g_iPosition_temp - g_iPosition, len);
            if (len!=0){ // Update if this not zero length block
            	iRet = g_iPosition_temp - g_iPosition;
            	g_iPosition = g_iPosition_temp;
        	}
            //----------------------------------------------End Thuan edited block -----------------------------------------------
        }
        /* End of process send api */
        if(pMsg)
        {
            free(pMsg);
        }
        if(pRecv)
        {
            free(pRecv);
        } 
    }
    return iRet;
}

#endif
//-----------------------------------------------------------------------------------------------------



ssize_t upload_recv_api(int sockfd, void *buf, size_t len, int flags)
{
    int iRet = 0;
    unsigned short usLen = 0;
    int iLengthCpy = 0;
    unsigned int uiNumOfSend = 0;
    int iPosition = 0;
    int iRetry = 10;
    int  i = 0;
    char *pRecv = NULL;

    unsigned long ulCurrentTick = get_tick_count();

    // ak_print_normal_ex("Recv len %u, state %d, pos %d\r\n", len, g_enum_state_simple, g_iPosition);
	// if(g_enum_state_simple != E_UPLOAD_STATE_SIMPLE_RECV)
    // {
    //     printf("%s wrong state\r\n",__func__);
    //     g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_OPEN;
    //     iRet = -1;
    // }
    // else
    // {
        if(buf == NULL)
        {
            printf("%s buffer is null", __func__);
            iRet = -1;
        }
    // }
    /* Main process */
    if(iRet == 0)
    {
        // send command SPI to open socket
        pRecv = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        if(pRecv == NULL)
        {
            printf("Cannot malloc pMsg or pRecv to receive socket!\r\n");
            iRet = -2;
        }
        else
        {
            if(g_first_recv_call == 0){
                /* Send position 04 to get response */
                upload_update_position_resp(pRecv, g_iPosition);
                iRet = spi_uploader_send(pRecv, MAX_SPI_PACKET_LEN);
                g_first_recv_call = 1;
            }

            uiNumOfSend = len/(MAX_SPI_PACKET_LEN - 10);
            if(len % (MAX_SPI_PACKET_LEN - 10))
            {
                uiNumOfSend++;
            }
            iLengthCpy = 0; // init value before loop
            printf("uiNumOfSend: %d g_iPosition: %d\r\n", uiNumOfSend, g_iPosition);
            for(i = 0; i < uiNumOfSend; i++)
            {
                // receive a packet
                // printf("[RECV]head for i =%d\r\n", i);
                iRet = 0;
                memset(pRecv, 0x00, MAX_SPI_PACKET_LEN);
                iRet = spi_uploader_receive(pRecv, MAX_SPI_PACKET_LEN);
                if(iRet > 0 && ( *(pRecv + UPLOAD_CTRL_BYTE_OFFSET) == UPLOAD_CTRL_RESPONSE) )
                {
                    printf("[RECV(%d)]", iRet);
                    dump_data_upload(pRecv, 12);
                    usLen = (*(pRecv + UPLOAD_LEN_BYTE_OFFSET) & 0x00FF) << 8;
                    usLen |= (*(pRecv + UPLOAD_LEN_BYTE_OFFSET + 1) & 0x00FF) << 0;
                    iPosition = ((*(pRecv + UPLOAD_POSITION_BYTE_OFFSET) & 0x000000FF) << 24);
                    iPosition |= ((*(pRecv + UPLOAD_POSITION_BYTE_OFFSET + 1) & 0x000000FF) << 16);
                    iPosition |= ((*(pRecv + UPLOAD_POSITION_BYTE_OFFSET + 2) & 0x000000FF) << 8);
                    iPosition |= ((*(pRecv + UPLOAD_POSITION_BYTE_OFFSET + 3) & 0x000000FF) << 0);
                    usLen = usLen - 4;
                    if(usLen != 0)
                    {
                        printf("RECV: pos:%d, len(data):(0x%x) %u, g_pos:%d\r\n", iPosition, usLen, usLen, g_iPosition);
                        
                        if((iPosition == g_iPosition) && (usLen > 0))
                        {
                            printf("[RECV] iLengthCpy: %d, iPosition: %d, g_iPosition:%d time %u, i: %d\r\n", iLengthCpy, iPosition, g_iPosition, get_tick_count() - ulCurrentTick, i);
                            memcpy(buf + iLengthCpy, pRecv + UPLOAD_DATA_BYTE_OFFSET, usLen);
                            iLengthCpy += usLen;
                            iPosition += usLen;
                            g_iPosition += usLen;
                        }
                        else
                        {
                            i--;
                            iRetry--;
                            printf("[RECV]invalid ipos:%d, gpos:%d\r\n", iPosition, g_iPosition);
                            if(iRetry == 0)
                            {
                                ak_print_error_ex("RECV retry 0, break");
                                iLengthCpy = -1;
                                break;
                            }
                        }
                        // send postion to confirm
                        upload_update_position_resp(pRecv, g_iPosition);
                        // prepare packet and send it
                        iRet = spi_uploader_send(pRecv, MAX_SPI_PACKET_LEN);
                        // printf("[RECV]send_position2:");
                        // dump_data_upload(pRecv, 20);
                    }
                    else // if(usLen == 0)
                    {
                        iRet = 0;
                        g_iPosition = 0;
                        g_first_recv_call = 0;
                        printf("No more data response!->STATE_SIMPLE_CLOSE======================\r\n");
                        g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_CLOSE;
                        break;
                    }
                }// if iRet > 0
                else
                {
                    // ak_print_error("Data invalid!");
                    // dump_data_upload(pRecv, 8);
                    i--;
                    ak_sleep_ms(10);
                   
                    // printf(" [RECV][%d] Ret %d! ->Try to send gpos:%d\r\n", i, iRet, g_iPosition);
                    // send postion to confirm
                    upload_update_position_resp(pRecv, g_iPosition);
                    // prepare packet and send it
                    iRet = spi_uploader_send(pRecv, MAX_SPI_PACKET_LEN);

                    if((get_tick_count() - ulCurrentTick) > 3000) // timeout 300ms
                    {
                        iLengthCpy = 0;
                        printf(" [RECV]Timeout 3000ms!gpos:%d\r\n", g_iPosition);
                        break;
                    }
                }
                // printf("[%d] uiNumOfSend: %d g_iPosition: %d\r\n", i, uiNumOfSend, g_iPosition);
            }// end of for
            iRet = iLengthCpy; //usLen;
        }

        if(len == 0) // recevice len = 0, no more data
        {
            iRet = 0;
            g_iPosition = 0;
            g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_CLOSE;
            ak_print_normal_ex("Upgrading done, change to state E_UPLOAD_STATE_SIMPLE_RECV\r\n");
        }

        /* End of process send api */
        if(pRecv)
        {
            free(pRecv);
        }
    }
    printf("========= %s return %d=======\r\n", __func__, iRet);
    return iRet;
}

int upload_close_api(int fd)
{
    int iRet = 0;
    char *pSend = NULL;
    // int i = 0;

    ak_print_normal_ex("Close state %d, pos %d\r\n", g_enum_state_simple, g_iPosition);
    // if(g_enum_state_simple != E_UPLOAD_STATE_SIMPLE_CLOSE)
    // {
    //     printf("%s wrong state\r\n",__func__);
    //     g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_OPEN;
    //     iRet = -1;
    // }
    /* Main process */
    if(iRet == 0)
    {
        // send command SPI to open socket
        pSend = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        if(pSend == NULL)
        {
            printf("Cannot malloc pMsg or pRecv to receive socket!\r\n");
            iRet = -2;
        }
        else
        {
            memset(pSend, 0x00, MAX_SPI_PACKET_LEN);
            *(pSend + 0) = SPI_CMD_MOTION_UPLOAD;
            *(pSend + UPLOAD_CTRL_BYTE_OFFSET) = UPLOAD_CTRL_CLOSE_SOCKET;
            // for(i = 0; i < 2; i++)
            // {
                iRet = spi_uploader_send(pSend, MAX_SPI_PACKET_LEN);
                // ak_sleep_ms(10);
            // }

        }
        /* End of process send api */
        if(pSend)
        {
            free(pSend);
        }
    }
    g_iPosition = 0;
    g_enum_state_simple = E_UPLOAD_STATE_SIMPLE_OPEN;
    uploader_flush_buffer_after_uploading();
    return iRet;
}

void uploader_flush_buffer_after_uploading(void)
{
    // try to read all data and return
    int iRet = 0;
    int iPosResp = 0;
    int iLen = 0;
    int i = 0;
    char *pResp = (char *)malloc(sizeof(char)*MAX_SPI_PACKET_LEN);
    if(pResp == NULL)
    {
        return -1;
    }
    memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
    for(;;)
    {
        iRet = spi_uploader_receive(pResp, MAX_SPI_PACKET_LEN);
        if(iRet == 0)
        {
            break;
        }
    }
    free(pResp);
    return iRet;
}


/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/
