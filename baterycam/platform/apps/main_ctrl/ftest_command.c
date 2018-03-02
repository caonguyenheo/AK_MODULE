/**
  ******************************************************************************
  * File Name          : ftest.c
  * Description        : This file contain functions for factory test
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
#include <math.h>

#include "ftest_command.h"
#include <stdlib.h>
#include "main_ctrl.h"
#include "ak_apps_config.h"
#include "ak_common.h"
#include "ak_ai.h"
#include "ak_aenc.h"
#include "ak_login.h"
#include "kernel.h"
#include "hal_timer.h"

#include "fm2018_drv.h"

#include "ak_audio.h"
#include "ak_talkback.h"

/* uvc */
#include "anyka_types.h"
#include "ak_drv_uvc.h"
#include "ak_vi.h"
#include "command.h"
#include "ak_common.h"
#include "ak_drv_ircut.h"
#include "kernel.h"
#include "ak_venc.h"
#include "ak_vpss.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/

static int g_atecmd_led_on = 0;


void *adc_drv;
void *ao_handle;

static int g_stop_testing_audioloop = 0;
static char g_loopback_running = 0; // 0 dont running, 1 running

void ftest_audioloop_stop(void)
{
    g_stop_testing_audioloop = 1;
    g_loopback_running = 0;
}
void ftest_audioloop_start(void)
{
    g_stop_testing_audioloop = 0;
    g_loopback_running = 1;
}

int ftest_audioloop_get_mode_stop(void)
{
    return g_loopback_running;
}

void ftest_mount_sd(void)
{
    // if (0 ==ak_mount_fs(DEV_MMCBLOCK, 0, ""))
    // if (0 ==ak_mount_fs(1, 0, ""))
    //     ak_print_normal("ao mount sd ok!\n");
    // else
    //     ak_print_error("ao mount sd fail!\n");
    sdcard_mount();
}

/*---------------------------------------------------------------------------*/
/*                          FTEST AUDIO LOOPBACK                             */
/*---------------------------------------------------------------------------*/


/* static function for audio loopback test audio */
/* API set speaker volume on flight */
int ftest_speaker_set_volume_onflight_with_value(int iValue)
{
    int iRet = 0;
    if(ao_handle)
    {
        iRet = ak_ao_set_volume(ao_handle, iValue);
        ak_print_normal("%s value: %d (%d)\r\n", __func__, iValue, iRet);
    }
    return iRet;            
}

/* API set mic volume on flight */
int ftest_mic_set_volume_onflight_with_value(int iValue)
{
    int iRet = 0;
    if(adc_drv)
    {
        iRet = ak_ai_set_volume(adc_drv, iValue);
        ak_print_normal("%s value: %d (%d)\r\n", __func__, iValue, iRet);
    }
    return iRet;            
}

