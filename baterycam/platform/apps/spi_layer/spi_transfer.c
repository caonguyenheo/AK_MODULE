/**
  ******************************************************************************
  * File Name          : spi_transfer.c
  * Description        : This file contain functions for spi
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ringbuffer.h"
#include "spi_transfer.h"
#include "ak_apps_config.h"
#include "spi_cmd.h"
#include "ak_drv_spi.h"

#include "ak_talkback.h"
// #include "hal_timer.h"

/*
********************************************************************************
*                            DEFINE                                            *
********************************************************************************
*/

/*
********************************************************************************
*                            VARIABLES                                         *
********************************************************************************
*/
extern int g_play_dingdong;
extern int g_SpiCmdSnapshot;

extern int g_iCdsValue;

unsigned int g_uiSmallestPid = 0;
int g_iPidStart = 0;

// p2p config
// extern xQueueHandle  queue_for_p2p_config;

/*static unsigned char *ui8SpiTxBuffer;
static unsigned char *ui8SpiRxBuffer;*/
static unsigned char ui8SpiTxResendBuffer[MAX_SPI_PACKET_LEN];
static unsigned char ui8SpiTxBuffer[MAX_SPI_PACKET_LEN];
static unsigned char ui8SpiRxBuffer[MAX_SPI_PACKET_LEN];

static int g_iIsSpiTaskInit = 0;

typedef struct SPIDATA{
    RingBuffer          *fifo;
    //xSemaphoreHandle    sem_lock;
#ifndef SPI_DISABLE_MUTEX
    ak_mutex_t          lock;
#endif
}SPIDATA_t;

typedef struct SPI_TRANSFER_DATA{
    SPIDATA_t tx;
    SPIDATA_t rx;
}SPI_TRANSFER_DATA_t;

static SPI_TRANSFER_DATA_t g_spi_data;

#ifdef CFG_SPI_COMMAND_ENABLE
static SPI_TRANSFER_DATA_t g_spi_cmd_buf;
#endif

/* define structure for fwupgrade, I make it seperate to easy maintain */
static SPI_TRANSFER_DATA_t g_spi_fwupg;
/* define structure for uploading, I make it seperate to easy maintain */
static SPI_TRANSFER_DATA_t g_spi_uploader;
/* define structure for ack p2p sdcard streaming */
static SPI_TRANSFER_DATA_t g_spi_p2psdstream;

#ifdef CFG_RESEND_VIDEO
RingBuffer          *pRbRecvAck;
#endif

/*
********************************************************************************
*                            FUNCTIONS                                         *
********************************************************************************
*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
// use spi_data
static int int_spi_data(SPIDATA_t *spidata, int length)
{
    int ret = 0;
    if(!spidata || length <= 0){
        return -1;
    }
    // init fifo
    spidata->fifo = RingBuffer_create(length);
    if(spidata->fifo == NULL){
        ret = -2;
    }
    else{
#ifndef SPI_DISABLE_MUTEX
        ret = ak_thread_mutex_init(&(spidata->lock));
        if(ret == -1){
            printf("AK: Init mutex failed!\n");
            ret = -3;
        }
        else{
            // do nothing, spidata init OK
            dzlog_debug("SPI init mutex!\n");
        }
#endif
        // printf("DEBUG: Rb: 0x%x with length: %d\n", (unsigned int)(spidata->fifo), length);
    }
    return ret;
}

static void deinit_spi_data(SPIDATA_t *spidata)
{
    if(spidata){
        if(spidata->fifo){
            RingBuffer_destroy(spidata->fifo);
        }
        spidata->fifo = NULL;
/*
        if(spidata->sem_lock){
            vSemaphoreDelete(spidata->sem_lock);
        }
        spidata->sem_lock = NULL;
*/
#ifndef SPI_DISABLE_MUTEX
        ak_thread_mutex_destroy(&(spidata->lock));
#endif
    }
}

static int init_spi_transfer_data(SPI_TRANSFER_DATA_t *spi_x_data, int length)
{
    int ret = 0;
    if(!spi_x_data || length <= 0){
        return -1;
    }
    ret = int_spi_data(&(spi_x_data->tx), length);
    if(ret < 0){
        printf("[ERR] init structure spi data tx failed! ret = %d\n", ret);
    }
    else{
        ret = int_spi_data(&(spi_x_data->rx), length);
        if(ret < 0){
            printf("[ERR] init structure spi data rx failed! ret = %d\n", ret);
            // free spidata
            deinit_spi_data(&(spi_x_data->tx)); 
        }
        else{
            // structure spi_x_data init OK
        }
    }
    return ret;
}

static void deinit_spi_transfer_data(SPI_TRANSFER_DATA_t *spi_x_data)
{
    if(spi_x_data){
        deinit_spi_data(&(spi_x_data->tx)); 
        deinit_spi_data(&(spi_x_data->rx)); 
    }
}

