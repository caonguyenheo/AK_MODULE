/**
  ******************************************************************************
  * File Name          : ak_audio.c
  * Description        : This file contain functions for ak_audio
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
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

#include "ak_apps_config.h"

#include "g722_encoder.h"
#include "ak_audio.h"

#include "spi_transfer.h"

#include "packet.h"

#include "ak_common.h"
#include "ak_ai.h"
#include "ak_aenc.h"
#include "ak_login.h"
#include "kernel.h"
#include "hal_timer.h"
// #include "main_ctrl.h"
#include "ringbuf_1.h"
#include "flv_engine.h"
#ifdef CFG_USE_SCHEDULER
#include "transfer_scheduler.h"
#endif 

#include "doorbell_config.h"

#include "sdcard.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/
//TODO: one way talkback
extern int g_iOneWayTalkback;

void *g_audio_handle;

extern int g_iFirstFrameOK;

extern int g_iP2pSdStreamStart;

extern int g_CfgVideoDone;


extern int g_agc_enable;

/* audio input/output param */
typedef struct  {
    unsigned int sample_rate;
    unsigned int sample_bits;
    unsigned int channel_num;
}ai_param;
typedef struct  {
    unsigned char *data;        //ai_frame data
    unsigned long len;          //ai_frame len in bytes
    unsigned long ts;           //timestamp(ms)
}ai_frame;

unsigned long *ai_trans_buf;


void *adc_drv;
// wait 2 packets audio, each packet has 512 bytes ~ 32ms.
// we will store them into ringbuffer
static ring_buffer_t *g_pRbAudio32ms;
#define RINGBUF_AUDIO_32MS_ISSUE_LEN    (8*1024)  // 2Kbyte

char dummy_data[1024];


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/* Trung add debug */

#define PER_AUDIO_DURATION		    20	// AMR 20ms one frame
#define enable 1
// #define AENC_TCARD_FREE_SIZE        1024  // T  Card  the lease  free  size (1M) 

/*
* Debug, recording stream audio 
*/
#define SAVE_PATH        "a:"
#define FILE_NAME_AUDIO  "debug_audio.pcm"

#ifdef CFG_DEBUG_AUDIO_WITH_SDCARD

static void init_debug_sdcard(void)
{
    // ak_mount_fs(1, 0, "a:");
    sdcard_mount();
}
static void deinit_debug_sdcard(void)
{
    // ak_unmount_fs(1, 0, "a:");
    sdcard_umount();
}
static void *open_file(int index)
{
    char file[100] = {0};
    sprintf(file, "%s/ch%d_%s", SAVE_PATH, index, FILE_NAME_AUDIO);

    FILE *filp = NULL;
    filp = fopen(file, "w");
    if (!filp) {
        ak_print_error_ex("fopen, %s\n", strerror(errno));
        exit(0);
    }

    return filp;
}

static void close_file(FILE *filp)
{
    fclose(filp);
}

static void a_save_stream(FILE *filp, unsigned char *data, int len)
{
    int ret = len;
    
    do {
        ret -= fwrite(data, 1, ret, filp);
    } while (ret > 0);
}

#endif
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/
void audio_encrypt_send_spi_queue(void *stream_handle);
/*---------------------------------------------------------------------------*/
/*                           AUDIO NEW SDK                                   */
/*---------------------------------------------------------------------------*/

void audio_send_dummy(void)
{
    memset(dummy_data, 0x00, 1024);
    send_spi_data(dummy_data, 1024);
}



static int init_ringbuffer_32ms_issue(void)
{
    int iRet = 0;
    g_pRbAudio32ms = Ringbuf_Init(RINGBUF_AUDIO_32MS_ISSUE_LEN);
    if(g_pRbAudio32ms == NULL)
    {
        ak_print_error("%s g_pRbAudio32ms NULL!\r\n", __func__);
        iRet = -1;
    }
    else
    {
        ak_print_normal("%s initialized buffer with %d bytes\r\n", \
                            __func__, RINGBUF_AUDIO_32MS_ISSUE_LEN);
    }
    return iRet;
}

static int deinit_ringbuffer_32ms_issue(void)
{
    int iRet = 0;
    if(g_pRbAudio32ms)
    {
        Ringbuf_Free(g_pRbAudio32ms);
        ak_print_normal("%s free g_pRbAudio32ms\r\n", __func__);
    }
    return iRet;
}