#if 0
int ftest_cmd_audio_loopback(int argc, char **argv)
{
    int ret, i;
    // init audio
    struct pcm_param input;
    unsigned long gain = 0;
    unsigned long frame_size = 0;
    unsigned short *s_buf;
    unsigned char *tmp;
    struct frame aframe; 
    int cnt = 0;

    // init speaker
    struct pcm_param param = {0}; // ref ak_adec_demo.c
    unsigned int tmpBuf_size = 0;
    unsigned char *tmpBuf = NULL;
    unsigned short *ptmpbuf = NULL;
    int send_size, size;
    int encode_type = AK_AUDIO_TYPE_PCM; //get_encode_type(type);

    // sdcard
    unsigned short *tmpBuf1 = NULL;    //for data double
    
    void *aenc_handle = NULL;
    struct list_head stream_head ;
	struct aenc_entry *entry = NULL; 
	struct aenc_entry *ptr = NULL;
    INIT_LIST_HEAD(&stream_head);
    //-------------------------------------------------------------------------
    // init audio
    input.sample_rate = CFG_AUDIO_SAMPLE_RATE;
    input.channel_num = CFG_AUDIO_CHANNEL_NUM;
    input.sample_bits = CFG_AUDIO_SAMPLE_BITS;

    gain = CFG_AUDIO_GAIN;
    frame_size = CFG_AUDIO_FRAME_SIZE;
    s_buf = malloc(1024*64);
    adc_drv = ak_ai_open(&input);
    if(adc_drv == NULL)
    {
        ak_print_normal("adc_drv NULL!\n");
        goto ai_end;
    }

    ak_ai_set_source(adc_drv, AI_SOURCE_MIC);
    ak_ai_set_frame_interval(adc_drv, CFG_AUDIO_DURATION);
    ak_ai_set_volume(adc_drv, 0x05);
    // if(audio_ai_set_volume(adc_drv) != 0)
    // {
    //     ak_print_error_ex("AI set volume failed!\r\n");
    // }
    // else
    // {
    //     ak_print_error_ex("AI set volume OK!\r\n");
    // }
	ak_ai_clear_frame_buffer(adc_drv);

    // ak_ai_set_resample(adc_drv, 1);
	ak_ai_set_nr_agc(adc_drv, 0);
    ak_ai_set_aec(adc_drv, 0);

    //audio encode imfo
	struct audio_param aenc_param = {0};
	aenc_param.sample_bits = 16;
	aenc_param.channel_num = AUDIO_CHANNEL_STEREO; // AUDIO_CHANNEL_MONO;
	aenc_param.sample_rate = 8000;
    aenc_param.type = encode_type;
	/* 2. open audio encode */
    aenc_handle = ak_aenc_open(&aenc_param);	
	if(NULL == aenc_handle) 
	{
    	goto ai_end;
    }
    void *stream_handle = ak_aenc_request_stream(adc_drv, aenc_handle);
	if(NULL == stream_handle) 
	{
    	goto aenc_end;
    }

    tmp = (unsigned char*)s_buf;
    //-------------------------------------------------------------------------
    // init speaker
    param.sample_bits = 16;
    param.sample_rate = 8000;
    param.channel_num = AUDIO_CHANNEL_MONO; //1; //mono

    ao_handle = ak_ao_open(&param);
    if(ao_handle == NULL)
    {
        ak_print_error("ao open false\n");
        goto ao_end;
    }
 
    tmpBuf_size = ak_ao_get_frame_size(ao_handle);
    ak_print_normal("SPEAKER ao frame size =%d\n", tmpBuf_size);
    tmpBuf_size = CFG_AUDIO_FRAME_SIZE;
    if(ak_ao_set_frame_size(ao_handle, tmpBuf_size) != 0)
    {
        ak_print_error_ex("Why don't set ao frame size successful?");
    }
    else
    {
        tmpBuf_size = ak_ao_get_frame_size(ao_handle);
        ak_print_normal("Confirm SPEAKER ao frame size =%d\n", tmpBuf_size);
    }

    tmpBuf = (unsigned char *)malloc(tmpBuf_size * 2);
    if(NULL == tmpBuf)
    {
        ak_print_error("ao demo malloc false!\n");
        goto ao_end;
    }
    //set volume
    ak_ao_set_volume(ao_handle, 0x04);
    // if(talkback_ao_set_volume(ao_handle) != 0)
    // {
    //     ak_print_error_ex("AO set volume failed!\r\n");
    // }
    ak_ao_enable_speaker(ao_handle, SPK_ENABLE);

#ifdef DOORBELL_TINKLE820
    ak_print_error_ex("ak_ao_enable_speaker active low (open) for FM\r\n");
    fmcfg_cfg_fm2018();
    // fm2018_analog_communication_mode();
    // ak_print_normal("====================FM was be initialized=================\r\n");
#endif

    // ak_ai_set_aec(adc_drv, 1);

    ak_print_normal("Init mic and speaker OK!\n");
    // loop
    while(g_stop_testing_audioloop == 0)
    {
        if(0 == ak_aenc_get_stream(stream_handle, &stream_head))
        {
            list_for_each_entry_safe(entry, ptr, &stream_head, list)
			{
				if(NULL != entry) 
				{
					tmpBuf1 = (char *)(entry->stream.data);
                    ret = entry->stream.len;
                    ptmpbuf = (unsigned short *)tmpBuf;
                #if 0
                    for(i = 0; i < ret/2; i++)
                    {
                        *(ptmpbuf + 2*i) = *(tmpBuf1 + i);
                        *(ptmpbuf + 2*i +1) = *(tmpBuf1 + i);
                    }
                    ret = ret*2;
                #else
                    memcpy(tmpBuf, entry->stream.data, ret);
                #endif
                    // ak_print_normal("MIC s:%d, SPK s:%d\n", entry->stream.len, ret);

                    // // play speaker
                    send_size = 0;
                    #if 1
                    while(ret > 0)
                    {
                        size = ak_ao_send_frame(ao_handle, tmpBuf + send_size, ret, 0);
                        // ak_print_normal("-Send audio: %d\n", size);
                        if(size < 0)
                        {
                            ak_print_error("ao send frame error!  %d \n", size);
                            break;
                        }
                        else
                        {
                            send_size += size; 
                            ret = ret - size;

                            cnt++;
                            if(cnt%10 == 0){
                                printf("#%d ", size);
                                cnt = 0;
                            }
                        }
                        // mini_delay(10);
                    }
                    #endif
                    ak_aenc_release_stream(entry);
				}
			}
        }
        else
        {
            ak_sleep_ms(AUDIO_TASK_NOFRAME);
            continue;
        }
    }
    // mini_delay(900);  //wait data to DAC
aenc_end:
    if(NULL != stream_handle) 
	{
    	ak_aenc_cancel_stream(stream_handle);
    }

	if(NULL != aenc_handle) 
	{
    	ak_aenc_close(aenc_handle);
    }

ao_end:
    //close SPK
    ak_ao_enable_speaker(ao_handle, SPK_DISABLE);
    // ak_print_error_ex("ak_ao_enable_speaker active high (close) for FM\r\n");
    ak_ao_close(ao_handle);
    if(tmpBuf != NULL){
        free(tmpBuf);
    }
ai_end:
    ak_ai_release_frame(adc_drv, &aframe);
    if(s_buf)
    {
        free(s_buf);
    }
    if(adc_drv)
    {
        ak_ai_close(adc_drv);
    }
    ak_thread_exit();
    return 0;
}
#endif