/* Put data to structure with locking semaphore */
static int spidata_put(SPIDATA_t *spidata, char *data, int length)
{
    int ret = 0;
    if(!spidata || !data || (length <= 0)){
        return -1;
    }
/*
    if(spidata->sem_lock == NULL){
        printf("[WARN]Put: Need to initialize semaphore!\n");
        return -2;
    }
*/
    if(spidata->fifo == NULL){
        printf("[WARN]Put: Need to initialize fifo!\n");
        return -3;
    }
#ifndef SPI_DISABLE_MUTEX
    ak_thread_mutex_lock(&(spidata->lock));
#endif
    // write data to fifo
    ret = RingBuffer_write(spidata->fifo, data, length);
#ifndef SPI_DISABLE_MUTEX
    ak_thread_mutex_unlock(&(spidata->lock));
#endif
    if(ret <= 0){
        // printf("[ERR]Put: Write to FIFO failed! ret (%d)\n!", ret);
        ret = -4;
    }
    return ret;
}
/* Pop data from structure with locking semaphore */
static int spidata_pop(SPIDATA_t *spidata, char *data, int length)
{
    int ret = 0;
    // dzlog_debug("%s param: spidata: 0x%x, data: 0x%x, length: 0x%x\n", __func__, spidata, data, length);
    if(!spidata || !data || (length <= 0)){
        return -1;
    }
/*
    if(spidata->sem_lock == NULL){
        printf("[WARN]Pop: Need to initialize semaphore!\n");
        return -2;
    }
*/    
    if(spidata->fifo == NULL){
        printf("[WARN]Pop: Need to initialize fifo!\n");
        return -3;
    }
#ifndef SPI_DISABLE_MUTEX
    ak_thread_mutex_lock(&(spidata->lock));
#endif
    // write data to fifo
    ret = RingBuffer_read(spidata->fifo, data, length);
#ifndef SPI_DISABLE_MUTEX
    ak_thread_mutex_unlock(&(spidata->lock));
#endif

    if(ret < 0){
        printf("[ERR]Pop: Read to FIFO failed! ret (%d)\n!", ret);
        ret = -4;
    }
    return ret;
}


/* put and pop for interrupt */
/* Put data to structure with locking semaphore */
static int spidata_put_irq(SPIDATA_t *spidata, char *data, int length)
{
    int ret = 0;
    ret = spidata_put(spidata, data, length);
    return ret;
}
/* Pop data from structure with locking semaphore */
static int spidata_pop_irq(SPIDATA_t *spidata, char *data, int length)
{
    int ret = 0;
    ret = spidata_pop(spidata, data, length);
    return ret;
}



/*
* Write data to spi transfer's buffer to send over MOSI
* In case of IRQ, set variable isIRQ = 1
*/
static int spi_x_write_tx(SPI_TRANSFER_DATA_t *spi_x_data, char *data, int length, int isIRQ)
{
    int ret = 0;
    SPIDATA_t *_spidata;
    if(!spi_x_data){
        printf("[ERR]Need to initialize structure SPI_TRANSFER_DATA\n");
        return -1;
    }
    _spidata = &(spi_x_data->tx);
    if(isIRQ == 1){
        ret = spidata_put_irq(_spidata, data, length);
    }
    else{
        ret = spidata_put(_spidata, data, length);
    }
    return ret;
}

/*
* Read data from spi transfer's buffer to send over MOSI
*/
static int spi_x_read_tx(SPI_TRANSFER_DATA_t *spi_x_data, char *data, int length, int isIRQ)
{
    int ret = 0;
    SPIDATA_t *_spidata;
    // dzlog_info("%s entered\n", __func__);
    if(!spi_x_data){
        printf("[ERR]Need to initialize structure SPI_TRANSFER_DATA\n");
        return -1;
    }
    _spidata = &(spi_x_data->tx);
    if(isIRQ == 1){
        ret = spidata_pop_irq(_spidata, data, length);
    }
    else{
        ret = spidata_pop(_spidata, data, length);
    }
    return ret;
}

/*
* Write data to spi transfer's buffer form interrupt SPI DMA
*/
static int spi_x_write_rx(SPI_TRANSFER_DATA_t *spi_x_data, char *data, int length, int isIRQ)
{
    int ret = 0;
    SPIDATA_t *_spidata;
    if(!spi_x_data){
        printf("[ERR]Need to initialize structure SPI_TRANSFER_DATA\n");
        return -1;
    }
    _spidata = &(spi_x_data->rx);
    if(isIRQ == 1){
        ret = spidata_put_irq(_spidata, data, length);
    }
    else{
        ret = spidata_put(_spidata, data, length);
    }
    return ret;
}

/*
* Read data from spi transfer's buffer from SPI MISO
*/
static int spi_x_read_rx(SPI_TRANSFER_DATA_t *spi_x_data, char *data, int length, int isIRQ)
{
    int ret = 0;
    SPIDATA_t *_spidata;
    if(!spi_x_data){
        printf("[ERR]Need to initialize structure SPI_TRANSFER_DATA\n");
        return -1;
    }
    _spidata = &(spi_x_data->rx);
    if(isIRQ == 1){
        ret = spidata_pop_irq(_spidata, data, length);
    }
    else{
        ret = spidata_pop(_spidata, data, length);
    }
    return ret;
}

/*----------------------------------------------------------------------------*/
/*                        SPI COMMAND                                         */
/*----------------------------------------------------------------------------*/

/*
*   Brief: Init structure buffer for spi command
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-June-2017
*/
void spi_cmd_init(void)
{
#ifdef CFG_SPI_COMMAND_ENABLE
    int ret = init_spi_transfer_data(&g_spi_cmd_buf, SPI_CMD_RB_LEN);
    if(ret < 0){
        dzlog_error("%s Init buffer spi cmd failed!\n", __func__);
    }
#endif
}

/*
*   Brief: Free structure buffer for spi command
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-June-2017
*/
void spi_cmd_deinit(void)
{
#ifdef CFG_SPI_COMMAND_ENABLE
    deinit_spi_transfer_data(&g_spi_cmd_buf);
#endif
}

