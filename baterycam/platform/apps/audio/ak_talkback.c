/**
  ******************************************************************************
  * File Name          : ak_talkback.c
  * Description        : This file contain functions for ak_talkback
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

#include "ak_talkback.h"

#include "ak_apps_config.h"
#include "parse_frame_h264.h"

#include "g722_decoder.h"
#include "ak_audio.h"

#include "spi_transfer.h"

#include "ringbuf_1.h"
#ifdef CFG_USE_SCHEDULER
#include "transfer_scheduler.h"
#endif 

#include "packet_queue.h"       // use API from packet_queue
#include "ak_adec.h"
#include "main_ctrl.h"

#include "doorbell_config.h"

#include "sdcard.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/

extern ring_buffer_t *g_pRbLoopback;

extern int g_iP2pSdStreamStart;
//TODO: one way talkback
int g_iOneWayTalkback = 0;
#define ONE_WAY_TALKBACK_THRESOLD           12
#define TIMEOUT_ONE_WAY_ENABLE_AUDIO           500 // ms

extern int g_play_dingdong;

/* Update volume */
void *g_talkback_handle;
extern char g_voicevalue;


//int g_up=0;
extern int g_receive_update;
extern unsigned char Beep_Data[];
extern unsigned char Ding_Data[];


extern unsigned char Success_Data_CH[];
extern unsigned char Fail_Data_CH[];
extern unsigned char Timeout_Data_CH[];
extern unsigned char CH_SORRYNOTFREETOMEETNOW[];
extern unsigned char CH_YOUCANLEAVEIT[];
extern unsigned char CH_WElCOMECOMINGOVERNOW[];
extern unsigned char CH_HOWCANIHELP[];
extern unsigned char CH_SORRYNOTATHOME[];


extern unsigned char Success_Data[];
extern unsigned char Fail_Data[];
extern unsigned char Timeout_Data[];
extern unsigned char SORRYNOTFREETOMEETNOW[];
extern unsigned char YOUCANLEAVEIT[];
extern unsigned char WElCOMECOMINGOVERNOW[];
extern unsigned char WHOAREYOULOOKINGFOR[];

int16_t temp_data[150*1024]={0};

//#ifdef DOORBELL_TINKLE820
//#else
extern unsigned char HOWCANIHELP[];
extern unsigned char SORRYNOTATHOME[];
//#endif

//extern unsigned char EN_CONNECTED[];
extern unsigned char EN_UNABLETOCONNECT[];
extern unsigned char EN_WARNINGWIFIISWEAK[];




static unsigned char *pucDataDingDong = NULL;
static unsigned long ulLenDataDingDong = 0;

#ifdef CFG_TALKBACK_G722
G722_DEC_CTX *g722_dctx;
#endif

char acBufRecv[4*1024];   // receive data g722 encrypt from CC over SPI

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
// static ring_buffer_t *pRingBuf1;
static ring_buffer_t *g_pRbTalkbackBuffer;
#define RINGBUF_TALKBACK_BUFFER_LEN             (8*1024)  // 2Kbyte
#define LEN_DATA_PLAY                           (2*1024)  // buffer 2Kb


static void play_dingdong_mp3(void);
// static void play_dingdong_pcm(void);
/*---------------------------------------------------------------------------*/
static void * mount_open_file(void);
static void unmount_close_file(FILE *filp);
static int fake_data_play_speaker(void *file, char *pData, int *pLen);


/*---------------------------------------------------------------------------*/

static void tb_init_sd(void)
{
#ifdef CFG_DEBUG_TALKBACK_WITH_SDCARD
    sdcard_mount();
#endif
}

static FILE *tb_open_file(void)
{
#ifdef CFG_DEBUG_TALKBACK_WITH_SDCARD
    FILE *fd = fopen("talkback_debug.pcm", "w" );
	if (fd == NULL)
	{
		ak_print_error("open file failed!\n");
	}
	else
	{
		ak_print_normal("open file success!\n");
	}
    return fd;
#else
    return NULL;
#endif
}

static int tb_read_file(FILE *fd, char *p_data, unsigned int size)
{
#ifdef CFG_DEBUG_TALKBACK_WITH_SDCARD
    int ret = 0;

    if(fd == NULL || p_data == NULL)
    {
        return -1;
    }
    ret = fread(p_data, size, 1, fd);
    return ret;
#else
    return 0;
#endif
}

static int tb_write_file(FILE *fd, char *p_data, unsigned int size)
{
#ifdef CFG_DEBUG_TALKBACK_WITH_SDCARD
    int ret = size;
    do {
        ret -= fwrite(p_data, 1, ret, fd);
    } while (ret > 0);
    return size;
#else
    return 0;
#endif
}


static void tb_close_umount_file_record(FILE *filp)
{
    fclose(filp);
    // ak_unmount_fs(1, 0, "a:");
    sdcard_umount();
}


/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/

static int init_ringbuffer_talkback_issue(void)
{
    int iRet = 0;
    g_pRbTalkbackBuffer = Ringbuf_Init(RINGBUF_TALKBACK_BUFFER_LEN);
    if(g_pRbTalkbackBuffer == NULL)
    {
        ak_print_error("%s g_pRbTalkbackBuffer NULL!\r\n", __func__);
        iRet = -1;
    }
    else
    {
        ak_print_normal("%s initialized buffer with %d bytes\r\n", \
                            __func__, RINGBUF_TALKBACK_BUFFER_LEN);
    }
    return iRet;
}

static int deinit_ringbuffer_talkback_issue(void)
{
    int iRet = 0;
    if(g_pRbTalkbackBuffer)
    {
        Ringbuf_Free(g_pRbTalkbackBuffer);
        ak_print_normal("%s free g_pRbTalkbackBuffer\r\n", __func__);
    }
    return iRet;
}


static int talkback_decoder_init(void)
{
    int s32Ret = 0;
#ifdef CFG_TALKBACK_G722
    g722_dctx = g722_decoder_new(64000,0);
    if (g722_dctx == NULL)
    {
       printf("\ng722_decoder_new() failed\r\n");
    }
#endif
    return s32Ret;
}

//static int talkback_decoder_deinit(void)
//{
//    g722_dec_deinit(&g_dec_g722);
//    return 0;
//}


static int talkback_decode_data(char *i8DataIn, int i32LenIn, char *i8DataOut, int *i32LenOut)
{
    int s32Ret = 0;
    char i8DataDecode[4*1024];
    unsigned int u32NSample = 0;
    int16_t *ptr_16=(int16_t *)i8DataDecode;
    int i, max_signal=0, temp;
    if(i8DataIn == NULL || i8DataOut == NULL)
    {
        s32Ret = -1;
        printf("[ERR] Agrument NULL\n\r");
    }
    else
    {
#ifdef CFG_TALKBACK_G722
        //printf("[G722] u32NSample: %d\n\r", u32NSample);
    	u32NSample = g722_decode(g722_dctx,i8DataIn,i32LenIn,(int16_t *)i8DataDecode);
        for(i=0;i<u32NSample;i++){
            if (max_signal<ptr_16[i])
                max_signal = ptr_16[i];
        }
        if (max_signal>8000){
            for(i=0;i<u32NSample;i++){
                temp=ptr_16[i];
                temp=temp*8000;
                temp=temp/max_signal;
                ptr_16[i]=temp;
            }
        }
        ak_print_normal("Max spk signal: %d\r\n", max_signal);
        memcpy(i8DataOut, i8DataDecode, u32NSample*sizeof(int16_t));
        *i32LenOut = u32NSample*sizeof(int16_t);
#endif
    }
    return s32Ret;
}