// void* ftest_audio_loopback_thread(void *arg)
// {   
//     ak_login_all_filter();
// 	ak_login_all_encode();
// 	ak_login_all_decode();
//     ftest_cmd_audio_loopback(0, NULL);
// }

/* Test speaker */
static short audio_doorbell_buf[]=
{
	#include "4204.txt"
};

void* ftest_speaker_thread(void *arg)
{
	struct pcm_param info;
	int ret;
	unsigned int fram_size = 0;
	int *fd = NULL;
	unsigned char file_over = 0;
    int send_size, size;
    int count  = 5;
	
	unsigned char* curPtr = (unsigned char*)audio_doorbell_buf;
	unsigned char* endPtr = (unsigned char*)audio_doorbell_buf + sizeof(audio_doorbell_buf);
	unsigned char* bkCurPtr = (unsigned char*)audio_doorbell_buf;
    unsigned char* bkEndPtr = (unsigned char*)audio_doorbell_buf + sizeof(audio_doorbell_buf);

	unsigned char *tmpBuf;
	info.sample_bits = 16;
	info.sample_rate = 24000;
	info.channel_num = 1;
	
	//open ao
	ao_handle = ak_ao_open(&info);
	if(ao_handle == NULL)
	{
		ak_print_error("ao open false\n");
		return ;
	}
	fram_size = ak_ao_get_frame_size(ao_handle);
	ak_print_normal("ao frame size =%d\n", fram_size);
	
	tmpBuf = malloc(fram_size);
	if(NULL == tmpBuf)
	{
		ak_print_error("ao demo malloc false!\n");
		return ;
	}
	//set volume
    // ak_ao_set_volume(fd, 0x05);
    if(talkback_ao_set_volume(ao_handle) != 0)
    {
        ak_print_error_ex("AO set volume failed!\r\n");
    }
	
	ak_print_normal("star trans\n");
    ak_ao_enable_speaker(ao_handle, SPK_ENABLE);
    // ak_print_error_ex("ak_ao_enable_speaker active low (open) for FM\r\n");

    ak_print_normal("curPtr: %p, bkCurPtr: %p\n", curPtr, bkCurPtr);
    ak_print_normal("endPtr: %p, bkEndPtr: %p\n", endPtr, bkEndPtr);
	while (1)
	{
		if(endPtr - curPtr > fram_size)
		{
			memcpy(tmpBuf, curPtr, fram_size);
			ret = fram_size;
			curPtr += fram_size;
		}
		else
		{
			ret = endPtr - curPtr;
			memcpy(tmpBuf, curPtr, ret);
			file_over = 1;
		}		
		send_size = 0;
		while(ret > 0)
		{
			size = ak_ao_send_frame(ao_handle, tmpBuf + send_size, ret, 0);
			if(size < 0)
			{
				ak_print_error("ao send frame error!  %d \n", size);
				break;
			}
			else
			{
				send_size += size; 
				ret = ret - size;
			}
		}
		if(file_over)
		{
            curPtr = bkCurPtr;
            endPtr = bkEndPtr;
            ak_print_normal("curPtr: %p, bkCurPtr: %p\n", curPtr, bkCurPtr);
            ak_print_normal("endPtr: %p, bkEndPtr: %p\n", endPtr, bkEndPtr);
            mini_delay(500);
            file_over = 0;
            count--;
            if(count <= 0)
            {
                break;
            }
		}
	}

	mini_delay(900);
    ak_ao_enable_speaker(ao_handle, SPK_DISABLE);
    // ak_print_error_ex("ak_ao_enable_speaker active high (close) for FM\r\n");
	//close ao
    ak_ao_close(ao_handle);
    ao_handle = NULL;
    free(tmpBuf);
    ak_thread_exit();
}