/*
*   Brief: Spi command send response to CC3200
*   Param: [buf] pointer to data wanna send
*           [len] length of data send
*   Return: <0 error
*           >0 length data that push to buffer
*   Author: Tien Trung
*   Date: 22-June-2017
*/
int spi_cmd_send(char *buf, int len)
{
#ifdef CFG_SPI_COMMAND_ENABLE
    int ret = 0;
    if(buf == NULL){
        dzlog_error("spi cmd send Buffer NULL\n");
        ret = -1;
    }
    else{
        /* NOTE: Inside RingBuffer_write function, it will check size of space! */
        ret = spi_x_write_tx(&g_spi_cmd_buf, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("spi cmd send failed, ret=%d\n", ret);
        }
        else
        {
            // printf("*%02X%02X", *(buf+0), *(buf+1));
        }
    }
    return ret;
#else
    return 0;
#endif
}
/*
*   Brief: Spi command receive command packet from CC3200
*   Param: [buf] pointer to data wanna receive
*           [len] length of data receive
*   Return: <0 error
*           >0 length data receive
*   Author: Tien Trung
*   Date: 22-June-2017
*/
int spi_cmd_receive(char *buf, int len)
{
#ifdef CFG_SPI_COMMAND_ENABLE
    int ret = 0;
    if(buf == NULL){
        dzlog_error("spi cmd receive Buffer NULL\n");
        ret = -1;
    }
    else{
        ret = spi_x_read_rx(&g_spi_cmd_buf, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("sspi cmd receive failed,  ret=%d\n", ret);
        }
    }
    return ret;
#else
    return 0;
#endif
}

/*---------------------------------------------------------------------------*/
/*                           FIRMWARE UPGRADE                                */
/*---------------------------------------------------------------------------*/
/*
*   Brief: Init structure buffer for firmware upgrade
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 2-August-2017
*/
void spi_fwupgrade_init(void)
{
    int ret = init_spi_transfer_data(&g_spi_fwupg, SPI_FWUPGRADE_BUFFER_LEN);
    if(ret < 0){
        dzlog_error("%sInit buffer for firmware upgrade FAILED\n", __func__);
    }
}

/*
*   Brief: Free structure buffer for transfer firmware upgrade
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 2-August-2017
*/
void spi_fwupgrade_deinit(void)
{
    deinit_spi_transfer_data(&g_spi_fwupg);
}

/*
*   Brief: AK send data or ACK to CC3200 in fwupgrade mode
*   Param: [buf] pointer to data wanna send
*           [len] length of data send
*   Return: <0 error
*           >0 length data that push to buffer
*   Author: Tien Trung
*   Date: 2-August-2017
*/
int spi_fwupgrade_send(char *buf, int len)
{
    int ret = 0;
    if(buf == NULL){
        dzlog_error("SPI_FW send Buffer NULL\n");
        ret = -1;
    }
    else{
        /* NOTE: Inside RingBuffer_write function, it will check size of space! */
        ret = spi_x_write_tx(&g_spi_fwupg, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("SPI_FW send failed, ret=%d\n", ret);
        }
        else
        {
            // printf("*%02X", *(buf+0));
        }
    }
    return ret;
}
/*
*   Brief: AK receive data or ACK to CC3200 in fwupgrade mode
*   Param: [buf] pointer to data wanna receive
*           [len] length of data receive
*   Return: <0 error
*           >0 length data receive
*   Author: Tien Trung
*   Date: 2-August-2017
*/
int spi_fwupgrade_receive(char *buf, int len)
{
    int ret = 0;
    if(buf == NULL){
        dzlog_error("SPI_FW receive Buffer NULL\n");
        ret = -1;
    }
    else{
        ret = spi_x_read_rx(&g_spi_fwupg, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("SPI_FW receive failed,  ret=%d\n", ret);
        }
    }
    return ret;
}

/*---------------------------------------------------------------------------*/
/*                           UPLOADING                                       */
/*---------------------------------------------------------------------------*/
/*
*   Brief: Init structure buffer for uploading
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-August-2017
*/
void spi_uploader_init(void)
{
    int ret = init_spi_transfer_data(&g_spi_uploader, SPI_UPLOADER_BUFFER_LEN);
    if(ret < 0){
        dzlog_error("%sInit buffer for g_spi_uploader FAILED\n", __func__);
    }
}

/*
*   Brief: Free structure buffer for uploading
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-August-2017
*/
void spi_uploader_deinit(void)
{
    deinit_spi_transfer_data(&g_spi_uploader);
}

/*
*   Brief: AK send data or ACK to CC3200
*   Param: [buf] pointer to data wanna send
*           [len] length of data send
*   Return: <0 error
*           >0 length data that push to buffer
*   Author: Tien Trung
*   Date: 22-August-2017
*/
int spi_uploader_send(char *buf, int len)
{
    int ret = 0;
    int i = 0;
    if(buf == NULL){
        dzlog_error("SPI_UPLOAD send Buffer NULL\n");
        ret = -1;
    }
    else{
        // printf("SSSS:");
        // for(i = 0; i < 8; i++)
        // {
        //     printf("%02X ", *(buf + i));
        // }
        printf("P");
        /* NOTE: Inside RingBuffer_write function, it will check size of space! */
        ret = spi_x_write_tx(&g_spi_uploader, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("SPI_UPLOAD send failed, ret=%d\n", ret);
        }
        else
        {
            // printf("*%02X", *(buf+0));
        }
    }
    return ret;
}
/*
*   Brief: AK receive data or ACK to CC3200
*   Param: [buf] pointer to data wanna receive
*           [len] length of data receive
*   Return: <0 error
*           >0 length data receive
*   Author: Tien Trung
*   Date: 22-August-2017
*/
int spi_uploader_receive(char *buf, int len)
{
    int ret = 0;
    if(buf == NULL){
        dzlog_error("SPI_UPLOAD receive Buffer NULL\n");
        ret = -1;
    }
    else{
        ret = spi_x_read_rx(&g_spi_uploader, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("SPI_UPLOAD receive failed,  ret=%d\n", ret);
        }
    }
    return ret;
}