/*
Read value from config
Set volume
*/
int talkback_ao_set_volume(void *pAoHandle)
{
    int iRet_a = 0, iRet_d = 0, iRet = 0;
    int iValueSpkd = 0, iValueSpka = 0;

    // volume analog
    iRet_a = doorbell_cfg_get_spka_gain(&iValueSpka);
    if(iRet_a == 0)
    {
        ak_print_normal("Read Speaker: %d\r\n", iValueSpka);
        ak_ao_set_volume(pAoHandle, iValueSpka);
    }
    else
    {
        iValueSpka = SYS_MICSPK_DIGITAL_MAX - 2;
        ak_print_normal("Set max volume to Speaker (%d)\r\n", iValueSpka);
        ak_ao_set_volume(pAoHandle, iValueSpka);
        iRet = 1;
    }

    /*
    We don't need to use this snipcode to set digital. 
    Because API setting analog will set value from 0 to 12.
    */
#if 0
    // volume digital
    iRet_d = doorbell_cfg_get_spkd_gain(&iValueSpkd);
    if(iRet_d == 0)
    {
        ak_print_normal("Read Speaker digital: %d\r\n", iValueSpkd);
        ak_ao_set_volume(pAoHandle, iValueSpkd);
    }
    else
    {
        ak_print_normal("Set max volume to Speaker digital (%d)\r\n", SYS_MICSPK_DIGITAL_MAX);
        ak_ao_set_volume(pAoHandle, SYS_MICSPK_DIGITAL_MAX - 2);
        iRet += 1;
    }
#endif
    return iRet;
}

/* API set speaker volume on flight */
int talkback_set_volume_onflight(void)
{
    int iRet = 0;
    if(g_talkback_handle)
    {
        iRet = talkback_ao_set_volume(g_talkback_handle);
    }
    return iRet;            
}

/* API set speaker volume on flight */
int talkback_set_volume_onflight_with_value(int iValue)
{
    int iRet = 0;
    if(g_talkback_handle)
    {
        iRet = ak_ao_set_volume(g_talkback_handle, iValue);
        ak_print_normal("%s value: %d (%d)\r\n", __func__, iValue, iRet);
    }
    return iRet;            
}

int talkback_init(void **pp_ao_handler)
{
    int i32Ret = 0;
    struct pcm_param param = {0}; // ref ak_adec_demo.c

    param.sample_bits = CFG_TALKBACK_SAMPLE_BITS;
    param.sample_rate = CFG_TALKBACK_SAMPLE_RATE;
    param.channel_num = AUDIO_CHANNEL_MONO; //CFG_TALKBACK_CHANNEL_NUM; //mono

    *pp_ao_handler = ak_ao_open(&param);
    if(*pp_ao_handler == NULL)
    {
        ak_print_error("ao open false\n");
        i32Ret = -1;
    }
    else
    {
        //set volume
        // ak_ao_set_volume(*pp_ao_handler, CFG_TALKBACK_SPEAKER_VL);
        if(talkback_ao_set_volume(*pp_ao_handler) != 0)
        {
            ak_print_error_ex("AO set volume failed!\r\n");
        }
		/* Trung: disable it, only tunr on if talkback */
        // ak_ao_enable_speaker(*pp_ao_handler, SPK_ENABLE); // active low
        ak_ao_enable_speaker(*pp_ao_handler, SPK_ENABLE);
        ak_print_error_ex("ak_ao_enable_speaker SPK_DISABLE\r\n");
    }
    ak_print_normal("---->>>%s handler: %x\n", __func__, *pp_ao_handler);

    return i32Ret;
}

int talkback_deinit(void **pp_ao_handler)
{
    int i32Ret = 0;
    if(*pp_ao_handler != NULL)
    {
        ak_ao_enable_speaker(*pp_ao_handler, SPK_DISABLE);
        // ak_print_error_ex("ak_ao_enable_speaker active high (close) for FM\r\n");
        ak_ao_close(*pp_ao_handler);
    }
    return i32Ret;
}

int talkback_enc_init(void **pp_aenc_handler)
{
    int i32Ret = 0;
    /* open audio encode */
	struct audio_param adec_param = {0};
	adec_param.sample_bits = CFG_AUDIO_SAMPLE_BITS;
	adec_param.channel_num = AUDIO_CHANNEL_STEREO; // AUDIO_CHANNEL_MONO; // ;    // same demo adec
	adec_param.sample_rate = CFG_AUDIO_SAMPLE_RATE;
    adec_param.type = AK_AUDIO_TYPE_PCM;

    *pp_aenc_handler = ak_adec_open(&adec_param);
	if(NULL == *pp_aenc_handler) {
    	ak_print_error("ao enc open false\n");
        i32Ret = -1;
    }
    else
    {
        ak_print_normal("AO ENC OK!\n");
    }
    return i32Ret;
}
int talkback_enc_deinit(void **pp_aenc_handler)
{
    int i32Ret = 0;
    if(*pp_aenc_handler != NULL)
    {
        ak_adec_close(*pp_aenc_handler);
    }
    return i32Ret;
}
///////////////////////////////////////////////////////////////////////////////

char acReadDataSpi[SPI_TRAN_SIZE];
static unsigned int g_u32LastPid = 0;

int talkback_read_data_fast(char *p_data, int *p_out_len)
{
    int i32Ret = 0;
    int i32RetRdSpi = 0;
    int i32RetCreatePacket = 0;

    packet_header_t *ptr_header = NULL; // parse header SPI
    char *pDataWannaDescrypt = NULL;
    int i32DataWannaDescryptLength = 0;

    int i32Length = SPI_TRAN_SIZE;
    unsigned int u32LenDataRecv = 0;

    if(p_data == NULL || p_out_len == NULL)
    {
        i32Ret = -1;
    }
    else
    {
        memset(acReadDataSpi, 0x00, sizeof(acReadDataSpi));
        i32RetRdSpi = read_spi_data(acReadDataSpi, SPI_TRAN_SIZE);
        if(i32RetRdSpi > 0)
        {
            i32RetCreatePacket = create_packet(&ptr_header, acReadDataSpi, i32RetRdSpi);
            if (i32RetCreatePacket < 0 || ptr_header == NULL) 
            {
                i32Ret = -2;
                ak_print_error("%s Create packet failed!\n", __func__);
            }
            else
            {
                #if 0
                if(g_u32LastPid != 0) // for other packet which is not the fist packet
                {
                    if(ptr_header->PID > g_u32LastPid)
                    {
                        //---------------------------------------------------------------------------------------
                        g_u32LastPid = ptr_header->PID;
                        pDataWannaDescrypt = (char *)ptr_header->packet_data;
                        i32DataWannaDescryptLength = ptr_header->packet_size;
                        i32Ret = packet_descrypt(pDataWannaDescrypt, i32DataWannaDescryptLength);
                        if(i32Ret != 0)
                        {
                            ak_print_error("Wanna Decrypt failed!\n");
                            i32Ret = -3;
                        }
                        else
                        {
                            talkback_decode_data(pDataWannaDescrypt, i32DataWannaDescryptLength, p_data, p_out_len);
                            // #ifndef UPLOADER_DEBUG
                            dzlog_info("Talk: in:%d, out:%d\n", i32DataWannaDescryptLength, *p_out_len);
                            // #endif
                        }
                        //---------------------------------------------------------------------------------------
                    }
                    else
                    {
                        dzlog_info("Old pid: %d\r\n", ptr_header->PID);
                    }
                }
                else // for first time, we always get and play it
                {
                    //---------------------------------------------------------------------------------------
                    g_u32LastPid = ptr_header->PID;
                    pDataWannaDescrypt = (char *)ptr_header->packet_data;
                    i32DataWannaDescryptLength = ptr_header->packet_size;
                    i32Ret = packet_descrypt(pDataWannaDescrypt, i32DataWannaDescryptLength);
                    if(i32Ret != 0)
                    {
                        ak_print_error("Wanna Decrypt failed!\n");
                        i32Ret = -3;
                    }
                    else
                    {
                        talkback_decode_data(pDataWannaDescrypt, i32DataWannaDescryptLength, p_data, p_out_len);
                        // #ifndef UPLOADER_DEBUG
                        dzlog_info("Talk: in:%d, out:%d\n", i32DataWannaDescryptLength, *p_out_len);
                        // #endif
                    }
                    //---------------------------------------------------------------------------------------
                } // if(g_u32LastPid != 0)
                #else // process old pid
                pDataWannaDescrypt = (char *)ptr_header->packet_data;
                i32DataWannaDescryptLength = ptr_header->packet_size;
                i32Ret = packet_descrypt(pDataWannaDescrypt, i32DataWannaDescryptLength);
                if(i32Ret != 0)
                {
                    ak_print_error("Wanna Decrypt failed!\n");
                    i32Ret = -3;
                }
                else
                {
                    talkback_decode_data(pDataWannaDescrypt, i32DataWannaDescryptLength, p_data, p_out_len);
                    #ifndef UPLOADER_DEBUG
                    dzlog_info("Talk: in:%d, out:%d\n", i32DataWannaDescryptLength, *p_out_len);
                    #endif
                }
                #endif
                delete_packet(ptr_header);
            } // create packet
        } // read spi data
        else
        {
            // no data talkback
            i32Ret = -4;
        }
    }
    return i32Ret;
}