/*---------------------------------------------------------------------------*/
/*                          FTEST STRUCTURE                                  */
/*---------------------------------------------------------------------------*/
void *ftest_test_structure_thread(void *arg)
{
    int count = 0;
    ak_pthread_t id = ak_thread_get_tid();
    ak_print_normal_ex("Arg: %s\n", (char *)arg);
    ak_print_normal_ex("Thread start with id: %u\n", id);
    while(1)
    {
        count++;
        ak_sleep_ms(1000);
        if(count % 10 == 0){
            ak_print_normal_ex("(%u) %d\n", id, count);
        }
    }
    ak_thread_exit();
    return NULL;
}

/*---------------------------------------------------------------------------*/
/*                          FTEST TONE GENERATION                            */
/*---------------------------------------------------------------------------*/
static int g_tone_generate_on = 0;
/*
 * PURPOSE : Generate beep buffer to write to speaker
 * INPUT   : [freq]   - Frequency
 *           [buff]   - Buffer
 *           [bufLen] - Length of buffer
 * OUTPUT  : [buff]   - Buffer
 * RETURN  : None
 * DESCRIPT: None
 */
static void generate_beep(uint32_t freq, int16_t *buff, size_t bufLen)
{
    unsigned int i;
    int16_t gain = 16000;

    // ak_print_normal_ex("freq: %u\n", freq);
    if(buff)
    {
        for(i = 0; i < bufLen; ++i){
            buff[i] = gain * sin((float)2.0 * (float)M_PI * freq / 8000 * (i+1));
        }
    }   
}
static void generate_beep_s16khz(uint32_t freq, int16_t *buff, size_t srate)
{
    unsigned int i;
    int16_t gain = 16000;

    // ak_print_normal_ex("freq: %u\n", freq);
    if(buff)
    {
        for(i = 0; i < srate; ++i){
            // buff[i] = gain * sin((float)2.0 * (float)M_PI * freq / 8000 * (i+1));
            buff[i] = gain * sin((float)2.0 * (float)M_PI * freq / 16000 * (i+1));
        }
    }   
}
#if 0
void* ftest_speaker_tone_generate_thread(void *arg)
{
	struct pcm_param info;
	int ret;
	unsigned int fram_size = 0;
	int *fd = NULL;
	int send_size, size;

    int interval = 5;
    unsigned long ulLastTick = 0, ulCurrentTick = 0;

    int *pParam = (int *)arg;
    int freq = atoi(arg);

	unsigned char *tmpBuf;
	info.sample_bits = 16;
	info.sample_rate = 8000;
	info.channel_num = 1;
	
    if(freq > 20000 || freq <= 0)
    {
        ak_print_normal_ex("Set freq default 1kHz! (%d)\n", freq);
        freq = 1000;
    }

    ak_print_normal_ex("Generate freq: %d\n", freq);
	//open ao
	fd = ak_ao_open(&info);
	if(fd == NULL)
	{
		ak_print_error("ao open false\n");
		return ;
	}
	fram_size = ak_ao_get_frame_size(fd);
	ak_print_normal("ao frame size =%d\n", fram_size);
	
	tmpBuf = malloc(16000);
	if(NULL == tmpBuf)
	{
		ak_print_error("ao demo malloc false!\n");
		return ;
	}
	//set volume
    // ak_ao_set_volume(fd, 0x02);
    if(talkback_ao_set_volume(ao_handle) != 0)
    {
        ak_print_error_ex("AO set volume failed!\r\n");
    }
	
	ak_print_normal("star trans\n");
    ak_ao_enable_speaker(fd, SPK_ENABLE);
    // ak_print_error_ex("ak_ao_enable_speaker active low (open) for FM\r\n");

    ulLastTick = get_tick_count();
	while (g_tone_generate_on == 1)
	{
        send_size = 0;
        generate_beep(freq, (int16_t *)tmpBuf, 8000);
        ret = fram_size;
		while(ret > 0)
		{
            
			size = ak_ao_send_frame(fd, tmpBuf + send_size, ret, 0);
			if(size < 0)
			{
				ak_print_error("ao send frame error!  %d \n", size);
				break;
			}
			else
			{
				send_size += size; 
				ret = ret - size;
			}
		}
        ulCurrentTick = get_tick_count();
        if((ulCurrentTick - ulLastTick) > interval*1000)
        {
            ak_print_normal_ex("Time out play beep!\n");
            break;
        }
        // ak_sleep_ms(100);
	}

    ak_print_normal_ex("exit thread!\n");
    ak_ao_enable_speaker(fd, SPK_DISABLE);
    // ak_print_error_ex("ak_ao_enable_speaker active high (close) for FM\r\n");
	//close ao
	ak_ao_close(fd);
	free(tmpBuf);
    ak_thread_exit();
    return NULL;
}
#else