int spi_uploade_reset_buffer(void)
{
    RingbBuffer_flush(g_spi_uploader.tx.fifo);
    RingbBuffer_flush(g_spi_uploader.rx.fifo);
}
/*---------------------------------------------------------------------------*/
/*                           P2P SDCARD STREAMING                            */
/*---------------------------------------------------------------------------*/
/*
*   Brief: Init structure buffer for uploading
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-August-2017
*/
void spi_p2psdstream_init(void)
{
    int ret = init_spi_transfer_data(&g_spi_p2psdstream, SPI_P2PSDSTREAM_BUFFER_LEN);
    if(ret < 0){
        dzlog_error("%sInit buffer for g_spi_uploader FAILED\n", __func__);
    }
}

/*
*   Brief: Free structure buffer for uploading
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-August-2017
*/
void spi_p2psdstream_deinit(void)
{
    deinit_spi_transfer_data(&g_spi_p2psdstream);
}

/*
*   Brief: AK send data or ACK to CC3200
*   Param: [buf] pointer to data wanna send
*           [len] length of data send
*   Return: <0 error
*           >0 length data that push to buffer
*   Author: Tien Trung
*   Date: 22-August-2017
*/
int spi_p2psdstream_send(char *buf, int len)
{
    int ret = 0;
    int i = 0;
    if(buf == NULL){
        dzlog_error("SPI_UPLOAD send Buffer NULL\n");
        ret = -1;
    }
    else{
        /* NOTE: Inside RingBuffer_write function, it will check size of space! */
        ret = spi_x_write_tx(&g_spi_p2psdstream, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("SPI_UPLOAD send failed, ret=%d\n", ret);
        }
        else
        {
            // printf("*%02X", *(buf+0));
        }
    }
    return ret;
}
/*
*   Brief: AK receive data or ACK to CC3200
*   Param: [buf] pointer to data wanna receive
*           [len] length of data receive
*   Return: <0 error
*           >0 length data receive
*   Author: Tien Trung
*   Date: 22-August-2017
*/
int spi_p2psdstream_receive(char *buf, int len)
{
    int ret = 0;
    if(buf == NULL){
        dzlog_error("SPI_P2PSD receive Buffer NULL\n");
        ret = -1;
    }
    else{
        ret = spi_x_read_rx(&g_spi_p2psdstream, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("SPI_P2PSD receive failed,  ret=%d\n", ret);
        }
    }
    return ret;
}


/*----------------------------------------------------------------------------*/
/*                       End of API spi cmd                                   */
/*----------------------------------------------------------------------------*/

int spi_init_gpio_input_interrupt(void)
{
    int iRet = 0;
    // if(gpio_set_pin_as_gpio(SPI_CC3200_TRIGGER_INT) == 1)
    // {
    //     ak_print_normal("%s Pin %d is as gpio pin!\r\n", __func__, SPI_CC3200_TRIGGER_INT);
    //     gpio_set_pin_dir(SPI_CC3200_TRIGGER_INT, GPIO_DIR_INPUT);
    // }
    // else
    // {
    //     ak_print_normal("%s Pin %d is not as gpio pin!\r\n", __func__, SPI_CC3200_TRIGGER_INT);
    // }

    if(gpio_set_pin_as_gpio(SPI_AK_TRIGGER_INT) == 1)
    {
        ak_print_normal("%s Pin %d is as gpio pin!\r\n", __func__, SPI_AK_TRIGGER_INT);
        gpio_set_pin_dir(SPI_AK_TRIGGER_INT, GPIO_DIR_INPUT);
    }
    else
    {
        ak_print_normal("%s Pin %d is not as gpio pin!\r\n", __func__, SPI_AK_TRIGGER_INT);
    }
    
    // gpio_set_pin_level(SPI_CC3200_TRIGGER_INT, GPIO_LEVEL_LOW);  // pin 47 level 0
    return iRet;
}

int spi_read_gpio_slave_trigger(unsigned long pin)
{
    int iRet = 0;
    unsigned char ucValue = 0;
    ucValue = gpio_get_pin_level(pin);
    iRet = (int)ucValue;
    return iRet;
}

int drv_init_spi(void)
{
    int i32Ret = 0;
    // try to init spi
    i32Ret = ak_drv_spi_open(1, SPI_TRAN_SIZE);
    spi_init_gpio_input_interrupt();
    dzlog_debug("ak_drv_spi_open: ret=%d\n", i32Ret);
    return i32Ret;
}


int init_spi(void)
{
    int ret = 0;

    printf("%s ENTERED!\n", __func__);
    if(ui8SpiTxBuffer == NULL || ui8SpiRxBuffer == NULL){
        dzlog_error("alloc memory for SPI TX error!\n");
    }
    else{
        // dzlog_debug("%s txbuf: 0x%x, rxbuf: 0x%x\n", __func__, ui8SpiTxBuffer, ui8SpiRxBuffer);
    }

    ret = init_spi_transfer_data(&g_spi_data, SPI_RB_MAX_LEN);
    if(ret < 0){
        printf("BUG in my code!\n");
    }

    ret = drv_init_spi();
    dzlog_info("drv init spi (%d)\n", ret);
    
#ifdef CFG_TALKBACK_FAST_PLAY    
    // code test
    ret = talkback_init_engine();
    if(ret != 0)
    {
        dzlog_info("init talkback (%d)\n", ret);
    }
#endif

#ifdef CFG_RESEND_VIDEO
    pRbRecvAck = RingBuffer_create(16*1024);
    if(pRbRecvAck == NULL)
    {
        dzlog_error("Cannot create ringbuf for resend!\n");
    }
#endif

    /* Init ringbuffer for SPI command */
    spi_cmd_init();
    /* Init buffer transfer for firmware upgrade over CC3200 */
    spi_fwupgrade_init();

// #ifdef UPLOADER_ENABLE
    /* Init buffer transfer for uploading */
    spi_uploader_init();
// #endif // #ifdef UPLOADER_ENABLE
    spi_p2psdstream_init();

    return ret;
}