#if 0
void audio_encrypt_send_spi_queue(void *stream_handle)
{
    struct list_head stream_head ;
    struct aenc_entry *entry = NULL; 
    struct aenc_entry *ptr = NULL;
    unsigned long first_ts = 0, frame_ts = 0;
	// int total_ts = time;
    bool first_get_flag = false;
    INIT_LIST_HEAD(&stream_head);

    char *pDtG722Out;
    char *pDtAuSpiOut;
    int i32Ret = 0;
    int packet_counter = 0;
    int out_len = 0;
    int tmp_len = 0;
    int s32RetVal = 0;
    int i32G722OutLen = 0;
    unsigned int u32CntPkt = 0;
    ENUM_SYS_MODE_t sys_get_mode;
    unsigned long ulLastTick = 0, ulCurrentTick = 0;
    
    int i32LenRb = 0;
    int i32CntG722 = 0, i32CntPcm = 0;
    unsigned char ucGetTimestampFirstAudioPacket = 0;
	int iLenPacketSend = 0;
	int iAgcCalGain = 0;
	int iLengthSampling = 0;
	int iCntSampling = 0;

#ifdef CFG_AUDIO_G722
    /* G722 */
//    g722_enc_t g722_enc;
//    g722_enc_init(&g722_enc);
#endif
    if(init_ringbuffer_32ms_issue() == -1)
    {
        ak_print_error_ex("Not enough buffer for audio send!!!\r\n");
        return;
    }
#ifdef CFG_AUDIO_G722
    pDtG722Out = (char *)malloc(CFG_AUDIO_FRAME_SIZE);
    if(pDtG722Out == NULL)
    {
        printf("Cannot allocate memory for pDtG722Out send SPI\n\r");
        goto g722_exit;
    }
#endif
    pDtAuSpiOut = (char *)malloc(CFG_AUDIO_BUF_SPI_OUT);
    if(pDtAuSpiOut == NULL)
    {
        printf("Cannot allocate memory for pDtAuSpiOut send SPI\n\r");
        goto g722_exit;
    }

	//set_agc_debug(1);

    ulCurrentTick = get_tick_count();
	while(1)
	{	
        ulLastTick = get_tick_count();
		/* get audio stream list */		
		if(0 == ak_aenc_get_stream(stream_handle,&stream_head))
		{
			list_for_each_entry_safe(entry, ptr, &stream_head, list)
			{
				if(NULL != entry) 
				{
                    if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
                    {
                    // #ifndef DOORBELL_TINKLE820
                    #if (RECORDING_SDCARD == 1)
				        /* Feed data to FLV muxer before send over SPI */
                        if(FLVEnginePushAudioData(entry->stream.data, entry->stream.len, entry->stream.ts) != 0)
                        {
                            ak_print_normal("FLVEnginePushAudioData size %d, ts %lu fail\n", entry->stream.len, entry->stream.ts);
                        }
                        else
                        {
                            // ak_print_normal("FLVEnginePushAudioData size %d, ts %lu\n", entry->stream.len, entry->stream.ts);
                        }
                    #endif
                        
                        /* Feed timestamp to packet */
                        if(ucGetTimestampFirstAudioPacket == 0)
                        {
                            packet_set_audio_timestamp((unsigned int)(entry->stream.ts));
                            ucGetTimestampFirstAudioPacket++;
                        }
                        else
                        {
                            ucGetTimestampFirstAudioPacket = 0;
                        }
                        // printf("a_ts: %lu tick:%u\r\n", entry->stream.ts, ulLastTick);
                        // printf("tick: %u\n", ulLastTick);

						// AGC: we insert automatic gain control at here to filter
						
						if(iCntSampling < 4){
							iAgcCalGain += audio_agc_calculate_gain(entry->stream.data, entry->stream.len);
							//printf("iAgcCalGain: %d\r\n", iAgcCalGain);
							iLengthSampling += entry->stream.len;
							iCntSampling++;
						}
						else
						{
							iAgcCalGain = iAgcCalGain/4;
							printf("%d\n", iAgcCalGain);
							audio_gain_adaptivity(iAgcCalGain);
							iCntSampling = 0;
							iLengthSampling = 0;
							iAgcCalGain = 0;
						}
						
						
#ifdef CFG_AUDIO_G722
//                        g722_enc_encode(&g722_enc, (short *)(entry->stream.data), \
//                                    (int)(entry->stream.len)/2, pDtG722Out, &i32G722OutLen);
                        // save data to ringbuffer, wait enough 64ms data.
                        i32LenRb = Ringbuf_GetAvailSpace(g_pRbAudio32ms);
                        if(i32LenRb >= i32G722OutLen)
                        {
                            Ringbuf_WriteBuf(g_pRbAudio32ms, pDtG722Out, i32G722OutLen);
							// printf("G%d ", i32G722OutLen);
                        }
#else
						i32LenRb = Ringbuf_GetAvailSpace(g_pRbAudio32ms);
                        if(i32LenRb >= entry->stream.len)
                        {
                            Ringbuf_WriteBuf(g_pRbAudio32ms, entry->stream.data, entry->stream.len);
							printf("[A-PCM]%d\n", entry->stream.len);
                        }
#endif
                        else
                        {
                            ak_print_error_ex("Rb32ms is full!\r\n");
                        }
                        i32CntPcm += entry->stream.len;
                    }
                    ak_aenc_release_stream(entry);
				}
			}
		}
        else
    	{
            if((ulLastTick - ulCurrentTick) >= 1000)
            {
                if(g_iP2pSdStreamStart == 0)
                {
                	#ifndef UPLOADER_DEBUG
                    printf("Audio sent: %d pkts, g722:%d ~ pcm:%d\n", u32CntPkt, i32CntG722, i32CntPcm);
					#endif
                }
                
                u32CntPkt = 0;
                i32CntG722 = 0;
                i32CntPcm = 0;
                ulCurrentTick = ulLastTick;
            }
            // ak_sleep_ms(AUDIO_TASK_NOFRAME);
			ak_sleep_ms(10);
		    // continue;
        }
        
        if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
        {
		#ifdef CFG_AUDIO_G722
			iLenPacketSend = 256;
		#else
			iLenPacketSend = 512;
		#endif
            // check length >256 and send
            i32LenRb = Ringbuf_GetAvailData(g_pRbAudio32ms);
            if(i32LenRb >= iLenPacketSend)
            {
                i32G722OutLen = iLenPacketSend;
                i32Ret = Ringbuf_ReadBuf(g_pRbAudio32ms, pDtG722Out, i32G722OutLen);
        
                //TODO: one way talkback
                // drop all data audio when talkback, don't send it
            #if(TALKBACK_2WAY == 0)
                if(g_iOneWayTalkback == 0)
                {
            #endif
                    if(i32Ret == 0)
                    {
                        // /* Feed timestamp to packet */
                        // if(ucGetTimestampFirstAudioPacket == 0)
                        // {
                        //     packet_set_audio_timestamp((unsigned int)(ulLastTick));
                        //     ucGetTimestampFirstAudioPacket++;
                        // }
                        // else
                        // {
                        //     ucGetTimestampFirstAudioPacket = 0;
                        // }
                        

                        packet_lock_mutex();
			#ifdef CFG_AUDIO_G722
                        i32Ret = packet_create(85, &packet_counter, (char *)(pDtG722Out), \
                                    i32G722OutLen, pDtAuSpiOut, &out_len);
			#else
						i32Ret = packet_create(81, &packet_counter, (char *)(pDtG722Out), \
                                    i32G722OutLen, pDtAuSpiOut, &out_len);
			#endif
                        packet_unlock_mutex();
                        i32CntG722 += 256;
                        tmp_len = 0;
                        while(tmp_len < out_len)
                        {
                            s32RetVal = send_spi_data((pDtAuSpiOut + tmp_len), 1024);
                            if(s32RetVal != 1024)
                            {
                                ak_sleep_ms(20);
                                // ak_print_normal("[ERROR] send data spi failed (%d)!\n", s32RetVal);
                                ak_print_normal("#");
                                break;
                            }
                            else{
                                tmp_len +=1024;
                                u32CntPkt++;
                            }
                        }
                    }//if(i32Ret != 0)
                    else
                    {
                        ak_print_error("%s: Read buf failed (%d)!\r\n", __func__, i32Ret);
                    }
            
            #if(TALKBACK_2WAY == 0)
                }
                else
                {
                    audio_send_dummy();
                    audio_send_dummy();
                    // printf("L:%d_", i32LenRb);
                    // ak_sleep_ms(15);
                    ak_sleep_ms(10);
                }//if(g_iOneWayTalkback == 0)
            #endif
            }

        }
        else
        {
            ak_sleep_ms(10);
        }
        sys_get_mode = eSysModeGet();
        if(E_SYS_MODE_FTEST == sys_get_mode || \
            E_SYS_MODE_FWUPGRADE == sys_get_mode)
        {
            ak_print_normal("%s exit. Mode: %d\r\n", __func__, sys_get_mode);
            break;
        }
	}

g722_exit:
    
    free(pDtAuSpiOut);
#ifdef CFG_AUDIO_G722
	free(pDtG722Out);
//    g722_enc_deinit(&g722_enc);
#endif
    deinit_ringbuffer_32ms_issue();
	ak_print_normal("Audio exit loop!\n\n");
}
#endif
static void set_frame_interval(void *handle, int encode_type, unsigned int sample_rate)