#ifndef SYS_USE_DINGDONG_WAV_1S
#else

void prepare_data_play_dingdong(void)
{
    void *pPartition = NULL;
    unsigned long ulLenPartition = 0;
    int iLenRead = 0;
    unsigned long ulTick = get_tick_count();
    ak_print_normal("%s open MUSIC from %u \n", __func__, ulTick);
    // open partition
    pPartition = ak_partition_open("MUSIC");
    if(pPartition == NULL)
    {
        ak_print_error_ex("Open partition MUSIC failed!\n");
        return;
    }
    // read length partition
    ulLenPartition = ak_partition_get_dat_size(pPartition);
    ak_print_normal("MUSIC: len: %u\n", ulLenPartition);
    // read data
    if(ulLenPartition > 0)
    {
        ulLenDataDingDong = ulLenPartition;
        pucDataDingDong = (unsigned char *)malloc(ulLenPartition);
        if(pucDataDingDong == NULL)
        {
            ak_print_error_ex("MUSIC: malloc failed! Close MUSIC Partition\n");
        }
        else
        {
            memset(pucDataDingDong, 0x00, ulLenDataDingDong);
            iLenRead = ak_partition_read(pPartition, (char *)pucDataDingDong, ulLenPartition);
            memset(pucDataDingDong, 0x00, 64);// Remove header, 44 round to to 64
            memset(pucDataDingDong+ulLenPartition-128, 0x00, 128);// 108 round up to 128
            if(iLenRead <= 0)
            {
                free(pucDataDingDong);
                ak_print_error_ex("Read partition error!\n");
                pucDataDingDong = NULL;
            } else {
                int16_t *ptr_16=(int16_t *)pucDataDingDong;
                int l_len = iLenRead/2;
                int l_max=0;
                int i;
                int l_temp;
                for (i=32;i<(l_len-64);i++){
                   if (l_max<ptr_16[i])
                       l_max=ptr_16[i];
                }
                if (l_max>(0x7FFF/6)){
                    l_temp = l_max/(0x7FFF/6);
                    for (i=0;i<l_len;i++){
                       ptr_16[i]=ptr_16[i]/l_temp;
                    }
                }   
            }
        }
    }
    else
    {
        ak_print_error_ex("Read MUSIC failed!!!\r\n");
    }
    ak_print_normal("MUSIC: close partition! Loaded %u bytes in %u\n", \
        ulLenPartition, get_tick_count() - ulTick);
    ak_partition_close(pPartition);
}
static void play_dingdong_pcm2(void *p_ao_handle)
{
    unsigned long ulTotalPlay = 0;
    int iNumOfRead = 0;
    int iLastRead = 0;

    int iLenRead = 0;
    int iPlaySize = 0;
    unsigned char *pData = NULL;
    int i;
    
    ak_print_normal("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
    ak_print_normal("Time play %u\r\n", get_tick_count());
    ak_print_normal("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");

    pData = pucDataDingDong;
    iLenRead = ulLenDataDingDong;
//    iNumOfRead = ulLenDataDingDong/1024;
//    iLastRead = ulLenDataDingDong - (iNumOfRead * 1024);
    ak_print_normal("MUSIC: len: %u, num read: %d, iLastRead: %d\n", \
                            ulLenDataDingDong, iNumOfRead, iLastRead);
    // read data
    if(ulLenDataDingDong > 0 && pucDataDingDong)
    {
        while(iLenRead > 255)
        {
//            ak_sleep_ms(30);
//            iNumOfRead--;
//            if(iNumOfRead == 0)
//            {
//                break;
//                iPlaySize = ak_ao_send_frame(p_ao_handle, pData , iLastRead, 0);
//                iLastRead -= iPlaySize;
//            }
//            else
//            {
//                iPlaySize = ak_ao_send_frame(p_ao_handle, pData , 1024 , 0);
//            }
            if (i%8==7)
            	ak_sleep_ms(30);
            i++;
        	if(iLenRead < 0)
        	{
        		ak_print_normal("\niLenRead_exit:%d\r\n",iLenRead);
        		break;
        	}
        	iPlaySize = ak_ao_send_frame(p_ao_handle, pData , 256 , 0);
        	if (iPlaySize<=0)
        		ak_sleep_ms(100);
            if(iPlaySize < 0)
            {
                ak_print_error_ex("write data music to DA error!\n");
                break;
            }
            else
            {
            	ulTotalPlay += iPlaySize;
            // #ifndef UPLOADER_DEBUG
            // ak_print_normal("Read: %d, play: %d, total: %d\n", iLenRead, iPlaySize, ulTotalPlay);
            // #endif
            	pData += iPlaySize;
            	iLenRead = iLenRead - iPlaySize;
            }

        }
    }
    ak_print_normal("\r\nMUSIC exit at %u\r\n", get_tick_count());
}
static void play_dingdong_pcm(void *p_ao_handle)
{
    void *pPartition = NULL;
    unsigned long ulLenPartition = 0;
    unsigned long ulTotalPlay = 0;
    int iNumOfRead = 0;
    int iLastRead = 0;

    int iLenRead = 0;
    int iPlaySize = 0;
    unsigned char *pData = NULL;

    // ak_print_normal("%s time 7A %d \n", __func__, get_tick_count());
    ak_print_normal("%s time 7B %d \n", __func__, get_tick_count());
    // open partition
    pPartition = ak_partition_open("MUSIC");
    if(pPartition == NULL)
    {
        ak_print_error_ex("Open partition MUSIC failed!\n");
        return;
    }
    
    // read length partition
    ulLenPartition = ak_partition_get_dat_size(pPartition);
    iNumOfRead = ulLenPartition/1024;
    iLastRead = ulLenPartition - (iNumOfRead * 1024);
    ak_print_normal("MUSIC: len: %u, num read: %d, iLastRead: %d\n", \
                            ulLenPartition, iNumOfRead, iLastRead);
    // read data
    if(ulLenPartition > 0)
    {
        pData = (unsigned char *)malloc(ulLenPartition);
        if(pData == NULL)
        {
            ak_print_error_ex("MUSIC: malloc failed!\n");
            goto exit_part;
        }
        else
        {
            memset(pData, 0x00, CFG_PLAY_MUSIC_BUF_SIZE);
            iLenRead = ak_partition_read(pPartition, (char *)pData, ulLenPartition);
            // play data
            if(iLenRead > 0)
            {
                while(iLenRead > 0)
                {
                    ak_sleep_ms(10);
                    iNumOfRead--;
                    if(iNumOfRead == 0)
                    {
                        iPlaySize = ak_ao_send_frame(p_ao_handle, pData , iLastRead, 0);
                        iLastRead -= iPlaySize;
                    }
                    else
                    {
                        iPlaySize = ak_ao_send_frame(p_ao_handle, pData , 1024, 0);
                    }
                    // iPlaySize = iLenRead;
                    if(iPlaySize < 0)
                    {
                        ak_print_error_ex("write data mp3 to DA error!\n");
                        break;
                    }
                    #ifndef UPLOADER_DEBUG
                    ak_print_normal("Read: %d, play: %d, total: %d\n", iLenRead, iPlaySize, ulTotalPlay);
                    #endif
                    iLenRead = iLenRead - iPlaySize;
                    pData += iPlaySize;
                    
                }
            }
            else
            {
                ak_print_error_ex("end of file!\n");
                ak_print_error("read, %s\n", strerror(errno));
                // break;
            }
            // ak_sleep_ms(20);
            free(pData);

        }//if(pData == NULL)
    }
exit_part:
    ak_print_normal("MUSIC: exit partition!\n");
    ak_partition_close(pPartition);
    ak_print_normal("MUSIC exit");
}

int aktb_backup_data_audio(char *pData, int len)
{
    int iRet = 0;
    if(g_iOneWayTalkback == 0)
    {
        iRet = Ringbuf_WriteBuf(g_pRbTalkbackBuffer, (unsigned char *)pData, len);
    }
    return iRet;
}

static int aktb_play_data_over_spk(void *pHandle, char *pData, int len)
{
    int iRet = 0, iNumOfRead = 0;
    int iSizeData = len;
    int iSendSize = 0, i = 0, volume_max = 0, volume_temp = 0;
    int16_t *temp_ptr = pData;
//    iNumOfRead = iSizeData/256;
    if(pHandle == NULL || pData == NULL || len <= 0)
    {
        return -1;
    }

    for(i=0;i<(len/2);i++)
    {
        if (volume_max<temp_ptr[i])
        {
            volume_max = temp_ptr[i];
        }
//        ak_print_normal("\nvolume_max_out:%d\r\n",volume_max);
    }
    if (volume_max>(0x7FFF/14))
    {
        volume_temp = volume_max/(0x7FFF/14);
        for (i=0;i<(len/2);i++)
        {
        	temp_ptr[i]=temp_ptr[i]/volume_temp;
        }
    }
    for(i=0;i<(len/2);i++)
    {
    	temp_data[i*2]=temp_ptr[i+44];
    	temp_data[i*2+1]=temp_ptr[i+44];
    }

    iSizeData = (iSizeData - 44) * 2;
    ak_print_normal("\n(iSizeData - 44 byte)*2:%d\r\n",iSizeData);
    pData=temp_data;
    while(iSizeData > 255)
    {
//    	iNumOfRead --;
//    	if(iNumOfRead == 0)
//    	{
//    		break;
//    	}
//    	else
//    	{
//    		iRet = ak_ao_send_frame(pHandle, pData + iSendSize, 256, 0);
//    	}
    	iRet = ak_ao_send_frame(pHandle, pData , 256, 0);
        if(iRet < 0)
        {
            ak_print_error("ao send frame error! (%d) \n", iRet);
            break;
        }
        else
        {
        	pData += iRet;
            iSizeData = iSizeData - iRet;
        }
        ak_sleep_ms(5);
    }
    if(iRet >= 0)
    {
        iRet = len;
    }
    printf("\n&&&&&&&&&&&&&&&&&& END END END\ $$$$$$$$$$$$$$$$\r\n");
    return iRet;
}

static int aktb_play_data_over_spk1(void *pHandle, char *pData, int len)
{
    int iRet = 0;
    int iSizeData = len;
    int iSendSize = 0;
    if(pHandle == NULL || pData == NULL || len <= 0)
    {
        return -1;
    }
    while(iSizeData > 0)
    {
        iRet = ak_ao_send_frame(pHandle, pData + iSendSize, iSizeData, 0);
        if(iRet < 0)
        {
            ak_print_error("ao send frame error! (%d) \n", iRet);
            break;
        }
        else
        {
            iSendSize += iRet;
            iSizeData = iSizeData - iRet;
            // printf("P%d ", iRet);
        }
        ak_sleep_ms(5);
    }
    if(iRet >= 0)
    {
        iRet = len;
    }
    return iRet;
}

//Thuan add - 18/10/2017
static int ao_send_frame_maintask(void * ao_handle)
{
    int ret = -1, i = 0,iPlaySize = 0;

    ak_print_normal("\nSTART SELECT TYPE CMD PLAY\r\n");
    ak_print_normal("TYPE: %02x\r\n", g_voicevalue);
    if(g_voicevalue == SPICMD_TYPE_VOICE_BEEP)
    {
    	ak_print_normal("size_len_beep_data: %d\r\n",LEN_BEEP);
    	ret = aktb_play_data_over_spk(ao_handle,Beep_Data,LEN_BEEP);
    	g_receive_update = 0;
    	ak_print_normal("Beep Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_VOICE_JINGLE)
    {
    	ak_print_normal("size_len_jingle_data: %d\r\n",LEN_DING);
    	ret = aktb_play_data_over_spk(ao_handle,Ding_Data,LEN_DING);
    	g_receive_update = 0;
    	ak_print_normal("Jingle Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_FAIL)
    {
        ak_print_normal("len_fail_CH: %d\r\n",LEN_CHFAIL);
        ret = aktb_play_data_over_spk(ao_handle,Fail_Data_CH,LEN_CHFAIL);
        g_receive_update = 0;
        ak_print_normal("CH fail Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_SUCCESS)
    {
        ak_print_normal("size_len_success_data: %d\r\n",LEN_CHSUCCESS);
        ret = aktb_play_data_over_spk(ao_handle,Success_Data_CH,LEN_CHSUCCESS);
        g_receive_update = 0;
        ak_print_normal("Success Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_TIMEOUT)
    {
        ak_print_normal("len_timeout_CH: %d\r\n",LEN_CHTIMEOUT);
        ret = aktb_play_data_over_spk(ao_handle,Timeout_Data_CH,LEN_CHTIMEOUT);
        g_receive_update = 0;
        ak_print_normal("CH timeout Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_SORRYNOTFREETOMEETNOW)
    {
    	ak_print_normal("LEN_CH_SORRYNOTATHOME: %d\r\n",LEN_CH_SORRYNOTFREETOMEETNOW);
    	ret = aktb_play_data_over_spk(ao_handle,CH_SORRYNOTFREETOMEETNOW,LEN_CH_SORRYNOTFREETOMEETNOW);
    	g_receive_update = 0;
    	ak_print_normal("CH SORRYNOTATHOME PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_YOUCANLEAVEIT)
    {
    	ak_print_normal("LEN_CH_YOUCANLEAVEIT: %d\r\n",LEN_CH_YOUCANLEAVEIT);
    	ret = aktb_play_data_over_spk(ao_handle,CH_YOUCANLEAVEIT,LEN_CH_YOUCANLEAVEIT);
    	g_receive_update = 0;
    	ak_print_normal("CH YOUCANLEAVEIT PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_WElCOMECOMINGOVERNOW)
    {
    	ak_print_normal("LEN_CH_WElCOMECOMINGOVERNOW: %d\r\n",LEN_CH_WElCOMECOMINGOVERNOW);
    	ret = aktb_play_data_over_spk(ao_handle,CH_WElCOMECOMINGOVERNOW,LEN_CH_WElCOMECOMINGOVERNOW);
    	g_receive_update = 0;
    	ak_print_normal("CH YOUCANLEAVEIT PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_HOWCANIHELP)
    {
    	ak_print_normal("LEN_CH_HOWCANIHELP: %d\r\n",LEN_CH_HOWCANIHELP);
    	ret = aktb_play_data_over_spk(ao_handle,CH_HOWCANIHELP,LEN_CH_HOWCANIHELP);
    	g_receive_update = 0;
    	ak_print_normal("CH HOWCANIHELP PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_SORRYNOTATHOME)
    {
    	ak_print_normal("LEN_CH_SORRYNOTATHOME: %d\r\n",LEN_CH_SORRYNOTATHOME);
    	ret = aktb_play_data_over_spk(ao_handle,CH_SORRYNOTATHOME,LEN_CH_SORRYNOTATHOME);
    	g_receive_update = 0;
    	ak_print_normal("CH SORRYNOTATHOME PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_CONNECTED)
    {
    	g_receive_update = 0;
    	ak_print_normal("CH CONNECTED PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_UNABLETOCONNECT)
    {

    	g_receive_update = 0;
    	ak_print_normal("CH UNABLETOCONNECT PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_WARNINGWIFIISWEAK)
    {

    	g_receive_update = 0;
    	ak_print_normal("CH WARNINGWIFIISWEAK PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_SUCCESS)
    {
        ak_print_normal("size_len_success_data: %d\r\n",LEN_SUCCESS);
        ret = aktb_play_data_over_spk(ao_handle,Success_Data,LEN_SUCCESS);
        g_receive_update = 0;
        ak_print_normal("Success Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_FAIL)
    {
        ak_print_normal("size_len_fail_data: %d\r\n",LEN_FAIL);
        ret = aktb_play_data_over_spk(ao_handle,Fail_Data,LEN_FAIL);
        g_receive_update = 0;
        ak_print_normal("Fail Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_TIMEOUT)
    {
        ak_print_normal("size_len_time_data: %d\r\n",LEN_TIMEOUT);
        ret = aktb_play_data_over_spk(ao_handle,Timeout_Data,LEN_TIMEOUT);
        g_receive_update = 0;
        ak_print_normal("Timeout Play Done\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_SORRYNOTFREETOMEETNOW)
    {
    	ak_print_normal("LEN_SORRYNOTFREETOMEETNOW: %d\r\n",LEN_SORRYNOTFREETOMEETNOW);
    	ret = aktb_play_data_over_spk(ao_handle,SORRYNOTFREETOMEETNOW,LEN_SORRYNOTFREETOMEETNOW);
    	g_receive_update = 0;
    	ak_print_normal("EN SORRYNOTFREETOMEETNOW PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_YOUCANLEAVEIT)
    {
    	ak_print_normal("LEN_YOUCANLEAVEIT: %d\r\n",LEN_YOUCANLEAVEIT);
    	ret = aktb_play_data_over_spk(ao_handle,YOUCANLEAVEIT,LEN_YOUCANLEAVEIT);
    	g_receive_update = 0;
    	ak_print_normal("EN YOUCANLEAVEIT PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_WElCOMECOMINGOVERNOW)
    {
    	ak_print_normal("LEN_SORRYNOTFREETOMEETNOW: %d\r\n",LEN_WElCOMECOMINGOVERNOW);
    	ret = aktb_play_data_over_spk(ao_handle,WElCOMECOMINGOVERNOW,LEN_WElCOMECOMINGOVERNOW);
    	g_receive_update = 0;
    	ak_print_normal("EN SORRYNOTFREETOMEETNOW PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_WHOAREYOULOOKINGFOR)
    {
    	ak_print_normal("LEN_WHOAREYOULOOKINGFOR: %d\r\n",LEN_WHOAREYOULOOKINGFOR);
    	ret = aktb_play_data_over_spk(ao_handle,WHOAREYOULOOKINGFOR,LEN_WHOAREYOULOOKINGFOR);
    	g_receive_update = 0;
    	ak_print_normal("EN WHOAREYOULOOKINGFOR PLAY DONE\r\n");
    }
//#ifdef DOORBELL_TINKLE820
//#else
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_HOWCANIHELP)
    {
    	ak_print_normal("LEN_HOWCANIHELP: %d\r\n",LEN_HOWCANIHELP);
    	ret = aktb_play_data_over_spk(ao_handle,HOWCANIHELP,LEN_HOWCANIHELP);
    	g_receive_update = 0;
    	ak_print_normal("EN HOWCANIHELP PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_SORRYNOTATHOME)
    {
    	ak_print_normal("LEN_SORRYNOTATHOME: %d\r\n",LEN_SORRYNOTATHOME);
    	ret = aktb_play_data_over_spk(ao_handle,SORRYNOTATHOME,LEN_SORRYNOTATHOME);
    	g_receive_update = 0;
    	ak_print_normal("EN SORRYNOTATHOME PLAY DONE\r\n");
    }
//#endif
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_UNABLETOCONNECT)
    {
    	ak_print_normal("LEN_EN_UNABLETOCONNECT: %d\r\n",LEN_EN_UNABLETOCONNECT);
    	ret = aktb_play_data_over_spk(ao_handle,EN_UNABLETOCONNECT,LEN_EN_UNABLETOCONNECT);
    	g_receive_update = 0;
    	ak_print_normal("EN UNABLETOCONNECT PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_WARNINGWIFIISWEAK)
    {
    	ak_print_normal("LEN_EN_WARNINGWIFIISWEAK: %d\r\n",LEN_EN_WARNINGWIFIISWEAK);
    	ret = aktb_play_data_over_spk(ao_handle,EN_WARNINGWIFIISWEAK,LEN_EN_WARNINGWIFIISWEAK);
    	g_receive_update = 0;
    	ak_print_normal("EN WARNINGWIFIISWEAK PLAY DONE\r\n");
    }
    else if(g_voicevalue == SPICMD_TYPE_ZEROPADDING)
    {
    	char zero_padding[1024]= {0};
    	ak_print_normal("LEN_ZEROPADDING: %d\r\n",512); //.25s
    	ret = aktb_play_data_over_spk(ao_handle,zero_padding,512);
//    	g_receive_update = 0; //NA
    	ak_print_normal("LEN_ZEROPADDING 512 PLAY DONE\r\n");
    }
    else
    {
        ak_print_normal("Unknown type of cmd Play!\r\n");
    }
    ak_print_normal("End type of cmd Play!\r\n");

    return ret;
}
//----------------------------------------

void *ak_talkback_task(void *arg)
{
    int i32Ret = 0;
    void *ao_handle = NULL;
    void *ao_enc_handle = NULL;
    int ret = 0;
    int i = 0;
    struct pcm_param param = {0}; // ref ak_adec_demo.c
    int i32SpaceRb = 0;
    int i32DataRb = 0;
    unsigned int tmpBuf_size = 0;
    unsigned short *pBuffDataRead = NULL;
    unsigned char *pBuffPlay = NULL;
    unsigned short *ptmpbuf = NULL;
    int i32SizeTalkback = 0;
    int i32CntRecv = 0;
    int i32CntPlay = 0;
    int send_size, size;
    int len = 0;
    FILE * fd = NULL;
    static int iCntOneWay = 0;
    unsigned long ulOneWayLastTick = 0;
    unsigned long ulLastDataTalkbackTick = 0, ulTempTick = 0;
    ENUM_SYS_MODE_t sys_get_mode;
    unsigned long ulLastTick = 0, ulCurrentTick = 0;
    /* Debug: record talkback */
    int iSizeTbRec = 0, iSizeRec = 0, iSizeRet = 0, iCntRec = 0;
    int i32LenRb = 0;
	int iCntDataTalkback = 0;
	//unsigned long ulCntDataLastTick = 0;

#ifdef CFG_DEBUG_TALKBACK_WITH_SDCARD
    tb_init_sd();
    fd = tb_open_file();
    if(fd == NULL)
    {
        printf("%s open failed!\r\n", __func__);
    }
#endif
    /* Init speaker */
    ret = talkback_init(&ao_handle);
    if(ret != 0)
    {
        return NULL;
    }
    g_talkback_handle = ao_handle;
    
    tmpBuf_size = ak_ao_get_frame_size(ao_handle);
    ak_print_normal("SPEAKER ao frame size =%d\n", tmpBuf_size);

    //Thuan add - 18/10/2017
    tmpBuf_size = CFG_AUDIO_FRAME_SIZE;
    if(ak_ao_set_frame_size(ao_handle, tmpBuf_size) != 0)
    {
        ak_print_error_ex("set ao frame size successful");
    }
    else
    {
        tmpBuf_size = ak_ao_get_frame_size(ao_handle);
        ak_print_normal("Confirm SPEAKER ao frame size =%d\n", tmpBuf_size);
    }
    //---------------------------

    pBuffDataRead = (unsigned short *)malloc(CFG_TALKBACK_BUF_MAX);
    if(NULL == pBuffDataRead)
    {
        ak_print_error("pBuffDataRead malloc false!\n");
        goto end_talkback;
    }

    pBuffPlay = (unsigned char *)malloc(CFG_TALKBACK_BUF_MAX*2);
    if(NULL == pBuffPlay)
    {
        ak_print_error("pBuffPlay malloc false!\n");
        goto end_talkback;
    }

    if(talkback_decoder_init() != 0)
    {
        dzlog_error("Init decoder failed!");
        goto end_talkback;
    }
    ak_print_normal("%s time 6 %d \n", __func__, get_tick_count());
    ulCurrentTick = get_tick_count();
	// ulCntDataLastTick = ulCurrentTick;
    init_ringbuffer_talkback_issue();
    // if(spi_read_gpio_slave_trigger() == 1){
    //     g_play_dingdong = 1; // for test new lib ak_drv39xx.a
    //     ak_print_error_ex("Got pin CC TRIGGER DINGDONG!\r\n");
    // }
    
    {
    	char zero_padding[512]= {0};
    	ak_print_normal("LEN_ZEROPADDING: %d\r\n",512); //.25s
    	ret = aktb_play_data_over_spk(ao_handle,zero_padding,512);
    	ak_print_normal("LEN_ZEROPADDING 512 PLAY DONE\r\n");
    }
            
    while(1)
    {
        if(g_play_dingdong == 1)
        {
            play_dingdong_pcm2(ao_handle);
            g_play_dingdong = 0;
        }
        else
        {
            // read data g722 from spi and decode
            ret = talkback_read_data_fast((char *)pBuffDataRead, &i32SizeTalkback);
            if(ret != 0)
            {
            #if(TALKBACK_2WAY == 0)
                //TODO: one way talkback
                /* diable flag to send audio when there is no talkback */
                ulTempTick = get_tick_count();
                if((ulTempTick - ulLastDataTalkbackTick) >= TIMEOUT_ONE_WAY_ENABLE_AUDIO){
                    if(g_iOneWayTalkback){
                        g_iOneWayTalkback = 0; 
                        printf("Enable Audio:%u\r\n", ulTempTick - ulLastDataTalkbackTick);
                    }
                }
            #endif
                ak_sleep_ms(TALKBACK_TASK_DELAY);
            }
            else
            {
            	iCntDataTalkback += i32SizeTalkback;
                if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
                {
                    // buffer data talkback
                    i32LenRb = Ringbuf_GetAvailSpace(g_pRbTalkbackBuffer);
                    if(i32LenRb >= i32SizeTalkback)
                    {
                        Ringbuf_WriteBuf(g_pRbTalkbackBuffer, (unsigned char *)pBuffDataRead, i32SizeTalkback);
                    }
                    else
                    {
                        ak_print_error_ex("g_pRbTalkbackBuffer is full!\r\n");
                    }
                
                #if(TALKBACK_2WAY == 0)
                    //TODO: one way talkback
                    ulLastDataTalkbackTick = get_tick_count();
                    /* set flag not to send audio during talkback */
                    if(g_iOneWayTalkback == 0){
                        g_iOneWayTalkback = 1;
                        printf("Enable flag g_iOneWayTalkback:%d cnt:%d\r\n", g_iOneWayTalkback, iCntOneWay);
        
                    }
                #endif
                }
            }
            // play talkback here
            // buffer data talkback
#if 0 // need to process for 
            i32LenRb = Ringbuf_GetAvailData(g_pRbTalkbackBuffer);
            if(i32LenRb >= (LEN_DATA_PLAY) )
            {
                i32Ret = Ringbuf_ReadBuf(g_pRbTalkbackBuffer, pBuffPlay, i32LenRb);
                i32SizeTalkback = i32LenRb;
                if(i32Ret != 0)
                {
                    ak_print_error_ex("Get data from buffer talkback failed!\r\n");
                }
                else
                {
                    i32Ret = aktb_play_data_over_spk1(ao_handle, (char *)pBuffPlay, i32SizeTalkback);
                    if(i32Ret != i32SizeTalkback)
                    {
                        ak_print_error_ex("TB play data failed! (%d)\r\n", i32Ret);
                    }
                    else
                    {
                        i32CntPlay += i32SizeTalkback;
                        // printf("#%d ", i32SizeTalkback);
                    }
                }
            }
#else   // process for audio loopback
    {
        // check mode ftest
        if(sys_get_mode == E_SYS_MODE_FTEST)
        {
            // drop data talkback from CC3200
            i32LenRb = Ringbuf_GetAvailData(g_pRbTalkbackBuffer);
            if(i32LenRb > 0)
            {
                i32Ret = Ringbuf_ReadBuf(g_pRbTalkbackBuffer, pBuffPlay, i32LenRb);
            }

            // play data loopback
            i32Ret = audio_loopback_get_data(pBuffPlay, LEN_DATA_PLAY);
            // ak_print_normal("Getdata loopback: %d\n", i32Ret);
            if(i32Ret > 0)
            {
                if(ftest_audioloop_get_mode_stop() == 1 || tone_gen_get_running() == 1)
                {
                    i32SizeTalkback = i32Ret;
                    i32Ret = aktb_play_data_over_spk1(ao_handle, (char *)pBuffPlay, i32SizeTalkback);
                    if(i32Ret != i32SizeTalkback)
                    {
                        ak_print_error_ex("Loopback play data failed! (%d)\r\n", i32Ret);
                    }
                    else
                    {
                        i32CntPlay += i32SizeTalkback;
                        if(tone_gen_get_running() == 1){
                            printf("Tone#%d ", i32SizeTalkback);
                        }
                        else{
                             printf("Loop#%d ", i32SizeTalkback);
                        }
                    }
                }
            }
            else
            {
                // dont play data
            }
        }
        else{
            // drop data loobback
            audio_loopback_get_data(pBuffPlay, LEN_DATA_PLAY);

            i32LenRb = Ringbuf_GetAvailData(g_pRbTalkbackBuffer);
            if(i32LenRb >= (LEN_DATA_PLAY) )
            {
                i32Ret = Ringbuf_ReadBuf(g_pRbTalkbackBuffer, pBuffPlay, i32LenRb);
                i32SizeTalkback = i32LenRb;
                if(i32Ret != 0)
                {
                    ak_print_error_ex("Get data from buffer talkback failed!\r\n");
                }
                else
                {
                    i32Ret = aktb_play_data_over_spk1(ao_handle, (char *)pBuffPlay, i32SizeTalkback);
                    if(i32Ret != i32SizeTalkback)
                    {
                        ak_print_error_ex("TB play data failed! (%d)\r\n", i32Ret);
                    }
                    else
                    {
                        i32CntPlay += i32SizeTalkback;
                        // printf("#%d ", i32SizeTalkback);
                    }
                }
            }
        }
    }
#endif
        }// play dingdong
        // ret = fake_data_play_speaker(file, (char *)pBuffPlay, &i32SizeTalkback);
        sys_get_mode = eSysModeGet();
        // if(E_SYS_MODE_FTEST == sys_get_mode || \
        //      E_SYS_MODE_FWUPGRADE == sys_get_mode)
        if(E_SYS_MODE_FWUPGRADE == sys_get_mode)
        {
            ak_print_normal("%s exit. Mode: %d\r\n", __func__, sys_get_mode);
            break;
        }
        ak_sleep_ms(10);
        ulLastTick = get_tick_count();
        if((ulLastTick - ulCurrentTick) >= 1000)
        {
            if(sys_get_mode != E_SYS_MODE_FTEST){
        	    printf("Talk recv:%d play:%d (bytes)\n", iCntDataTalkback, i32CntPlay);
            }
            ulCurrentTick = ulLastTick;
            i32CntRecv = 0;
            i32CntPlay = 0;
            if(iCntRec <= 30){
                iCntRec++;
            }
			iCntDataTalkback = 0;
        }
        if(g_receive_update == 1)
        {
            //Thuan add - 18/10/2017
            ret = ao_send_frame_maintask(ao_handle);
            if (ret < 0)
            {
                ak_print_normal("ao_send_frame_maintask fail! \r\n");
            }
        }
    }

end_talkback:
    // free ao_handle
    ak_ao_enable_speaker(ao_handle, SPK_DISABLE);
    ak_ao_close(ao_handle);

    g_talkback_handle = NULL;
    // unmount_close_file(file);
    // deinit_ringbuffer_talkback_issue();
    // get thread id and exit
    ak_thread_join(ak_thread_get_tid());
    ak_thread_exit();

    return NULL;
}


// void *ak_talkback_fast_play_task(void *arg)
// {
//     int i32Ret = 0;
//     void *ao_handle = NULL;
//     void *ao_enc_handle = NULL;
//     int ret = 0;
//     int i = 0;
//     struct pcm_param param = {0}; // ref ak_adec_demo.c
//     int i32SpaceRb = 0;
//     int i32DataRb = 0;

//     unsigned int tmpBuf_size = 0;
//     unsigned short *pBuffDataRead = NULL;
//     unsigned char *pBuffPlay = NULL;
//     unsigned short *ptmpbuf = NULL;
// //    unsigned char *Voice_Data = NULL;
//     int cnt = 0;

//     int i32SizeTalkback = 0;
//     int i32CntRecv = 0;
//     int i32CntPlay = 0;
//     int send_size, size;
//     int len = 0;
//     FILE * fd = NULL;
//     unsigned char file_over = 0;

//     static int iCntOneWay = 0;
//     unsigned long ulOneWayLastTick = 0;
//     unsigned long ulLastDataTalkbackTick = 0, ulTempTick = 0;

//     ENUM_SYS_MODE_t sys_get_mode;

//     unsigned long ulLastTick = 0, ulCurrentTick = 0;
    
//     /* Debug: record talkback */
//     int iSizeTbRec = 0, iSizeRec = 0, iSizeRet = 0, iCntRec = 0;

//     int i32LenRb = 0;

//     #ifdef CFG_DEBUG_TALKBACK_WITH_SDCARD
//     tb_init_sd();
//     fd = tb_open_file();
//     if(fd == NULL)
//     {
//         printf("%s open failed!\r\n", __func__);
//     }
//     #endif

//     ak_print_normal("%s time 5 %d \n", __func__, get_tick_count());
//     /* Init speaker */
//     ret = talkback_init(&ao_handle);
//     if(ret != 0)
//     {
//         return NULL;
//     }

//     g_talkback_handle = ao_handle;
    
//     tmpBuf_size = ak_ao_get_frame_size(ao_handle);
//     ak_print_normal("SPEAKER ao frame size =%d\n", tmpBuf_size);
//     // tmpBuf_size = CFG_AUDIO_FRAME_SIZE;
//     // if(ak_ao_set_frame_size(ao_handle, tmpBuf_size) != 0)
//     // {
//     //     ak_print_error_ex("Why don't set ao frame size successful?");
//     // }
//     // else
//     // {
//     //     tmpBuf_size = ak_ao_get_frame_size(ao_handle);
//     //     ak_print_normal("Confirm SPEAKER ao frame size =%d\n", tmpBuf_size);
//     // }

//     pBuffDataRead = (unsigned short *)malloc(CFG_TALKBACK_BUF_MAX);
//     if(NULL == pBuffDataRead)
//     {
//         ak_print_error("pBuffDataRead malloc false!\n");
//         goto end_talkback;
//     }

//     pBuffPlay = (unsigned char *)malloc(CFG_TALKBACK_BUF_MAX*2);
//     if(NULL == pBuffPlay)
//     {
//         ak_print_error("pBuffPlay malloc false!\n");
//         goto end_talkback;
//     }

//     if(talkback_decoder_init() != 0)
//     {
//         dzlog_error("Init decoder failed!");
//         goto end_talkback;
//     }
//     ak_print_normal("%s time 6 %d \n", __func__, get_tick_count());
//     ulCurrentTick = get_tick_count();

// #if 0
//     init_ringbuffer_talkback_issue();
// #endif

//     // g_play_dingdong = 1; // for test new lib ak_drv39xx.a
//     while(1)
//     {
//         if(g_play_dingdong == 1)
//         {
//             // play_dingdong_pcm(ao_handle);
//             play_dingdong_pcm2(ao_handle);
//             g_play_dingdong = 0;
//         }
//         else
//         {
//             // read data g722 from spi and decode
//             ret = talkback_read_data_fast((char *)pBuffDataRead, &i32SizeTalkback);
//             if(ret != 0)
//             {
//             #if(TALKBACK_2WAY == 0)
//                 //TODO: one way talkback
//                 /* diable flag to send audio when there is no talkback */
//                 ulTempTick = get_tick_count();
//                 if((ulTempTick - ulLastDataTalkbackTick) >= TIMEOUT_ONE_WAY_ENABLE_AUDIO){
//                     if(g_iOneWayTalkback){
//                         g_iOneWayTalkback = 0; 
//                         printf("Enable Audio:%u\r\n", ulTempTick - ulLastDataTalkbackTick);
//                     }
//                 }
//             #endif
//                 ak_sleep_ms(TALKBACK_TASK_DELAY);
//             }
//             else
//             {
//                 if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
//                 {
//                 #if 1
//                     // buffer data talkback
//                     i32LenRb = Ringbuf_GetAvailSpace(g_pRbTalkbackBuffer);
//                     if(i32LenRb >= CFG_AUDIO_FRAME_SIZE)
//                     {
//                         Ringbuf_WriteBuf(g_pRbTalkbackBuffer, (unsigned char *)pBuffDataRead, i32SizeTalkback);
//                     }
//                     else
//                     {
//                         ak_print_error_ex("g_pRbTalkbackBuffer is full!\r\n");
//                     }
//                 #endif
                
//                 #if(TALKBACK_2WAY == 0)
//                     //TODO: one way talkback
//                     ulLastDataTalkbackTick = get_tick_count();
//                     /* set flag not to send audio during talkback */
//                     if(g_iOneWayTalkback == 0){
//                         g_iOneWayTalkback = 1;
//                         printf("Enable flag g_iOneWayTalkback:%d cnt:%d\r\n", g_iOneWayTalkback, iCntOneWay);
        
//                     }
//                 #endif

//                     // convert data from stereo to mono
//                     // play speaker
                    
//                 #if 1
//                     while(i32SizeTalkback > 0)
//                     {
//                         // convert data from stereo to mono
//                         ptmpbuf = (unsigned short *)pBuffPlay;
//                         memcpy((char *)ptmpbuf, pBuffDataRead, i32SizeTalkback);
//                         // play speaker
//                         iSizeTbRec = i32SizeTalkback;
//                         i32CntRecv += i32SizeTalkback;
//                         send_size = 0;
//                         while(i32SizeTalkback > 0)
//                         {
//                             size = ak_ao_send_frame(ao_handle, pBuffPlay + send_size, i32SizeTalkback, 0);
//                             if(size < 0)
//                             {
//                                 ak_print_error("ao send frame error!  %d \n", size);
//                                 break;
//                             }
//                             else
//                             {
//                                 send_size += size; 
//                                 i32SizeTalkback = i32SizeTalkback - size;
//                                 i32CntPlay += size;
//                                 cnt++;
//                                 if(cnt%30 == 0){
//                                     printf("#%d ", size);
//                                     cnt = 0;
//                                 }
//                             }
//                             ak_sleep_ms(5);
//                             send_size += size; 
//                             i32SizeTalkback = i32SizeTalkback - size;
//                             i32CntPlay += size;
//                             // printf("Play: %d\n", size);
//                         }
//                     }
//                 #endif
//                 }
//                 else
//                 {
//                     ak_sleep_ms(10);
//                 }

//                 /* debug talkback */
//                 #ifdef CFG_DEBUG_TALKBACK_WITH_SDCARD
//                 if(fd != NULL && iCntRec <= 20)
//                 {
//                     iSizeRet = 0;
//                     iSizeRec = 0;
//                     while(iSizeTbRec > 0)
//                     {
//                         iSizeRet = tb_write_file(fd, pBuffPlay + iSizeRec, iSizeTbRec);
//                         if(iSizeRet >= 0)
//                         {
//                             iSizeTbRec -= iSizeRet;
//                             iSizeRec += iSizeRet;
//                             #ifndef UPLOADER_DEBUG
//                             printf("rectb:%d cnt:%d\r\n", iSizeRet, iCntRec);
//                             #endif
//                         }
//                         else
//                         {
//                             printf("record talkback failed!\r\n");
//                             break;
//                         }
//                     }
//                     if(iCntRec >= 20)
//                     {
//                         printf("close file talkback record!\r\n");
//                         tb_close_umount_file_record(fd);
//                         fd = NULL;
//                     }
//                 }
//                 #endif
//             }

//             #if 0
//             // play talkback here
//             // buffer data talkback
//             i32LenRb = Ringbuf_GetAvailData(g_pRbTalkbackBuffer);
//             if(i32LenRb >= LEN_DATA_PLAY)
//             {
//                 i32Ret = Ringbuf_ReadBuf(g_pRbTalkbackBuffer, pBuffPlay, LEN_DATA_PLAY);
//                 i32SizeTalkback = LEN_DATA_PLAY;
//                 while(i32SizeTalkback > 0)
//                 {
//                     // convert data from stereo to mono
//                     ptmpbuf = (unsigned short *)pBuffPlay;
//                     memcpy((char *)ptmpbuf, pBuffDataRead, i32SizeTalkback);
//                     // play speaker
//                     iSizeTbRec = i32SizeTalkback;
//                     i32CntRecv += i32SizeTalkback;
//                     send_size = 0;
//                     while(i32SizeTalkback > 0)
//                     {
//                         size = ak_ao_send_frame(ao_handle, pBuffPlay + send_size, i32SizeTalkback, 0);
//                         if(size < 0)
//                         {
//                             ak_print_error("ao send frame error!  %d \n", size);
//                             break;
//                         }
//                         else
//                         {
//                             send_size += size; 
//                             i32SizeTalkback = i32SizeTalkback - size;
//                             i32CntPlay += size;
//                             // cnt++;
//                             // if(cnt%30 == 0){
//                                 printf("#%d ", size);
//                                 // cnt = 0;
//                             // }
//                         }
//                         ak_sleep_ms(5);
//                         send_size += size; 
//                         i32SizeTalkback = i32SizeTalkback - size;
//                         i32CntPlay += size;
//                         // printf("Play: %d\n", size);
//                     }
//                 }
//             }
//             #endif

//         }// play dingdong

//         // ret = fake_data_play_speaker(file, (char *)pBuffPlay, &i32SizeTalkback);
//         sys_get_mode = eSysModeGet();
//         if(E_SYS_MODE_FTEST == sys_get_mode || \
//             E_SYS_MODE_FWUPGRADE == sys_get_mode)
//         {
//             ak_print_normal("%s exit. Mode: %d\r\n", __func__, sys_get_mode);
//             break;
//         }
//         // ak_sleep_ms(TALKBACK_TASK_DELAY);
//         ak_sleep_ms(5);
//         ulLastTick = get_tick_count();
//         if((ulLastTick - ulCurrentTick) >= 1000)
//         {
//             // printf("TB recv:%d play:%d\n", i32CntRecv, i32CntPlay);
//             ulCurrentTick = ulLastTick;
//             i32CntRecv = 0;
//             i32CntPlay = 0;
//             if(iCntRec <= 30){
//                 iCntRec++;
//             }
//         }
//         if(g_receive_update == 1)
//         {
//         	ak_print_normal("START SELECT TYPE CMD PLAY\r\n");
//         	if(g_voicevalue == SPICMD_TYPE_VOICE_BEEP)
//         	{
//         		ak_print_normal("size_len_ding_data: %d\r\n",len_arrding);
// //        		for(i = 0;i<len_arrding;i++)
// //        		{
// //        			Voice_Data[i] = Ding_Data[i];
// //        			ak_ao_send_frame(ao_handle,Voice_Data,len_arrding,0);
// //        		}
//         		ak_ao_send_frame(ao_handle,Ding_Data,len_arrding,0);
//         		g_receive_update = 0;
//         		ak_print_normal("Ding Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_FAIL)
//         	{
//         		ak_print_normal("size_len_fail_data: %d\r\n",len_arrfail);
//         		ak_ao_send_frame(ao_handle,Fail_Data,len_arrfail,0);
//         		g_receive_update = 0;
//         		ak_print_normal("Fail Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_SUCCESS)
//         	{
//         		ak_print_normal("size_len_success_data: %d\r\n",len_arrsuccess);
//         		ak_ao_send_frame(ao_handle,Success_Data,len_arrsuccess,0);
//         		g_receive_update = 0;
//         		ak_print_normal("Success Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_TIMEOUT)
//         	{
//         		ak_print_normal("size_len_time_data: %d\r\n",len_arrtime);
//         		ak_ao_send_frame(ao_handle,Timeout_Data,len_arrtime,0);
//         		g_receive_update = 0;
//         		ak_print_normal("Timeout Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_FAIL)
//         	{
//         		ak_print_normal("len_fail_CH: %d\r\n",len_chfail);
//         		ak_ao_send_frame(ao_handle,Fail_Data_CH,len_chfail,0);
//         		g_receive_update = 0;
//         		ak_print_normal("CH fail Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_SUCCESS)
//         	{
//         		ak_print_normal("size_len_success_data: %d\r\n",len_chsuccess);
//         		ak_ao_send_frame(ao_handle,Success_Data_CH,len_chsuccess,0);
//         		g_receive_update = 0;
//         		ak_print_normal("Success Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_TIMEOUT)
//         	{
//         		ak_print_normal("len_timeout_CH: %d\r\n",len_chtimeout);
//         		ak_ao_send_frame(ao_handle,Timeout_Data_CH,len_chtimeout,0);
//         		g_receive_update = 0;
//         		ak_print_normal("CH timeout Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_SORRYNOTFREETOMEETNOW)
//         	{
//         		ak_print_normal("len_SORRYNOTFREETOMEETNOW: %d\r\n",Len_SORRYNOTFREETOMEETNOW);
//         		ak_ao_send_frame(ao_handle,SORRYNOTFREETOMEETNOW,Len_SORRYNOTFREETOMEETNOW,0);
//         		g_receive_update = 0;
//         		ak_print_normal("EN SORRYNOTFREETOMEETNOW Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_YOUCANLEAVEIT)
//         	{
//         		ak_print_normal("len_YOUCANLEAVEIT: %d\r\n",Len_YOUCANLEAVEIT);
//         		ak_ao_send_frame(ao_handle,YOUCANLEAVEIT,Len_YOUCANLEAVEIT,0);
//         		g_receive_update = 0;
//         		ak_print_normal("EN YOUCANLEAVEIT Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_WElCOMECOMINGOVERNOW)
//         	{
//         		ak_print_normal("len_WElCOMECOMINGOVERNOW: %d\r\n",Len_WElCOMECOMINGOVERNOW);
//         		ak_ao_send_frame(ao_handle,WElCOMECOMINGOVERNOW,Len_WElCOMECOMINGOVERNOW,0);
//         		g_receive_update = 0;
//         		ak_print_normal("EN WElCOMECOMINGOVERNOW Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_HOWCANIHELP)
//         	{
//         		ak_print_normal("len_HOWCANIHELP: %d\r\n",Len_HOWCANIHELP);
//         		ak_ao_send_frame(ao_handle,HOWCANIHELP,Len_HOWCANIHELP,0);
//         		g_receive_update = 0;
//         		ak_print_normal("EN HOWCANIHELP Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_EN_VOICE_SORRYNOTATHOME)
//         	{
//         		ak_print_normal("len_SORRYNOTATHOME: %d\r\n",Len_SORRYNOTATHOME);
//         		ak_ao_send_frame(ao_handle,SORRYNOTATHOME,Len_SORRYNOTATHOME,0);
//         		g_receive_update = 0;
//         		ak_print_normal("EN SORRYNOTATHOME Play Done\r\n");
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_SORRYNOTFREETOMEETNOW)
//         	{
//         		ak_print_normal("NOT YET FILE SOURCE:%d\r\n",24);
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_YOUCANLEAVEIT)
//         	{
//         		ak_print_normal("NOT YET FILE SOURCE:%d\r\n",25);
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_WElCOMECOMINGOVERNOW)
//         	{
//         		ak_print_normal("NOT YET FILE SOURCE:%d\r\n",26);
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_HOWCANIHELP)
//         	{
//         		ak_print_normal("NOT YET FILE SOURCE:%d\r\n",27);
//         	}
//         	else if(g_voicevalue == SPICMD_TYPE_CH_VOICE_SORRYNOTATHOME)
//         	{
//         		ak_print_normal("NOT YET FILE SOURCE:%d\r\n",28);
//         	}
//         	else
//         	{
//         		ak_print_normal("Unknown type of cmd Play!\r\n");
//         	}
//         	ak_print_normal("End type of cmd Play!\r\n");
//         }

//     }

// end_talkback:
//     // free ao_handle
//     ak_ao_enable_speaker(ao_handle, SPK_DISABLE);
//     ak_ao_close(ao_handle);

//     g_talkback_handle = NULL;
//     // unmount_close_file(file);
//     // deinit_ringbuffer_talkback_issue();
//     // get thread id and exit
//     ak_thread_join(ak_thread_get_tid());
//     ak_thread_exit();

//     return NULL;
// }

#endif

#define TEST_FILE_PLAY_SPEAKER          "play_speaker.wav"

static void * mount_open_file(void)
{
    char file[100] = {0};
    FILE *filp = NULL;

    
    sprintf(file, "%s/%s", "a:", TEST_FILE_PLAY_SPEAKER);
    
    filp = fopen(file, "r");
    if (!filp) 
    {
        ak_print_error_ex("fopen, %s\n", strerror(errno));
        exit(0);
    }
    return filp;
}

static void unmount_close_file(FILE *filp)
{
    fclose(filp);
    
    if(sdcard_get_status() != E_SDCARD_STATUS_UMOUNT)
    {
        sdcard_umount();
    }
    else
    {
        ak_print_normal("SDcard was umounted!\r\n");
    }
}

static int fake_data_play_speaker(void *file, char *pData, int *pLen)
{
    int ret = 0;
    if(pData == NULL|| pLen == NULL || file == NULL)
    {
        return -1;
    }
    ret = ak_fread((void *)pData, 1024, 1, file);
    if(ret == 0)
    {
        printf("Read file failed!\r\n");
    }
    else
    {
        *pLen = ret;
    }
    return ret;
}

/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/