int deinit_spi(void)
{
    deinit_spi_transfer_data(&g_spi_data);
    // code test
#ifdef CFG_TALKBACK_FAST_PLAY
    talkback_deinit_engine();
#endif

    /* Free ringbuffer for SPI Command */
    spi_cmd_deinit();
    /* Free ringbuffer for SPI Firmware upgrade over CC3200 */
    spi_fwupgrade_deinit();
    /* Free ringbuffer for SPI Uploader */
    spi_uploader_deinit();

    spi_p2psdstream_deinit();
}

int send_spi_data(char *buf, int len)
{
    int ret = 0;
    char *pData = NULL;
    if(buf == NULL){
        ret = -1;
    }
    else{
        pData = buf;
        /* NOTE: Inside RingBuffer_write function, it will check size of space! */
        //printf("\nWRITE: len:%d, start:%d, end:%d\n", pRingBufSpiTx->length, pRingBufSpiTx->start, pRingBufSpiTx->end);
        ret = spi_x_write_tx(&g_spi_data, pData, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            // dzlog_error("spi_x_write_tx failed, ret=%d\n", ret);
        }
    }
    return ret;
}

int read_spi_data(char *buf, int len)
{
    int ret = 0;
    if(buf == NULL){
        dzlog_error("Buffer read NULL\n\r");
        ret = -1;
    }
    else{
        ret = spi_x_read_rx(&g_spi_data, buf, SPI_TRAN_SIZE, 0);
        if(ret < 0){
            dzlog_error("spi_x_read_rx failed, ret=%d\n", ret);
        }
    }
    return ret;
}

int spi_check_error(char *p_data_recv)
{
    int i = 0;
    int ret = 0;
    char *pPacket = p_data_recv;
    unsigned short usCsErr = 0;
    char cHighByte = 0;
    char cLowByte = 0;
    char acDebugCs[1024];

    // TODO: add more machenism to check data interity
    // Trung add checksum
    for(i = 0; i < 1022; i++)
    {
        usCsErr += *(pPacket + i);
    }
    cHighByte = (usCsErr >> 8) & 0xFF;
    cLowByte = usCsErr & 0xFF;
    // UART_PRINT("\r\nusCs: %x, b1022: %x, b1023:%x\r\n", usCsErr, *(pPacket + 1022), *(pPacket + 1023));
    if((cHighByte != *(pPacket + 1022)) || (cLowByte != *(pPacket + 1023)))
    {
        ret = -1;
        // dzlog_error("ERROR: Checksum failed! nusCs: %x, b1022: %x, b1023:%x\r\n", usCsErr, *(pPacket + 1022), *(pPacket + 1023));
        memset(acDebugCs, 0x00, 1024);
        // show first 10 bytes header
        pPacket = p_data_recv;
        for(i=0; i < 10; i ++)
        {
            sprintf(acDebugCs + strlen(acDebugCs), "%02X ", *(pPacket + i));
        }
        sprintf(acDebugCs + strlen(acDebugCs), "... ");

        // show the last 20 bytes
        pPacket = (char *)(p_data_recv + 1023 - 20);
        for(i=0; i < 20; i ++)
        {
            sprintf(acDebugCs + strlen(acDebugCs), "%02X ", *(pPacket + i));
        }
        // dzlog_error("Data RECV: %s\r\n", acDebugCs);
    }
    return ret;
}