{
	if(NULL == handle)
	{
	    return ;
	}
	unsigned int interval = 0;	
	switch(encode_type){
	case AK_AUDIO_TYPE_AAC:
		interval = ((1024 *1000) / sample_rate); // 1k data in 1 second 
		break;
	case AK_AUDIO_TYPE_AMR:
		interval = PER_AUDIO_DURATION;
		break;
		
	case AK_AUDIO_TYPE_PCM_ALAW:	//G711, alaw
	case AK_AUDIO_TYPE_PCM_ULAW:	//G711, ulaw:
		interval = 100;
		break;
	
	default:	//default is AMR
		// interval = PER_AUDIO_DURATION;
        interval = CFG_AUDIO_DURATION;
		break;
	}
	ak_ai_set_frame_interval(handle, interval);
}


/*
Read value from config
Set volume
*/
int audio_ai_set_volume(void *pAiHandle)
{
    int iRet_a = 0, iRet_d = 0, iRet = 0;
    int iValueMicd = 0, iValueMica = 0;

    // volume analog
    iRet_a = doorbell_cfg_get_mica_gain(&iValueMica);
    if(iRet_a == 0)
    {
        ak_print_normal("Read MIC: %d\r\n", iValueMica);
        ak_ai_set_volume(pAiHandle, iValueMica);
    }
    else
    {
        iValueMica = 0; //SYS_MICSPK_DIGITAL_MAX - 2;
        ak_print_normal("Set max volume to MIC (%d)\r\n", iValueMica);
        ak_ai_set_volume(pAiHandle, iValueMica);
        iRet = 1;
    }
    
    /*
    We don't need to use this snipcode to set digital. 
    Because API setting analog will set value from 0 to 12.
    */
#if 0
    // volume digital
    iRet_d = doorbell_cfg_get_micd_gain(&iValueMicd);
    if(iRet_d == 0)
    {
        ak_print_normal("Read Mic digital: %d\r\n", iValueMicd);
        ak_ai_set_volume(pAiHandle, iValueMicd);
    }
    else
    {
        ak_print_normal("Set max volume to MIC digital (%d)\r\n", SYS_MICSPK_DIGITAL_MAX);
        ak_ai_set_volume(pAiHandle, SYS_MICSPK_DIGITAL_MAX - 2);
        iRet += 1;
    }
#endif
    return iRet;
}