void* ftest_speaker_tone_generate_thread(void *arg)
{
    int send_size = 0, ret = 0;
    char *tmpBuf = NULL;
    int srate = 1024; // sample rate

    int interval = 5; // seconds
    unsigned long ulLastTick = 0, ulCurrentTick = 0;
    int freq = atoi(arg);

    if(freq > 20000 || freq <= 0)
    {
        ak_print_normal_ex("Set freq default 1kHz! (%d)\n", freq);
        freq = 1000;
    }
    ak_print_normal_ex("Generate freq: %d\n", freq);
	tmpBuf = (char *)malloc(2*1024);
	if(NULL == tmpBuf)
	{
		ak_print_error("ao demo malloc false!\n");
		return ;
	}

    // generate_beep(freq, (int16_t *)tmpBuf, 16000);
    generate_beep_s16khz(freq, (int16_t *)tmpBuf, 1024);
    ulLastTick = get_tick_count();
    // we have 32000 bytes data play in 1000ms
    // so 1ms we need 32 bytes
    // 32ms need 1024 bytes
    send_size = 0;
	while (g_tone_generate_on == 1)
	{   
        ret = audio_loopback_send_queue(tmpBuf, 2*1024);
        // send_size += 1024;
        // send_size = send_size % (32*1024);
        // if(send_size >= 32000){
            // send_size = 0;
        // }
        if(ret != 0)
        {
            if(ret != -2){
                ak_print_error("Loopback ringbuf is full!(%d)\n", ret);
            }
            else
            {
                ak_print_error("Loopback send data failed!(%d)\n", ret);
            }
        }
        else
        {
            // ak_print_normal_ex("Send data tonegen size %d\n", 2*1024);
        }
        ulCurrentTick = get_tick_count();
        if((ulCurrentTick - ulLastTick) > interval*1000)
        {
            ak_print_normal_ex("Time out play beep!\n");
            g_tone_generate_on = 0;
            break;
        }
        ak_sleep_ms(40); // 32ms 
	}
    // audio_loopback_flush_buffer();
    // tone_gen_set_off();
    ak_print_normal_ex("exit thread!\n");
	free(tmpBuf);
    ak_thread_exit();
    return NULL;
}
#endif
void tone_gen_set_on(void)
{
    g_tone_generate_on = 1;
}
void tone_gen_set_off(void)
{
    g_tone_generate_on = 0;
}