/*
*   Brief: clarify receive data and push to queue
*   Param: [buf] pointer to the received data, 1024 bytes
*   Return: <0 error
*           =0 OK
*   NOTE: this functin will not check data NULL
*   Author: Tien Trung
*   Date: 22-June-2017
*/
int spi_process_received_data(unsigned char *pdata)
{
    int ret = 0;
    unsigned char ucPacketType = *(pdata + 0) & (~CC_SEND_SPI_PACKET_MASK);
    unsigned int uiPidRecv = 0;
    int iValue = 0;

    // dzlog_info("$%02x\n", ucPacketType);

    switch(ucPacketType)
    {
        case DATA_STREAMING:
            if(g_play_dingdong == 1)
            {
                printf("Play dingdong!\r\n");
                // break;
            }
            else
            {
                #ifndef CFG_TALKBACK_FAST_PLAY
                ret = spi_x_write_rx(&g_spi_data, pdata, SPI_TRAN_SIZE, 0);
                if(ret < 0)
                {
                    dzlog_error("%s spi_x_write_rx failed, ret=%d\n", __func__, ret);
                }
                #else
                talkback_process_data_spi(data_rx, SPI_TRAN_SIZE);
                #endif
                // break;
            }
            break;

        case SPI_CMD_ACK_P2P_SD_STREAM:
            uiPidRecv = (*(pdata + 3) & 0xFF) << 24;
            uiPidRecv |= (*(pdata + 4) & 0xFF) << 16;
            uiPidRecv |= (*(pdata + 5) & 0xFF) << 8;
            uiPidRecv |= (*(pdata + 6) & 0xFF) << 0;
            // printf("P2PACK: 0x%x (%d)\n", uiPidRecv, uiPidRecv);
            ret = spi_x_write_rx(&g_spi_p2psdstream, pdata, SPI_TRAN_SIZE, 0);
            if(ret < 0)
            {
                dzlog_error("%s Queue spi rx full ret=%d\n", __func__, ret);
            }
            break;

        case STREAM_ACK:
    #ifdef CFG_RESEND_VIDEO
            uiPidRecv = (*(pdata + 3) & 0xFF) << 24;
            uiPidRecv |= (*(pdata + 4) & 0xFF) << 16;
            uiPidRecv |= (*(pdata + 5) & 0xFF) << 8;
            uiPidRecv |= (*(pdata + 6) & 0xFF) << 0;
        #ifndef UPLOADER_DEBUG
            printf("AR:%u ", uiPidRecv);
        #endif
            if(g_iPidStart == 0)
            {
                ret = RingBuffer_write(pRbRecvAck, pdata, SPI_TRAN_SIZE);
                if(ret != SPI_TRAN_SIZE)
                {
                    dzlog_error("Recv: Write ACK to RB failed, ret=%d\n", ret);
                }
                else
                {
                    g_uiSmallestPid = uiPidRecv;
                    g_iPidStart = 1;
                    dzlog_info("RECEIVED FIRST PACKET ACK\n");
                }
            }
            else
            {
                if(uiPidRecv > g_uiSmallestPid){
                    // push data to ringbuffer
                    ret = RingBuffer_write(pRbRecvAck, pdata, SPI_TRAN_SIZE);
                    if(ret != SPI_TRAN_SIZE)
                    {
                        dzlog_error("Recv: Write ACK to RB failed, ret=%d\n", ret);
                    }
                    else
                    {
                        g_uiSmallestPid = uiPidRecv;
                    }
                }
                else
                {
                    #ifndef UPLOADER_DEBUG
                    dzlog_info("AP:%u_", g_uiSmallestPid);
                    #endif
                }
            }
    #endif
            break;

        case CMD_RESPONSE:
            dzlog_info("CMD_RESPONSE\n");
        case SPI_CMD_FLICKER:
        case SPI_CMD_FLIP:
        case SPI_CMD_VIDEO_BITRATE:
        case SPI_CMD_VIDEO_FRAMERATE:
        case SPI_CMD_VIDEO_RESOLUTION:
        case SPI_CMD_AES_KEY:
        case SPI_CMD_GET_CDS:
        case SPI_CMD_NTP:
        case SPI_UPGRADE_GET_VERSION:
        // case SPI_CMD_PLAY_DINGDONG:
        case SPI_CMD_UDID:
        case SPI_CMD_TOKEN:
        case SPI_CMD_URL:
        case SPI_CMD_FILE_TRANSFER:
        case SPI_CMD_PLAY_VOICE:
        case SPI_CMD_VOLUME:
        case SPI_CMD_FACTORY_TEST:
            ret = spi_x_write_rx(&g_spi_cmd_buf, pdata, SPI_TRAN_SIZE, 0);
            if(ret < 0)
            {
                dzlog_error("%s g_spi_cmd_buf failed, ret=%d\n", __func__, ret);
            }

            if(ucPacketType == SPI_CMD_URL)
            {
                ak_print_normal_ex("Receive cmd URL\r\n");
            }
            break;
        case SPI_CMD_PLAY_DINGDONG:
            // g_play_dingdong = 1;
            ret = spi_x_write_rx(&g_spi_cmd_buf, pdata, SPI_TRAN_SIZE, 0);
            if(ret < 0)
            {
                dzlog_error("%s g_spi_cmd_buf failed, ret=%d\n", __func__, ret);
            }
            break;
        case SPI_CMD_FWUPGRADE:
            ret = spi_x_write_rx(&g_spi_fwupg, pdata, SPI_TRAN_SIZE, 0);
            if(ret < 0)
            {
                dzlog_error("%s g_spi_fwupg failed, ret=%d\n", __func__, ret);
            }
            break;
        case SPI_CMD_MOTION_UPLOAD:
        // #ifdef UPLOADER_ENABLE
            ret = spi_x_write_rx(&g_spi_uploader, pdata, SPI_TRAN_SIZE, 0);
            if(ret < 0)
            {
                dzlog_error("%s g_spi_uploader failed, ret=%d\n", __func__, ret);
            }
        // #else
            // dzlog_info("Not yet support command fwupgrade and motion upload!\n");
        // #endif
            break;
        default:
            dzlog_info("Unsupport type: %02x\n", ucPacketType);
            break;
    }

    return ret;
}