/* API set mic volume on flight */
int audio_set_volume_onflight(void)
{
    int iRet = 0;
    if(g_audio_handle)
    {
        iRet = audio_ai_set_volume(g_audio_handle);
    }
    return iRet;            
}

/* API set mic volume on flight */
int audio_set_volume_onflight_with_value(int iValue)
{
    int iRet = 0;
    if(g_audio_handle)
    {
        iRet = ak_ai_set_volume(g_audio_handle, iValue);
        ak_print_normal("%s value: %d (%d)\r\n", __func__, iValue, iRet);
    }
    return iRet;            
}

int audio_ai_set_agc_nr(void *pAiHandle)
{
    int iRetCfg = 0, iRet = 0;
    int iValue = 0;

    // volume analog
    iRetCfg = doorbell_cfg_get_agcnr(&iValue);
    if(iRetCfg == 0)
    {
        ak_print_normal("Read AGC NR: %d\r\n", iValue);
        if(iValue == 1){
            ak_ai_set_nr_agc(pAiHandle, 1);
        }
        else{
            ak_ai_set_nr_agc(pAiHandle, 0);
        }
    }
    else
    {
        ak_ai_set_nr_agc(pAiHandle, 1);
        iRet = 1;
    }
    return iRet;
}
int audio_ai_set_echo(void *pAiHandle)
{
    int iRetCfg = 0, iRet = 0;
    int iValue = 0;

    // volume analog
    iRetCfg = doorbell_cfg_get_echocancellation(&iValue);
    if(iRetCfg == 0)
    {
        ak_print_normal("Read echo: %d\r\n", iValue);
        if(iValue == 1){
            ak_ai_set_aec(pAiHandle, 1);
        }
        else{
            ak_ai_set_aec(pAiHandle, 0);
        }
    }
    else
    {
        ak_ai_set_aec(pAiHandle, 0);
        iRet = 1;
    }
    return iRet;
}