/*
1: running
0: off
*/
int tone_gen_get_running(void)
{
    return g_tone_generate_on;
}


void atecmdled_set_on(void)
{
    g_atecmd_led_on = 1;
}
void atecmdled_set_off(void)
{
    g_atecmd_led_on = 0;
}


/*---------------------------------------------------------------------------*/
/*                          FTEST SYSTEM LOCAL TIME - NTP                    */
/*---------------------------------------------------------------------------*/
static void atecmd_localdate(long offset)
{
    struct ak_date systime;
    long local_second;
    long ret;
    int ret_val = 0;
    ret_val = ak_get_localdate(&systime);
    printf("Before Local date: %04d %02d %02d %02d %02d %02d (ret %d)\n", \
        systime.year, systime.month, systime.day, \
        systime.hour, systime.minute, systime.second, ret_val);
    local_second = ak_date_to_seconds(&systime);
    printf("local_second: %d\n", local_second);
    if(offset > 0)
    {
        local_second += offset;
        printf("add %d, local_second: %d\n", offset, local_second);

        ret = ak_seconds_to_date(local_second, &systime);
        printf("ak_seconds_to_date ret %d\n", ret);
        printf("Set local datetime");
        ret_val = ak_set_localdate(&systime);

        printf("After Local date: %04d %02d %02d %02d %02d %02d (ret %d)\n", \
            systime.year, systime.month, systime.day, \
            systime.hour, systime.minute, systime.second, ret_val);
    }    
}

void *ftest_test_ntp_thread(void *arg)
{
    int count = 0;
    long second = atoi(arg);
    ak_pthread_t id = ak_thread_get_tid();
    ak_print_normal_ex("Arg: %s\n", (char *)arg);
    ak_print_normal_ex("Thread start with id: %u\n", id);
    
    atecmd_localdate(second);

    ak_thread_exit();
    return NULL;
}


/*---------------------------------------------------------------------------*/
/*                                  UVC                                      */
/*---------------------------------------------------------------------------*/

#define FTEST_CH1_WIDTH	    1280
#define FTEST_CH1_HEIGHT	720
#define FTEST_CH2_WIDTH	    640
#define FTEST_CH2_HEIGHT	480

volatile static bool s_break = false;
static int g_iVideoTestUvcEnable = 0;

void videouvc_set_enable(void)
{
    g_iVideoTestUvcEnable = 1;
}

void videouvc_set_disable(void)
{
    g_iVideoTestUvcEnable = 0;
}

int videouvc_get_status(void)
{
    return g_iVideoTestUvcEnable;
}

static char *help[]={
	"uvc module demo",
	"usage:uvcdemo [yuv/mjpeg]\n"
};

static void cmd_uvc_signal(int signo)
{
    s_break = true;
}

static void *init_video_in(void)
{
	void *handle;

	/* open isp sdk */
	handle = ak_vi_open(VIDEO_DEV0);
	if (NULL == handle) {
		ak_print_error_ex("##########$$  ak_vi_open failed!\n");
		return NULL;
	} else {
		ak_print_normal("##$$  ak_vi_open success!--handle=%d\n",(int)handle);
	}

	/* get sensor resolution */
	struct video_resolution sensor_res = {0};
	if (ak_vi_get_sensor_resolution(handle, &sensor_res))
		ak_print_error_ex("ak_mpi_vi_get_sensor_res failed!\n");
	else
		ak_print_normal("ak_mpi_vi_get_sensor_res ok! w:%d, h:%d\n",
				sensor_res.width, sensor_res.height);

	/* set crop information */
	struct video_channel_attr obj_set_attr = {0};
	struct video_channel_attr *attr  = &obj_set_attr;

	attr->res[VIDEO_CHN_MAIN].width = FTEST_CH1_WIDTH;
	attr->res[VIDEO_CHN_MAIN].height = FTEST_CH1_HEIGHT;

	attr->res[VIDEO_CHN_SUB].width = FTEST_CH2_WIDTH;
	attr->res[VIDEO_CHN_SUB].height = FTEST_CH2_HEIGHT;
	attr->crop.left = 0;
	attr->crop.top = 0;
	attr->crop.width = sensor_res.width;
	attr->crop.height = sensor_res.height;

	if (ak_vi_set_channel_attr(handle, attr)) {
		ak_print_error_ex("ak_vi_set_channel_attr failed!\n");
	} else {
		ak_print_normal("ak_vi_set_channel_attr success!\n");
	}

	struct video_channel_attr obj_get_attr = {0};
	struct video_channel_attr *cur_attr = &obj_get_attr;

	if (ak_vi_get_channel_attr(handle, cur_attr)) {
		ak_print_error_ex("ak_vi_get_channel_attr failed!\n");
	}

	ak_print_normal("ak_vi_get_channel_attr =%d!\n",cur_attr->res[VIDEO_CHN_MAIN].width);
	/* set fps */
	ak_vi_set_fps(handle, 25);

	/* get fps */
	int fps = 0;
	fps = ak_vi_get_fps(handle);
	ak_print_normal("fps = %d\n",fps);

	return handle;
}