//stub for new SDK
int drv_spi_master_send(unsigned char *data_tx, unsigned char *data_rx,unsigned long len)
{
    int ret = 0;
    int *ptr_data = (int *)data_tx;
    int *p_data_recv = (int *)data_rx;
    int i = 0;
    unsigned short usCsCalc = 0;
    char *pPacket = (char *)data_tx;
    int iterator = 0;
    unsigned char ucCmdType = 0;

    /* Add checksum error */
    for(iterator = 0; iterator < (1024-2); iterator++)
    {
        usCsCalc += *(pPacket + iterator);
    }
    *(pPacket + 1024-2) = (char)((usCsCalc >> 8) & 0xFF);
    *(pPacket + 1024-1) = (char)(usCsCalc & 0xFF);
    usCsCalc = 0;
    for (i = 0; i < len/4; i++) {
        ptr_data[i] = drv_swap_byte_32(ptr_data[i]);
    }
    if(*data_tx == SPI_CMD_MOTION_UPLOAD)
    {
        printf("US:%02X%02X %02X%02X %02X%02X%02X%02X\r\n", \
            *(data_tx + 0), *(data_tx + 1), *(data_tx + 2), *(data_tx + 3),\
            *(data_tx + 4), *(data_tx + 5), *(data_tx + 6), *(data_tx + 7));
    }

    //ret = ak_drv_spi_write_read(SPI_ID, data_tx, data_rx, SPI_TRAN_SIZE, 1);
    ret =  ak_drv_spi_write_read(data_tx, data_rx, SPI_TRAN_SIZE);
    // check data received
    if(ret != 0)
    {
        for (i = 0; i < len/4; i++) 
        {
            p_data_recv[i] = drv_swap_byte_32(p_data_recv[i]);
        }

        // TODO: CC will change 0x03 to 0x83, we will remove 0x03 condition
        if((data_rx[0] & CC_SEND_SPI_PACKET_MASK) || \
            data_rx[0] == 0x03 || data_rx[0] == 0x0D || data_rx[0] == 0x15 \
            || data_rx[0] == 0x16 || data_rx[0] == 0x19 || data_rx[0] == 0x1E \
            || data_rx[0] == 0x1D || data_rx[0] == 0x1F)

        {
            if(spi_check_error(data_rx) == 0)
            {
                ucCmdType = data_rx[0] & (~CC_SEND_SPI_PACKET_MASK);
                if((ucCmdType >= CMD_VERSION) && (ucCmdType < MAX_PACKET_TYPE))
                {
                    ret = spi_process_received_data(data_rx);
                }
                else
                {
                    dzlog_error("%s unknow type: %d\r\n", __func__, ucCmdType);
                }
            }
            else
            {
                ak_print_error_ex("--->>>SPIChecksum failed! Type: 0x%02x\r\n", data_rx[0]);
            }
            if(*data_rx == SPI_CMD_MOTION_UPLOAD || \
                *data_rx == SPI_CMD_ACK_P2P_SD_STREAM || \
                *data_rx == SPI_CMD_FILE_TRANSFER)
            {
                //printf("UR:%02X%02X %02X%02X %02X%02X%02X%02X\r\n", \
                    *(data_rx + 0), *(data_rx + 1), *(data_rx + 2), *(data_rx + 3),\
                    *(data_rx + 4), *(data_rx + 5), *(data_rx + 6), *(data_rx + 7));
            }
        }
        // else
        // {
        //     printf("SPI Recv type: %x\r\n", data_rx[0]);
        // }
    }
    else
    {
        dzlog_error("SPI sent failed!\n");
    }
    return ret;
}