#if 0
// void cmd_aenc_test(int argc, char **args)
void *ak_audio_encode_task(void *arg)
{
	// char * type = args[1];
	int total_ts = 0;

	int encode_type = AK_AUDIO_TYPE_PCM; //get_encode_type(type);

	struct ak_date systime;
	struct frame frame = {0};
	int ret = -1;

	unsigned long first_ts = 0;
	int first_time = 0;
	char file_path[255] = {0};
	unsigned long free_size = 0; 
	void *ai_handle = NULL;
	void *aenc_handle = NULL;
	
	//audio input imfo
	struct pcm_param ai_param;// = {0};
	ai_param.sample_bits = 16;
	ai_param.channel_num = AUDIO_CHANNEL_MONO;
	ai_param.sample_rate = 16000;

    ak_print_normal_ex("----- ak_audio_encode_task start !!! -----\n");
	ak_print_normal_ex("AI version: %s\n\n", ak_ai_get_version());
	ak_print_normal_ex("AENC version: %s\n\n", ak_aenc_get_version());
	

	/* 1. open audio input */
    ai_handle = ak_ai_open(&ai_param);
	if(NULL == ai_handle) 
	{
        ak_print_error_ex("-------------ak_ai_open failed!-------------\r\n");
		return;
    }

    g_audio_handle = ai_handle;

    ak_print_normal_ex("ak_ai_open OK !\n");
	ak_ai_set_source(ai_handle, AI_SOURCE_MIC);
	/*
		AEC_FRAME_SIZE  is  256£¬in order to prevent droping data,
		it is needed  (frame_size/AEC_FRAME_SIZE = 0), at  the same 
		time, it  need  think of DAC  L2buf  , so  frame_interval  should  
		be  set  32ms¡£
	*/
    // ak_ai_set_frame_interval(ai_handle, 32); CFG_AUDIO_DURATION
    ak_ai_set_frame_interval(ai_handle, CFG_AUDIO_DURATION);
    // ak_ai_set_volume(ai_handle, 5);

#ifdef CFG_AUDIO_MIC_GAIN_DEFAULT
	ak_ai_set_volume(ai_handle, CFG_AUDIO_MIC_GAIN_DEFAULT);
#else
    if(audio_ai_set_volume(ai_handle) != 0)
    {
        ak_print_error_ex("AI set volume failed!\r\n");
    }
#endif
    ak_ai_clear_frame_buffer(ai_handle);

    // ak_ai_set_resample(ai_handle, enable);
    // if(audio_ai_set_agc_nr(ai_handle) != 0)
    // {
    //     ak_print_error_ex("AI set AGC NR failed!\r\n");
    // }
	// ak_ai_set_nr_agc(ai_handle, 1);
	
	/*
		if enable aec,  resample must be disable
    */
    // ak_ai_set_resample(ai_handle, enable);
    // ak_ai_set_nr_agc(ai_handle, 0);
    // ak_ai_set_aec(ai_handle, 0);
    ak_print_normal("Comment AEC AGC RESAMPLE!!!!!\r\n");
    // ak_print_normal_ex(">>AUDIO turn off AEC\r\n");
   
#ifdef DOORBELL_TINKLE820
    ak_print_error_ex("wait i2c init video done!...\r\n");
    while(g_iFirstFrameOK == 0)
    {
        ak_print_error_ex(".");
        ak_sleep_ms(100);
    }
    fmcfg_cfg_fm2018();
    ak_print_error_ex("====================FM was be initialized=================\r\n");
#endif    
    
    //audio encode imfo
	struct audio_param aenc_param = {0};
	aenc_param.sample_bits = 16;
	aenc_param.channel_num = AUDIO_CHANNEL_STEREO; //AUDIO_CHANNEL_MONO;
	aenc_param.sample_rate = 16000;
	
	memset(&systime, 0, sizeof(systime));
    aenc_param.type = encode_type;

	/* 2. open audio encode */
    aenc_handle = ak_aenc_open(&aenc_param);
	
	if(NULL == aenc_handle) 
	{
        ak_print_error_ex("-------------aenc_handle open NULL!!!-----------\r\n");
    	goto ai_end;
    }
    
	ak_print_normal_ex("ak_aenc_open OK !\n");
	/* 3. bind audio input & encode */
	void *stream_handle = ak_aenc_request_stream(ai_handle, aenc_handle);
	if(NULL == stream_handle) 
	{
        ak_print_error_ex("stream_handle request NULL!!!\r\n");
    	goto aenc_end;
    }

    ak_print_normal(">>======>> AEC START >>=====>>");
    /* 4. get audio stream and write it to file */
    audio_encrypt_send_spi_queue(stream_handle);

    ret = 0;
	if(NULL != stream_handle) 
	{
    	ak_aenc_cancel_stream(stream_handle);
    }

aenc_end:
	if(NULL != aenc_handle) 
	{
    	ak_aenc_close(aenc_handle);
    }

ai_end:
    if(NULL != ai_handle) 
	{		
		// ak_ai_release_frame(ai_handle, &frame);
        ak_ai_close(ai_handle);
        g_audio_handle = NULL;
    }

fp_end:	
    ak_print_normal("----- audio_enc demo exit -----\n");
}
#endif


/*---------------------------------------------------------------------------*/
/*                           TEST DRIVER                                     */
/*---------------------------------------------------------------------------*/
#define RINGBUF_LOOPBACK_LEN                (32*1024)
ring_buffer_t *g_pRbLoopback;
static int g_loopback_cnt_display = 0;

static int audio_init_buffer_loopback(void)
{
    int iRet = 0;
    g_pRbLoopback = Ringbuf_Init(RINGBUF_LOOPBACK_LEN);
    if(g_pRbLoopback == NULL)
    {
        ak_print_error("%s g_pRbLoopback NULL!\r\n", __func__);
        iRet = -1;
    }
    else
    {
        ak_print_normal("%s initialized buffer with %d bytes\r\n", \
                            __func__, RINGBUF_LOOPBACK_LEN);
    }
    return iRet;
}

static int audio_deinit_buffer_loopback(void)
{
    int iRet = 0, i32LenRb = 0;
    if(g_pRbLoopback)
    {
        Ringbuf_Free(g_pRbLoopback);
        ak_print_normal("%s free g_pRbLoopback\r\n", __func__);
    }
    return iRet;
}