static void *init_video_encode(void)
{
	struct encode_param *param = (struct encode_param *)calloc(1,
			sizeof(struct encode_param));


	param->width = FTEST_CH1_WIDTH;
	param->height = FTEST_CH1_HEIGHT;
	param->minqp = 20;
	param->maxqp = 51;
	param->fps = 25;
	param->goplen = param->fps * 2;
	param->bps = 1500; 
	param->profile = PROFILE_MAIN;
	if (param->width <=640)
	    param->use_chn = ENCODE_SUB_CHN ; 
	else
    	param->use_chn = ENCODE_MAIN_CHN;
	param->enc_grp = ENCODE_RECORD;
	param->br_mode = BR_MODE_CBR;
	param->enc_out_type = MJPEG_ENC_TYPE;
	void * encode_video = ak_venc_open(param);
	ak_print_normal("YMX : free 2: size = %d\n", sizeof(struct encode_param));
	free(param);
	return encode_video;
}

int check_uvc_send_status(void)
{
    unsigned long t1, t2, offset;

    t1 = get_tick_count();
    while(ak_drv_uvc_wait_stream())
    {
        if(s_break)
            return -1;

        t2 = get_tick_count();
        offset = (t2 >= t1) ? (t2- t1) : (t2 + 0xFFFFFFFF - t1);
        if(offset > 500){
            printf("[uvc timeout: %d]\n", offset);
            return -1;
        }
    }

    return 0;
}

static void handle_mjpeg_stream(void *vi_handle, void *enc_handle)
{
    int ret, i = 0;
	void *stream_handle;
    struct video_stream video_stream = {0};
    struct uvc_stream uvc_stream = {0};

	stream_handle = ak_venc_request_stream(vi_handle, enc_handle);
	if (stream_handle == NULL) {
		ak_print_error_ex("request stream failed\n");
		return;
	}

	while(1)
	{
		ret = ak_venc_get_stream(stream_handle, &video_stream);
		if (ret != 0) {
			ak_sleep_ms(3);
			continue;
		}

        if(check_uvc_send_status() < 0)
        {
            ak_venc_release_stream(stream_handle, &video_stream);
            break;
		}
		
		uvc_stream.data = video_stream.data;
		uvc_stream.len = video_stream.len;

		ak_drv_uvc_send_stream(&uvc_stream);
        
		i++;
		if(i == 1)
			if(ak_vpss_effect_set(vi_handle,VPSS_POWER_HZ,50))
				ak_print_error("set hz error\n");
			else
				ak_print_normal("set hz ok\n");
		//ak_print_normal("[%d]get stream, size: %d, ts=%d\n",
		//	 i, video_stream.len, video_stream.ts);
            
		ret = ak_venc_release_stream(stream_handle, &video_stream);
		if (ret != 0) {
			ak_print_error_ex("release stream failed\n");
        }
        

        if(g_iVideoTestUvcEnable == 0)
        {
            ak_print_normal("Exit UVC test Mjpeg....\r\n");
            break;
        }
	}
	
	ak_print_normal_ex("cancel stream ...\n");
	ak_venc_cancel_stream(stream_handle);
}