void spi_main_loop(void)
{
    int len = 0;
    unsigned long ulLastTick = 0, ulCurrentTick = 0;
    unsigned int u32CntPkt = 0;
    int iSendDummy1Second = 1;

    dzlog_info("spi_main_loop start!");
    ulLastTick = get_tick_count();
    for(;;){
        ulCurrentTick = get_tick_count();
        if((ulCurrentTick - ulLastTick) >= CFG_MONITOR_SPI_INTERVAL){
            ulLastTick = ulCurrentTick;
            u32CntPkt = (unsigned int)u32CntPkt/(CFG_MONITOR_SPI_INTERVAL/1000);
            if(eSysModeGet() != E_SYS_MODE_FTEST)
            {
                dzlog_warn("BW SPI %u KBps\n", u32CntPkt);
            }
            u32CntPkt = 0;
        }
		else if((ulCurrentTick - ulLastTick) >= CFG_MONITOR_SPI_SEND_DUMMY)
		{
			if(iSendDummy1Second)
			{
                iSendDummy1Second = 0;
            }
		}
        
        memset(ui8SpiTxBuffer, 0x00, SPI_TRAN_SIZE);
        /* Stream data */
        while(1)
        {
            len = spi_x_read_tx(&g_spi_data, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            if(len > 0){
                drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
				#if 0
				len = spi_x_read_tx(&g_spi_data, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
				if(len > 0){
					drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
				}					
                //u32CntPkt++;
				#endif
                u32CntPkt++;
                ak_sleep_ms(SPI_TRANSACTION_INTERVAL);
                // dzlog_debug("#");
            }
            else if(len == 0){
                // no data to send
                if(iSendDummy1Second){
                    memset(ui8SpiTxBuffer, 0x00, SPI_TRAN_SIZE);
                    drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
                }
                else{
					/*
                    if(eSysModeGet() != E_SYS_MODE_FTEST)
                    {
                        dzlog_debug("_");
                    }
                    */
                }
                ak_sleep_ms(3);  
                break;                      
            }
            else{
                dzlog_error("spi_x_read_tx failed, ret=%d\n", len);
                ak_sleep_ms(3);
                break;
            }
        }
        // ak_sleep_ms(3);
        /* Spi command */
    #ifdef CFG_SPI_COMMAND_ENABLE
        while(1)
        {
            len = spi_x_read_tx(&g_spi_cmd_buf, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            if(len > 0){
                drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
				#if 0
				len = spi_x_read_tx(&g_spi_cmd_buf, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            	if(len > 0){
                	drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
            	}
				//u32CntPkt++;
				#endif
                u32CntPkt++;
                ak_sleep_ms(SPI_TRANSACTION_INTERVAL);
                // ak_print_error("$$Response:%02X %02X\n", ui8SpiTxBuffer[0], ui8SpiTxBuffer[1]);
            }
            else if(len == 0){  
                break;                      
            }
            else{
                dzlog_error("spi cmd read to send failed, ret=%d\n", len);
                break;
            }
        }
        ak_sleep_ms(3);
    #endif

        while(1)
        {
            len = spi_x_read_tx(&g_spi_fwupg, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            if(len > 0){
                drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
				#if 0
				len = spi_x_read_tx(&g_spi_cmd_buf, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            	if(len > 0){
                	drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
            	}
                //u32CntPkt++;
				#endif
				u32CntPkt++;
				
                ak_sleep_ms(SPI_TRANSACTION_INTERVAL);
                // ak_print_error("##AKsent:%02X %02X\n", ui8SpiTxBuffer[0], ui8SpiTxBuffer[1]);
            }
            else if(len == 0){  
                break;                      
            }
            else{
                dzlog_error("SPIFW read buffer, ret=%d\n", len);
                break;
            }
        }
    
        while(1)
        {
            len = spi_x_read_tx(&g_spi_uploader, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            if(len > 0){
                drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
				#if 0
				len = spi_x_read_tx(&g_spi_cmd_buf, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            	if(len > 0){
                	drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
            	}
                u32CntPkt++;
				#endif
				u32CntPkt++;
                ak_sleep_ms(SPI_TRANSACTION_INTERVAL);
            }
            else if(len == 0){  
                break;                      
            }
            else{
                dzlog_error("SPI UPLOAD read buffer, ret=%d\n", len);
                break;
            }
        }
        // p2p sdcard streaming
        while(1)
        {
            len = spi_x_read_tx(&g_spi_p2psdstream, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            if(len > 0){
                drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
				#if 0
				len = spi_x_read_tx(&g_spi_cmd_buf, (char *)ui8SpiTxBuffer, SPI_TRAN_SIZE, 0);
            	if(len > 0){
                	drv_spi_master_send(ui8SpiTxBuffer, ui8SpiRxBuffer, SPI_TRAN_SIZE);
            	}
                u32CntPkt++;
				#endif
				u32CntPkt++;
                ak_sleep_ms(SPI_TRANSACTION_INTERVAL);
            }
            else if(len == 0){  
                break;                      
            }
            else{
                dzlog_error("SPI P2P read buffer, ret=%d\n", len);
                break;
            }
        }
    }
}



// This task will check data from ringbuffer and send via SPI
void *spi_task(void *arg)
{
    dzlog_info("[SPI] SPI layer init OK!\n\r");
    char acDummyData[MAX_SPI_PACKET_LEN] = {0x00, };
    int iNumOfSend = 3;
    int i = 0;
    int iRet = 0;

    /*tx_done = xQueueCreate(1, sizeof(u32));
    rx_done = xQueueCreate(1, sizeof(u32));*/
    //printf("tx_done: %x rx_done: %x \n", (u32)tx_done, (u32)rx_done);
    
    // init spi
    init_spi();

    g_iIsSpiTaskInit = 1;
    
    printf("time spi available:%u\r\n", get_tick_count());
    /* After initializing SPI, we must send dummy data to get CDS value */
    /* Maybe this sending dummy data will be disable in future, if
    Anyka chip reads CDS */
    memset(acDummyData, 0x00, sizeof(acDummyData));
    for(i = 0; i < iNumOfSend; i++)
    {
        iRet = spi_cmd_send(acDummyData, MAX_SPI_PACKET_LEN);
        if(iRet < 0)
        {
            dzlog_error("Send dummy FAILED! (%d)\r\n", i);
        }
    }
    dzlog_info("\nDone sending %d dummy get CDS value!\r\n", iNumOfSend);
    // main loop check data transfer
    spi_main_loop();

    // exit
    deinit_spi();
    g_iIsSpiTaskInit = 0;


    // ak_thread_join(ak_thread_get_tid());
    ak_thread_exit();
    return NULL;
}


int spi_get_status_task(void){
    return g_iIsSpiTaskInit;
}


/*
********************************************************************************
CLI for Testing spi command on fly
********************************************************************************
*/

static char *spi_cmd_help[]={
    "test spi command internal",
    "usage: spicmd <value1> <value2> <value3>\n"
    "       value:\n"
};

void cmd_spi_test_internal(int argc, char **args)
{

    char *pSpiData = NULL;
    int iRet = 0;
    int iAllValue = 0, iRtspValue = 0, iP2pValue = 0, iCfgValue = 0;

    if(argc <= 0)
    {
        ak_print_error_ex("Argument is invalid!\r\n");
    }
    else
    {
        pSpiData = (char *)malloc(sizeof(char) * MAX_SPI_PACKET_LEN);
        memset(pSpiData, 0x00, MAX_SPI_PACKET_LEN);
        // if(args[0] == 0x01)
        // {
            *(pSpiData + 0) = SPI_CMD_FACTORY_TEST;
            *(pSpiData + 1) = 0x01; // turn on light
            *(pSpiData + 2) = 0x01; // turn on tone gen
            *(pSpiData + 3) = 0x03; // 0x03E8 freq 1000Hz
            *(pSpiData + 4) = 0xE8;
            *(pSpiData + 5) = 0x01; // 0x017B bitrate actualy
            *(pSpiData + 6) = 0x7B;
        // }   

        iRet = spi_process_received_data(pSpiData);
        if(iRet == 0){
            ak_print_normal("OK\n");
        }
        else
        {
            ak_print_error_ex("Failed!\r\n");
        }
    }
    return;
}

#if 1
static int cmd_test_spi_internal_reg(void)
{
    cmd_register("spicmd", cmd_spi_test_internal, spi_cmd_help);
    return 0;
}
cmd_module_init(cmd_test_spi_internal_reg)
#else
cmd_module_init(cmd_spi_test_internal)
#endif


/*
********************************************************************************
*                            END OF FILE                                       *
********************************************************************************
*/