int audio_loopback_send_queue(char *pBuffer, int length)
{
    int iRet = 0, i32LenRb = 0;
    if(!g_pRbLoopback || !pBuffer || length < 0)
    {
        ak_print_error_ex("Parameter is invalid\n");
        return -1;
    }
    i32LenRb = Ringbuf_GetAvailSpace(g_pRbLoopback);
    if(i32LenRb >= length)
    {
        iRet = Ringbuf_WriteBuf(g_pRbLoopback, (unsigned char *)pBuffer, length);
    }
    else
    {
        ak_print_error_ex("Rb loopback is full\n");
        iRet = -2;
    }
    return iRet;
}

int audio_loopback_get_data(char *pBuffer, int amount)
{
    int iRet = 0, iAvailData = 0;
    if(!g_pRbLoopback || !pBuffer || amount <= 0)
    {
        return -1;
    }
    iAvailData = Ringbuf_GetAvailSpace(g_pRbLoopback);
    if(iAvailData > 0)
    {
        if(iAvailData >= amount)
        {
            iRet = Ringbuf_ReadBuf(g_pRbLoopback, pBuffer, amount);
            if(iRet >= 0)
            {
                iRet = amount;
            }
        }
        else
        {
            iRet = Ringbuf_ReadBuf(g_pRbLoopback, pBuffer, iAvailData);
            if(iRet >= 0)
            {
                iRet = iAvailData;
            }
        }
    }
    return iRet;
}

int audio_loopback_flush_buffer(void)
{
    char *pBuffer = NULL;
    int len = 0;
    
    pBuffer = (char *)malloc(1024);
    if(pBuffer == NULL)
    {
        return -1;
    }
    len = Ringbuf_GetAvailSpace(g_pRbLoopback);
    while(len > 0)
    {
        Ringbuf_ReadBuf(g_pRbLoopback, pBuffer, 1024);
        len = Ringbuf_GetAvailSpace(g_pRbLoopback);
    }
    return 0;
}