static void chang_attr(void *vi_handle)
{
	struct video_channel_attr attr;
	ak_vi_get_channel_attr(vi_handle, &attr);
	attr.res[0].width= 640;
	attr.res[0].height = 480;
	int ret = ak_vi_change_channel_attr(vi_handle, &attr);
	if(ret == 0)
		ak_print_notice("ymx: %s.success.\n",__func__);
	else
		ak_print_notice("ymx: %s.fail.\n",__func__);
}
static void chang_attr2(void *vi_handle)
{
	struct video_channel_attr attr;
	ak_vi_get_channel_attr(vi_handle, &attr);
	attr.res[0].width= 1280;
	attr.res[0].height = 720;
	int ret = ak_vi_change_channel_attr(vi_handle, &attr);
	if(ret == 0)
		ak_print_notice("ymx: %s.success.\n",__func__);
	else
		ak_print_notice("ymx: %s.fail.\n",__func__);
}

static void handle_yuv_stream(void *vi_handle)
{
	int i=0;
    int ret;
    struct uvc_stream stream;
    struct video_input_frame frame;

	while(1)
	{
		ret = ak_vi_get_frame(vi_handle, &frame);
		if (ret != 0) {
			ak_print_error_ex("get frame fail\n");
			continue;
		}
		ak_print_normal("get frame[%d] ok\n",i++);
			
		stream.data = frame.vi_frame[VIDEO_CHN_MAIN].data;
		stream.len = frame.vi_frame[VIDEO_CHN_MAIN].len;

        if(check_uvc_send_status() < 0)
        {
            ak_vi_release_frame(vi_handle, &frame);
            break;
        }
        
		ak_drv_uvc_send_stream(&stream);

		ak_vi_release_frame(vi_handle, &frame);
#if 1
		if(i == 30)
		{
			chang_attr(vi_handle);
		}

		if(i == 60)
			chang_attr2(vi_handle);
#endif

        if(g_iVideoTestUvcEnable == 0)
        {
            ak_print_normal("Exit UVC test UYV....\r\n");
            break;
        }

	}
}

// static void cmd_uvc_demo(int argc, char **args)
// void ftest_cmd_uvc_test(int argc)
void* ftest_cmd_uvc_test(void *arg)
{
	void *vi_handle = NULL, *venc_handle = NULL;
	int i;
	char *main_addr = "ISPCFG";
    enum stream_type uvc_type; 

    uvc_type = STREAM_MJPEG;
    s_break = false;

    //register signal
    cmd_signal(CMD_SIGTERM, cmd_uvc_signal);

	//sensor init
	ak_drv_ir_init();
	ak_drv_ir_set_ircut(IRCUT_STATUS_DAY);
	ak_print_normal("set ircut day\n");

	if (ak_vi_match_sensor(main_addr)) {
		ak_print_error_ex("##########$$  match sensor main_addr failed!\n");

		return;
	} else {
		ak_print_normal("##########$$  ak_vi_match_sensor success!\n");
	}

    //vi venc init
	vi_handle = init_video_in();
    if(NULL == vi_handle)
        goto EXIT;
    
	if(uvc_type == STREAM_MJPEG)
	{
        venc_handle = init_video_encode();
        if(NULL == venc_handle)
            goto EXIT;
    }
	/* start isp tool service */
	// ak_its_start();

	ak_vi_capture_on(vi_handle);
	
    //uvc start
	ak_drv_uvc_start(FTEST_CH1_WIDTH, FTEST_CH1_HEIGHT, uvc_type);

    //wait pc open
    while (ak_drv_uvc_wait_stream() && !s_break)
    {
        if(g_iVideoTestUvcEnable == 0)
        {
            ak_print_normal("PC doesn't open stream, receive command exit!");
            goto EXIT;
        }
    }

	/*******************thread works start *****************************/
    if(uvc_type == STREAM_MJPEG)
    {
        handle_mjpeg_stream(vi_handle, venc_handle);
    }
    else
    {
        handle_yuv_stream(vi_handle);
    }
	/*******************thread work send *****************************/

	ak_drv_uvc_stop();

	ak_vi_capture_off(vi_handle);
EXIT:
	/* stop isp tool service */
	// ak_its_stop();
	ak_venc_close(venc_handle);
	ak_vi_close(vi_handle);
}

/*---------------------------------------------------------------------------*/
/*
*******************************************************************************
*                           END OF FILES                                      *
*******************************************************************************
*/