#if 1
void audio_test_driver_encrypt_send_spi_queue(void *ai_handle)
{
    // struct list_head stream_head ;
    // struct aenc_entry *entry = NULL; 
    // struct aenc_entry *ptr = NULL;
    unsigned long first_ts = 0, frame_ts = 0;
	// int total_ts = time;
    bool first_get_flag = false;
    // INIT_LIST_HEAD(&stream_head);

    char *pDtG722Out;
    char *pDtAuSpiOut;
    int i32Ret = 0;
    int packet_counter = 0;
    int out_len = 0;
    int tmp_len = 0;
    int s32RetVal = 0;
    int i32G722OutLen = 0;
    unsigned int u32CntPkt = 0;
    ENUM_SYS_MODE_t sys_get_mode;
    unsigned long ulLastTick = 0, ulCurrentTick = 0;
    
    int i32LenRb = 0;
    int i32CntG722 = 0, i32CntPcm = 0;

	
	int iLenPacketSend = 0;
		int iAgcCalGain = 0;
		int iLengthSampling = 0;
		int iCntSampling = 0;
		int iCntAgcPrintf = 0;
		char acDebugAgcDb[256];
		double dAgcDb = 0;

    struct frame frame = {0};

    /* G722 */
    G722_ENC_CTX *g722_ectx;
    g722_ectx = g722_encoder_new(64000,0);
    if (g722_ectx == NULL)
    {
    	ak_print_error_ex("g722_encoder_new() failed\r\n");
    }
    if(init_ringbuffer_32ms_issue() == -1)
    {
        ak_print_error_ex("Not enough buffer for audio send!!!\r\n");
        return;
    }

    /* Maximun we only need g722 buffer equal pcm buffer! */
    // pDtG722Out = (char *)malloc(CFG_AUDIO_BUF_SPI_OUT);
    pDtG722Out = (char *)malloc(CFG_AUDIO_FRAME_SIZE);
    if(pDtG722Out == NULL)
    {
        printf("Cannot allocate memory for pDtG722Out send SPI\n\r");
        goto g722_exit;
    }
    pDtAuSpiOut = (char *)malloc(CFG_AUDIO_BUF_SPI_OUT);
    if(pDtAuSpiOut == NULL)
    {
        printf("Cannot allocate memory for pDtAuSpiOut send SPI\n\r");
        goto g722_exit;
    }

    /* Buffer loopback */
    audio_init_buffer_loopback();

    ulCurrentTick = get_tick_count();
	while(1)
	{	
        ulLastTick = get_tick_count();
        sys_get_mode = eSysModeGet();
        if(E_SYS_MODE_FWUPGRADE == sys_get_mode)
        {
            ak_print_normal("%s exit. Mode: %d\r\n", __func__, sys_get_mode);
            break;
        }

        i32Ret = ak_ai_get_frame(ai_handle, &frame, 1);
        if(i32Ret == 0)
        {
            /* data for loopback */
            if(E_SYS_MODE_FTEST == sys_get_mode)
            {
                // check tone gen
                if(tone_gen_get_running() == 0)
                {
                    i32Ret = audio_loopback_send_queue(frame.data, frame.len);
                    if(i32Ret != 0)
                    {
                        if(i32Ret != -2){
                            ak_print_error("Loopback get data failed!(%d)\n", i32Ret);
                        }
                    }
                    else
                    {
                        g_loopback_cnt_display++;
                        if(g_loopback_cnt_display == 300){
                            ak_print_normal("Loopback audio send to queue: %d\n", frame.len);
                            g_loopback_cnt_display = 0;
                        }
                    }
                }
                else
                {
                    // dont send data to loopback, because tone gen is on
                }
            }

            if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
            {
#if (RECORDING_SDCARD == 1)
    /* Feed data to FLV muxer before send over SPI */
    {
		int16_t temp_data[1024], *temp_ptr=frame.data;
		int i;
		for(i=0;i<(frame.len/2);i++)
			temp_data[i]=temp_ptr[i*2];
		if(FLVEnginePushAudioData(temp_data, frame.len/2, frame.ts) != 0)
		{
			ak_print_normal("\nFLVEnginePushAudioData size %d, ts %lu fail\n", frame.len, frame.ts);
		}
		else
		{
			// ak_print_normal("FLVEnginePushAudioData size %d, ts %lu\n", entry->stream.len, entry->stream.ts);
		}
    }
#endif
				#if 0
					/* Implement automatic gain control */
	            	if(iCntSampling < 4){
						iAgcCalGain += audio_agc_calculate_gain(frame.data, frame.len);
						// dAgcDb += audio_agc_calculate_gain_db(frame.data, frame.len);
						//printf("%lfdb \r\n", audio_agc_calculate_gain_db(frame.data, frame.len));
						//printf("iAgcCalGain: %d\r\n", iAgcCalGain);
						iLengthSampling += frame.len;
						iCntSampling++;
					}
					else
					{
						iAgcCalGain = iAgcCalGain/4;
						iCntAgcPrintf ++;
						
						if(iCntAgcPrintf != 0 && iCntAgcPrintf % 10 == 0){
							printf("\n");
							iCntAgcPrintf = 0;
						}
						printf("%d ", iAgcCalGain);
						if(g_agc_enable)
						{
							audio_gain_adaptivity(iAgcCalGain);
						}
						iCntSampling = 0;
						iLengthSampling = 0;
						iAgcCalGain = 0;
						/*
						memset(acDebugAgcDb, 0x00, sizeof(acDebugAgcDb));
						snprintf(acDebugAgcDb, sizeof(acDebugAgcDb), "%5.2lf dB\n", dAgcDb);
						printf("%s", acDebugAgcDb);
						dAgcDb = 0;
						*/
					}
				#endif
						
//		        ak_print_normal("\nSize GET FRAME: %d\r\n", frame.len);
			    i32G722OutLen = g722_encode(g722_ectx,(int16_t *)frame.data,frame.len/2,pDtG722Out);
                // save data to ringbuffer, wait enough 64ms data.
                i32LenRb = Ringbuf_GetAvailSpace(g_pRbAudio32ms);
                if(i32LenRb >= i32G722OutLen)
                {
                    Ringbuf_WriteBuf(g_pRbAudio32ms, pDtG722Out, i32G722OutLen);
                }
                else
                {
                    ak_print_error_ex("Rb32ms is full!\r\n");
                }
                i32CntPcm += frame.len;
            }
            ak_ai_release_frame(ai_handle, &frame);
		}
        else
    	{
            // ak_sleep_ms(AUDIO_TASK_NOFRAME);
			ak_sleep_ms(10);
		    // continue;
        }
	
        if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
        {
            // check length >256 and send
            i32LenRb = Ringbuf_GetAvailData(g_pRbAudio32ms);
            if(i32LenRb >= CFG_AUDIO_LENGTH_SEND)
            {
                i32G722OutLen = CFG_AUDIO_LENGTH_SEND;
                i32Ret = Ringbuf_ReadBuf(g_pRbAudio32ms, pDtG722Out, i32G722OutLen);
        
                //TODO: one way talkback
                // drop all data audio when talkback, don't send it
            #if(TALKBACK_2WAY == 0)
                if(g_iOneWayTalkback == 0)
                {
            #endif
                    if(i32Ret == 0)
                    {
                        packet_lock_mutex();
                        i32Ret = packet_create(85, &packet_counter, (char *)(pDtG722Out), \
                                    i32G722OutLen, pDtAuSpiOut, &out_len);
                        packet_unlock_mutex();
//                        i32CntG722 += 256;
                        i32CntG722 += CFG_AUDIO_LENGTH_SEND;
                        tmp_len = 0;
                        while(tmp_len < out_len)
                        {
                            s32RetVal = send_spi_data((pDtAuSpiOut + tmp_len), 1024);
                            if(s32RetVal != 1024)
                            {
                                ak_sleep_ms(20);
                                // ak_print_normal("[ERROR] send data spi failed (%d)!\n", s32RetVal);
                                ak_print_normal("#");
                                break;
                            }
                            else{
                                tmp_len +=1024;
                                u32CntPkt++;
                            }
                        }
                    }//if(i32Ret != 0)
                    else
                    {
                        ak_print_error("%s: Read buf failed (%d)!\r\n", __func__, i32Ret);
                    }
            
            #if(TALKBACK_2WAY == 0)
                }
                else
                {
                    audio_send_dummy();
                    audio_send_dummy();
                    // printf("L:%d_", i32LenRb);
                    // ak_sleep_ms(15);
                    ak_sleep_ms(10);
                }//if(g_iOneWayTalkback == 0)
            #endif
            }

        }
        else
        {
            ak_sleep_ms(10);
        }

		// count data
		if((ulLastTick - ulCurrentTick) >= 10000)
        {
            if(g_iP2pSdStreamStart == 0)
            {
                if(sys_get_mode != E_SYS_MODE_FTEST){
                    printf("A_sent:%d pps, g722:%d, size_encode:%d\n", u32CntPkt/10, i32CntG722/10, i32G722OutLen/10);
                }
            }
            
            u32CntPkt = 0;
            i32CntG722 = 0;
            i32CntPcm = 0;
            ulCurrentTick = ulLastTick;
        }

		
        sys_get_mode = eSysModeGet();
        // if(E_SYS_MODE_FTEST == sys_get_mode || \
        //     E_SYS_MODE_FWUPGRADE == sys_get_mode)
        // {
        //     ak_print_normal("%s exit. Mode: %d\r\n", __func__, sys_get_mode);
        //     break;
        // }
        /* data for loopback */
	}

g722_exit:
    audio_deinit_buffer_loopback(); // buffer for loopback
    free(pDtG722Out);
    free(pDtAuSpiOut);
    g722_encoder_destroy(g722_ectx);
    deinit_ringbuffer_32ms_issue();
	ak_print_normal("Audio exit loop!\n\n");
}

void *ak_audio_test_driver_task(void *arg)
{
	int total_ts = 0;
	int encode_type = AK_AUDIO_TYPE_PCM; //get_encode_type(type);
	struct ak_date systime;
	struct frame frame = {0};
	int ret = -1;

	unsigned long first_ts = 0;
	int first_time = 0;
	char file_path[255] = {0};
	unsigned long free_size = 0; 
	void *ai_handle = NULL;
	// void *aenc_handle = NULL;
	//audio input imfo
	struct pcm_param ai_param;// = {0};
	ai_param.sample_bits = 16;
	ai_param.channel_num = AUDIO_CHANNEL_MONO;
	ai_param.sample_rate = 16000;

	ak_print_normal("%s AI version: %s\n\n", __func__, ak_ai_get_version());

	/* 1. open audio input */
    ai_handle = ak_ai_open(&ai_param);
	if(NULL == ai_handle) 
	{
        ak_print_error_ex("-------------ak_ai_open failed!-------------\r\n");
		return;
    }

    g_audio_handle = ai_handle;

    ak_print_normal_ex("ak_ai_open OK !\n");
	ak_ai_set_source(ai_handle, AI_SOURCE_MIC);
	/*
		AEC_FRAME_SIZE  is  256£¬in order to prevent droping data,
		it is needed  (frame_size/AEC_FRAME_SIZE = 0), at  the same 
		time, it  need  think of DAC  L2buf  , so  frame_interval  should  
		be  set  32ms¡£
	*/
    // ak_ai_set_frame_interval(ai_handle, 32); CFG_AUDIO_DURATION
    ak_ai_set_frame_interval(ai_handle, CFG_AUDIO_DURATION);
    
    if(audio_ai_set_volume(ai_handle) != 0)
    {
    	ak_print_error_ex("AI set volume failed!\r\n");
    }
    ak_ai_clear_frame_buffer(ai_handle);
    
    // TODO agc nr
    if(audio_ai_set_agc_nr(ai_handle) != 0)
    {
        ak_print_error_ex("Set AGC NR failed!\n");
    }

#ifdef DOORBELL_TINKLE820
    printf("wait i2c init video done!...\r\n");
    while(g_iFirstFrameOK == 0)
    {
        printf(".");
        ak_sleep_ms(100);
    }
    fmcfg_cfg_fm2018();
    ak_print_normal("====================FM was be initialized=================\r\n");
#endif    
    
    audio_test_driver_encrypt_send_spi_queue(ai_handle);

ai_end:
    if(NULL != ai_handle) 
	{		
		// ak_ai_release_frame(ai_handle, &frame);
        ak_ai_close(ai_handle);
        g_audio_handle = NULL;
    }

fp_end:	
    ak_print_normal("----- audio_enc demo exit -----\n");
}

#endif


/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/

