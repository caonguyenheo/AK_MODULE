/**
  ******************************************************************************
  * File Name          : ak_video.c
  * Description        : This file contain functions for ak_video
  ******************************************************************************
  */

/*---------------------------------------------------------------------------*/
/*                           INCLUDED FILES                                  */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "ak_video.h"
#include "ak_apps_config.h"
#include "parse_frame_h264.h"
#include "debug_latency.h"
#include "spi_transfer.h"

#include "packet.h"
#include "ak_drv_ircut.h"
#include "dev_drv.h"
#include "dev_info.h"

#ifdef CFG_DEBUG_VIDEO_WITH_SDCARD
#include "arch_rtc.h"
#endif

#include "ringbuffer.h"
#include "packet_resend.h"
#include "list.h"

#include "doorbell_config.h"
#include "ak_vpss.h"

#include "idle.h"
#include "flv_engine.h"


#ifdef CFG_USE_SCHEDULER
#include "transfer_scheduler.h"
#endif 

#include "sdcard.h"


int g_iFirstFrameOK = 0;


extern int g_iCdsValue;
extern int g_ntp_updated;
extern int g_SpiCmdSnapshot;
int g_sd_update = 0;
extern ak_sem_t  sem_SpiCmdSnapShot;

static char acDebugH264[1024];


extern int set_play_init(char cCmdType);
/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/
#define SAVE_PATH        "a:"
#define FILE_NAME        "1212500_video.h264"

#define CAM_FIRST_PATH       "ISPCFG"
#define CAM_BACK_PATH        "ISPCFG"

#define FIRST_PATH       "ISPCFG"
#define BACK_PATH        "ISPCFG"


extern int g_iP2pSdStreamStart;

/* global variable store directory name of clip and snapshot */
char g_acDirName_Snapshot[256];
char g_acDirName_Clip[256];

DOORBELL_SNAPSHOT_DATA_t g_sDataSnapshot4Uploader;
char g_acSnapshotFileName[256];
char g_acClipFileName[256];

int g_iDoneClipRecord = 0;

char g_acDirName[256];
char g_acVideoMacAddress[13];
char g_acVideoClip_TimeLocal[256];
int g_iDoneSnapshot = SNAPSHOT_STATE_INIT; // 0;
int g_isDoneUpload = UPLOADER_STATE_INIT; // 0;

static int g_RecordDone = RECORD_STATE_INIT;


int  g_iRecordVideoFrameCount = 15*15;

static int enc_count;   //encode this number frames
/*global flag for video reques*/
static int video_stop_flag = 0; 
static int sys_idle_flag = 0;

static int g_channel_video = 0;

//TIME_DEBUG_UPLOAD_t g_sTimeDebugUpload = {0,0,0,0,0};



struct  video_param
{
    unsigned char  resolution;
    unsigned char  fps;
    unsigned short kbps;
};

struct  Venc_Demo_Para
{
    int width;
    int height;
    int fps;
    int kbps;
    int gop;
};

static struct video_param g_video_para = {1, 25, 500};

#if 0
static struct Venc_Demo_Para g_rec_para = 
    {CFG_DISPLAY_SRC_WIDTH_VGA, \
    CFG_DISPLAY_SRC_HEIGHT_VGA, \
    CFG_VIDEO_FRAME_PER_SECOND, \
    CFG_BITRATE_KBPS, \
    CFG_VIDEO_GOP};
#else
static struct Venc_Demo_Para g_rec_para = 
    {CFG_DISPLAY_SRC_WIDTH_MAX, \
    CFG_DISPLAY_SRC_HEIGHT_MAX, \
    CFG_VIDEO_FRAME_PER_SECOND, \
    CFG_BITRATE_KBPS, \
    CFG_VIDEO_GOP};

#endif


#ifdef CFG_RESEND_VIDEO
extern  RingBuffer          *pRbRecvAck;
void *resend_handle = NULL;
#endif

int g_CfgVideoDone = 0;

unsigned long g_video_timestamp = 0;
/* global variable for send SPI */
// static unsigned char g_aucBufferH264[CFG_MAX_FRAME_LEN *2];
// static unsigned char g_aucBufferSendSpi[CFG_MAX_FRAME_LEN *2];

void *take_snapshot_task(void *arg);

/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/
/* Record stream to SDcard to prove system capture the first picture after 
    300ms */
// #ifdef CFG_DEBUG_VIDEO_WITH_SDCARD

static RingBuffer *g_pRingbuf;
static int g_iCountFrame = 50;

static int db300_init_ringbuf(void)
{
    g_pRingbuf = RingBuffer_create(500*1024);   // 2MB
    if(g_pRingbuf == NULL)
    {
        return -1;
    }
    return 0;
}
static int db300_deinit_ringbuf(void)
{
    if(g_pRingbuf != NULL)
    {
        RingBuffer_destroy(g_pRingbuf);
    }
    return 0;
}

static int db300_backup_stream(char *pData, int length)
{
    int ret = 0;
    if(g_pRingbuf && (g_iCountFrame > 0))
    {
        if(pData == NULL)
        {
            printf("Stream NULL!\n");
            return -1;
        }
        else
        {
            ret = RingBuffer_write(g_pRingbuf, pData, length);
            if(ret != length)
            {
                printf("Failed to backup stream!\n");
                ret = -2;
            }
            else
            {
                g_iCountFrame--;
                ret = 0;
            }
            return ret;
        }
    }
    else
    {
        return -1;
    }
}

static void db300_init_debug_sdcard(void)
{
    // ak_mount_fs(1, 0, "a:");
    if(sdcard_get_status() != E_SDCARD_STATUS_MOUNTED )
    {
        sdcard_mount();
    }
    else
    {
        ak_print_normal("SDcard was mounted! Ready!\r\n");
    }
}

static void db300_deinit_debug_sdcard(void)
{
    //ak_unmount_fs(1, 0, "a:");
}

static void *db300_open_file()
{
	char file[100] = {0};
    struct ak_date systime;
    
    // systime = rtc_get_systime();
    ak_get_localdate(&systime);
    sprintf(file, "a:/venc_%04d%02d%02d%02d%02d%02d.h264"
        ,systime.year, systime.month, systime.day, 
        systime.hour, systime.minute, systime.second);

    ak_print_normal("file name=%s\n",file);
    
	FILE *filp = NULL;
	filp = fopen(file, "w");
	if (!filp) {
		ak_print_error_ex("fopen error\n");
		return NULL;
	}

	return filp;
}

static void local_date_time_str_get(char* str, int max_size)
{
    struct ak_date systime;
    memset(str, 0, max_size);

    if(ak_get_localdate(&systime) != 0)
    {
        snprintf(str, max_size - 1, "%04d%02d%02d%02d%02d%02d", 2017, 1, 1, 0, 0, 0);
    }
    else
    {
        snprintf(str, max_size - 1, "%04d%02d%02d%02d%02d%02d",
                 systime.year, systime.month, systime.day,
                 systime.hour, systime.minute, systime.second);
        // g_acVideoClip_TimeLocal;
        memcpy(str, g_acVideoClip_TimeLocal, strlen(g_acVideoClip_TimeLocal));
    }
}

static void macaddress_str_get(char *str)
{
    int iRet = 0;
    char acUdid[SYS_MAX_LENGTH_KEY + 1];
    memset(acUdid, 0x00, SYS_MAX_LENGTH_KEY + 1);
    iRet = doorbell_cfg_get_udid(acUdid);
    if(iRet != 0)
    {
        ak_print_error_ex("Get udid failed!\r\n");
    }
    else
    {
        memcpy(str, acUdid + 6, 12);
        str[12]='\0';
    }
}

static void db300_close_file(FILE *filp)
{
	fclose(filp);
}

static void db300_save_stream(FILE *filp, unsigned char *data, int len)
{
	int ret = len;
	do {
		ret -= fwrite(data, 1, ret, filp);
	} while (ret > 0);
}

void *db300_ak_record_sd_task(void *arg)
{
    FILE *fp = NULL;
    int iAvailableData = 0;
    unsigned char *pucData = NULL;
    int ret = 0;

    ak_sleep_ms(1000);
    ak_sleep_ms(1000);
    ak_sleep_ms(1000);

    while(g_iCountFrame > 0){
        ak_sleep_ms(1000);
    }
    db300_init_debug_sdcard();
    ak_sleep_ms(1000);
    printf("Record SDCard Start!\n");
    pucData = (unsigned char *)malloc(1024);
    if(pucData == NULL)
    {
        printf("Cannot allocate buffer data!\n");
        goto exit;
    }
    // open file
    fp = db300_open_file();
    if(fp == NULL)
    {
        ak_print_error_ex("open file failed!");
        goto exit;	    
    }
    // save stream
    iAvailableData = RingBuffer_available_data(g_pRingbuf);
    printf("Available data: %d\n", iAvailableData);
    while(iAvailableData > 0)
    {
        if(iAvailableData >= 1024){
            ret = RingBuffer_read(g_pRingbuf, (char *)pucData, 1024);
        }
        else
        {
            ret = RingBuffer_read(g_pRingbuf, (char *)pucData, iAvailableData);
        }
        
        if(ret > 0)
        {
            db300_save_stream(fp, pucData, ret);
        }
        else
        {
            printf("Read ringbuf failed!\n");
        }
        iAvailableData = RingBuffer_available_data(g_pRingbuf);
        printf("Available data: %d\n", iAvailableData);
        if(iAvailableData <= 0)
        {
            printf("end of record sd!\n");
            break;
        }
        ak_sleep_ms(50);

    }
    // close file
    db300_close_file(fp);
exit:
    free(pucData);
    db300_deinit_debug_sdcard();
    db300_deinit_ringbuf();
    printf("%s Exit thread!!!\n", __func__);
    ak_thread_exit();
    return NULL;
}

// #endif

/*---------------------------------------------------------------------------*/
/* Refactory, clean code */
/*---------------------------------------------------------------------------*/

/* global variable */
static int g_video_start = 0;

/*
*   This function was been copy from ak_venc_demo.c
*/
static int  set_dayornight_cfg(void * vi_handle)
{
    int status;
    int fd;
    int ret;

	ret = ak_drv_ir_init();
	if (0 != ret) {
		ak_print_error_ex("ircut init fail\n");
		return -1;
	}
	
	status = ak_drv_ir_get_input_level();
	
	if(status < 0)
	{
		ak_print_error_ex("ak_drv_ir_get_input_level fail\n");
		return -1;
	}
	
    // hardcode for demo
	status = 0; 
    
    if (status == 0) {
        ak_print_notice("set day isp config and ircut!\n");

        ak_drv_ir_set_ircut(IRCUT_STATUS_DAY);
        ak_vi_switch_mode(vi_handle, VI_MODE_DAYTIME); 
    } else {
        ak_print_notice("set night isp config and ircut!\n");
        ak_vi_switch_mode(vi_handle, VI_MODE_NIGHTTIME); 
        ak_drv_ir_set_ircut(IRCUT_STATUS_NIGHT);
    }
    
    return 0;
}

/*
*   This function was been copy from ak_venc_demo.c
*/
unsigned short g_exposure=0;
void *ak_video_input_init(const char *first, const char *second)
{
    int iRet = 0;
    CFG_DOORBELL_INT_t CfgDoorbellInt;
    int i;
    
    iRet = doorbell_load_config(&CfgDoorbellInt);
    if(iRet == 0)
    {
        ak_print_normal("Load config OK! height: %d\n", CfgDoorbellInt.height);
        g_rec_para.height = CfgDoorbellInt.height;
        switch(g_rec_para.height)
        {
            case 960:
            case 720:
                g_rec_para.width = 1280;
                break;
            default:
                g_rec_para.height = 360;
                g_rec_para.width = 640;
                break;
        }
        g_rec_para.fps = CfgDoorbellInt.framerate;
        g_rec_para.kbps = CfgDoorbellInt.bitrate;
        g_rec_para.gop = CfgDoorbellInt.gop;
        ak_print_normal("h:%d w:%d %d fps, bitrate:%d, gop: %d\n", \
            g_rec_para.height, g_rec_para.width, g_rec_para.fps, g_rec_para.kbps, g_rec_para.gop);
    }
    ak_print_normal("CDS: CHECK CDS VALUE %d\n", g_iCdsValue);
    if (ak_vi_match_sensor(first) < 0)
    {
        ak_print_error_ex("match sensor failed\n");
        return NULL;
    }

	/* open device */
	void *handle = ak_vi_open(VIDEO_DEV0);
	if (handle == NULL) {
		ak_print_error_ex("vi open failed\n");
		return NULL;
	}

    if (0 != set_dayornight_cfg(handle))
    {
        ak_vi_close(handle);
        return NULL;
    }


	/* get camera resolution */
	struct video_resolution resolution = {0};
	if (ak_vi_get_sensor_resolution(handle, &resolution))
	{
		ak_print_error_ex("get sensor resolution failed\n");
        ak_vi_close(handle);
        return NULL;
	}	
    else
    {
		ak_print_normal("sensor resolution height:%d,width:%d.\n",resolution.height,resolution.width);
    }

    //judge resolution and user need
    if (g_rec_para.width > resolution.width || g_rec_para.height > resolution.height )
    {
        ak_print_error("sensor resolution is too smaller!\n"
                       "request video width=%d,height=%d;sensor width=%d,height=%d.\n"
                       ,g_rec_para.width,g_rec_para.height,resolution.width,resolution.height);
        ak_vi_close(handle);
        return NULL;
    }
    
	/* set crop information */
	struct video_channel_attr attr;

    //set default crop
	attr.crop.left = 0;
	attr.crop.top = 0;
	attr.crop.width = resolution.width;
	attr.crop.height = resolution.height;

    //set channel default pixel
	attr.res[VIDEO_CHN_MAIN].width = resolution.width;
	attr.res[VIDEO_CHN_MAIN].height = resolution.height;
	attr.res[VIDEO_CHN_SUB].width = 640;
	attr.res[VIDEO_CHN_SUB].height= 360;

    //change pixerl by user need
    if (g_rec_para.width >640 ) //use  main channel
    {
    	attr.res[VIDEO_CHN_MAIN].width = g_rec_para.width;
    	attr.res[VIDEO_CHN_MAIN].height = g_rec_para.height;

        g_channel_video = VIDEO_CHN_MAIN;
        ak_print_error("---H720p---\r\n");    
    }
    else  //use sub channel
    {
    	attr.res[VIDEO_CHN_SUB].width  = g_rec_para.width;
    	attr.res[VIDEO_CHN_SUB].height = g_rec_para.height;

        //change vga height if sensor height is 960 or 480
    	if (g_rec_para.height==360 &&  
    	    (resolution.height == 960 || resolution.height==480))
        {
            printf("---H360---\r\n");    	    
        	attr.res[VIDEO_CHN_SUB].height = 360; //480;
            g_rec_para.height = 360;//480;
        }

        g_channel_video = VIDEO_CHN_SUB;        
    }
	
    if (ak_vi_set_channel_attr(handle, &attr))
    {
		ak_print_error_ex("set channel attribute failed\n");
    }
	/* get crop */
	struct video_channel_attr cur_attr ;
    if (ak_vi_get_channel_attr(handle, &cur_attr))
    {
		ak_print_normal("ak_vi_get_channel_attr failed!\n");
    }
    else
	{
        ak_print_normal("channel crop:left=%d,top=%d, width=%d,height=%d\n"
            ,cur_attr.crop.left,cur_attr.crop.top,cur_attr.crop.width,cur_attr.crop.height);
        ak_print_normal("channel pixel:main width=%d,height=%d;sub width=%d,height=%d\n"
            ,attr.res[VIDEO_CHN_MAIN].width,attr.res[VIDEO_CHN_MAIN].height
            ,attr.res[VIDEO_CHN_SUB].width,attr.res[VIDEO_CHN_SUB].height);
    }        
    i=0;
    while((g_exposure==0)&&(i<5)){
        ak_sleep_ms(100);
        i++;
    }
    if (g_exposure==0){
        g_exposure = 500;
        ak_print_error("No CDS given, set to default %d\n",g_exposure);
    } 
    /*
    if(g_iCdsValue)
    {
        //g_exposure = g_iCdsValue;
        ak_print_normal("CDS: SET CDS VALUE %d\n", g_iCdsValue);
    }
    else
    {
        ak_print_normal("CDS not set!\n");
    }*/
    cam_set_sensor_ae_info(&g_exposure);
    ak_print_normal(">>---> Set sensor ae info %p-%d -> fix black image!\n", &g_exposure, g_exposure);

	/* don't set camera fps in demo */
    // ak_vi_set_fps(handle, 25);
	ak_print_normal("capture fps: %d\n", ak_vi_get_fps(handle));
	return handle;
}

/*
*   This function was been copy from ak_venc_demo.c
*/
void *ak_video_encode_init(void)
{
	int fps = 0;
	int gop = 0;
	int bitrate = 0;
	struct encode_param *param = (struct encode_param *)calloc(1,
			sizeof(struct encode_param));

    param->width = g_rec_para.width;
    param->height = g_rec_para.height;
    param->fps = g_rec_para.fps;
    param->goplen = (g_rec_para.fps); //12
    param->bps = g_rec_para.kbps ; 
    param->minqp = 30;
    param->maxqp = 35;//51;
    param->profile = PROFILE_MAIN;
    if (param->width <=640)
        param->use_chn = ENCODE_SUB_CHN ; 
    else
        param->use_chn = ENCODE_MAIN_CHN;
    param->enc_grp = ENCODE_RECORD;
    param->br_mode = BR_MODE_CBR;
    param->enc_out_type = H264_ENC_TYPE;

    void * encode_video = ak_venc_open(param);
    ak_print_normal("YMX : free 2: size = %d\n", sizeof(struct encode_param));
	memset(acDebugH264, 0x00, 1024);
	bitrate = ak_venc_get_kbps(encode_video);
	gop = ak_venc_get_gop_len(encode_video);
	fps = ak_venc_get_fps(encode_video);
	sprintf(acDebugH264, "H%d W:%d, %d fps, gop %d, %d kbps\n", param->height, param->width, fps, gop, bitrate);
	free(param);
	return encode_video;
}

unsigned long video_get_timestamp(void)
{
    return g_video_timestamp;
}

/*
* Brief: Encypt data and send to spi queue
* Param:
* Return:
* Author: Tien Trung
* Date: 13-June-2017
*/
static int previous_frame=-1;
static int current_gop_bytecount=0;
static int first_iframe_received=0;
static int g_measure_video_bitrate = 0;
int g_bitrateadaptive_status = 1;
static int g_set_video_bitrate = 0;
static gop_count = 0;
int g_total_frame_count=0;
static int video_encrypt_send_spi_queue(struct video_stream *pstream, 
                                            unsigned char *pH264,
                                            unsigned char *pSpi)
{
    int iRet = 0;
    // variable for create packet
    char cPacketType = 0;
    int iPacketCount = 0;
    int iBufferSendSpiLen = 0;
    // variable for send loop
    int iSendSize = 0;
    static int speed = 0;
#ifdef CFG_RESEND_VIDEO
    static int num_of_resend = 0;
    // void *resend_handle = NULL;
    char acRecvAck[1024] = {0x00, };
    char *pSend = NULL;

    int iPacketResendRate = 0;
    int iPacketBackup = 0;
    int iPacketTimeout = 0;
    int iAvailableBk = 0;

#endif

    if(pstream == NULL || pH264 == NULL || pSpi == NULL)
    {
        ak_print_error_ex("%s stream null\n", __func__);
        return -1;
    }
    
    /* copy data */
    // memcpy(g_aucBufferH264, pstream->data, pstream->len);
    memcpy(pH264, pstream->data, pstream->len);
    cPacketType = (pstream->frame_type == 1)?10:12;
    g_total_frame_count++;
    
    int new_gop=0;
    // This section compute video bitrate
    if (cPacketType==10)
        first_iframe_received |= 1;
    if (first_iframe_received){ // Start counting from first Iframe
        if ((cPacketType==10) && (previous_frame!=10)){ // Frame type change, time to compute bitrate, reset byte count
            gop_count++;
            g_measure_video_bitrate = current_gop_bytecount*8/1000;
            if(eSysModeGet() != E_SYS_MODE_FTEST){
            ak_print_normal("Bitrate %d, packet count %d\r\n", g_measure_video_bitrate, current_gop_bytecount/999);
            }
            current_gop_bytecount = pstream->len;
            new_gop = 1;
        }else
            current_gop_bytecount += pstream->len;
        previous_frame = cPacketType;
    }
    if (g_bitrateadaptive_status){
        int temp=0;
        if (g_set_video_bitrate==0){
            g_set_video_bitrate = BITRATE_SET_MAX_KBPS;
            temp = 1;
        }
        if (((gop_count%3)==2) && new_gop){
            if (g_measure_video_bitrate>BITRATE_TRUE_MAX_KBPS){
                if (g_set_video_bitrate>BITRATE_SET_MIN_KBPS){
                    g_set_video_bitrate-=BITRATE_CHANGE_STEP_KBPS;
                    temp = 1;
                }
            }
            // Low cap
            if (g_set_video_bitrate<BITRATE_SET_MIN_KBPS)
                g_set_video_bitrate = BITRATE_SET_MIN_KBPS;
        }
        if (temp) {
            if ((g_venc_handle_tinkle!=NULL) && (ak_venc_set_rc(g_venc_handle_tinkle, g_set_video_bitrate)==0))
                ak_print_normal_ex("Auto Set bitrate success: %d\n", g_set_video_bitrate);
            else
                ak_print_error_ex("Auto Set bitrate %d, handler %p: got issue\n", g_set_video_bitrate, g_venc_handle_tinkle);        
        }
    }
    
    //
    g_video_timestamp = pstream->ts;

    /* Feed timestamp to packet */
    packet_set_video_timestamp((unsigned int)(pstream->ts));

    packet_lock_mutex();
    iRet = packet_create(cPacketType, &iPacketCount, \
                            (char *)pH264, pstream->len, \
                            (char *)pSpi, &iBufferSendSpiLen);
    packet_unlock_mutex();
    if(iRet != 0)
    {
        ak_print_error_ex("%s packet_create failed!\n", __func__);
    }
    else
    {
        iSendSize = 0;
        while(iSendSize < iBufferSendSpiLen)
        {
        #ifdef CFG_RESEND_VIDEO
            pSend = (char *)(pSpi + iSendSize);
        #endif
            iRet = send_spi_data((char *)(pSpi + iSendSize), \
                                                    MAX_SPI_PACKET_LEN);
            if(iRet != MAX_SPI_PACKET_LEN)
            {
                iRet = -2;
                // ak_print_error_ex("Video send spi failed!\n");
                break;
            }
            else
            {
                iSendSize += MAX_SPI_PACKET_LEN;
                speed++;
                // printf("%d ", iSendSize);
            #ifdef CFG_RESEND_VIDEO
                #ifndef CFG_DISABLE_RESEND
                if(pstream->frame_type == 1)
                {
                    iRet = packet_resend_push(resend_handle, (char *)pSend, MAX_SPI_PACKET_LEN);
                }
                #endif
            #endif
            }
        }

	#ifdef CFG_RESEND_VIDEO // because resendvideo share spi buffer so we need a define for it.
        
        /* Process packet ACK */
        iRet = RingBuffer_read(pRbRecvAck, acRecvAck, 1024);
        #ifndef CFG_DISABLE_RESEND
        if(iRet == 1024)
        {
            char *pAck = &acRecvAck[3];
            int length = (acRecvAck[1]&0xFF) << 8;
            length |= (acRecvAck[2]&0xFF) << 0;
            // printf("ACK len: hex %02x %02x dec %d\n", acRecvAck[1], acRecvAck[2], length);
            // process it
            iRet = packet_resend_process_ack(resend_handle, pAck, length);
        }
        /* Process timeout */
        packet_resend_process_timeout(resend_handle);
        #endif
    #endif
        if(pstream->frame_type == 1)
        {
            #ifndef UPLOADER_DEBUG
            // printf("Video send: %d packets\n", speed);
            #endif
            
		#ifdef CFG_RESEND_VIDEO
            #ifndef CFG_DISABLE_RESEND
            iPacketResendRate = packet_resend_get_resendrate();
            iPacketTimeout = packet_resend_get_numbertimeout();
            iPacketBackup = packet_resend_get_numberbackup();
            iAvailableBk = packet_resend_get_numberavailbalebackup();
            printf("V_sent:%d pps resend:%d T.O:%d ", speed, iPacketResendRate, iPacketTimeout);
            #ifndef UPLOADER_DEBUG
                // ak_print_error("TotalBk:%d, resend:%d, T.O:%d, availableBK:%d\n", iPacketBackup, iPacketResendRate, iPacketTimeout, iAvailableBk);
            #endif
            #endif
        #else
            // printf("V_sent:%d pps ", speed);
        #endif
			speed = 0;
        }
    }
    return iRet;
}

/* Porting from ak_photo_demo.c */
/**
 * @brief   initialize the video encoding handle
 *
 * @param   no
 * @return  the video encoding handle
 */
static void *video_encode_jpeg_init(void)
{
	struct encode_param param ;
    memset(&param, 0 , sizeof(param));
	/* Trung decrease resolution snapshot 360P */
	param.width = 640; //g_rec_para.width;
	param.height = 360; //g_rec_para.height;
	param.enc_out_type = MJPEG_ENC_TYPE;
    param.enc_grp = ENCODE_PICTURE;
    if(param.width <= 640){
        printf("Sub channel for recoring snapshot");
        param.use_chn = ENCODE_SUB_CHN;
    }
    else
    {
        printf("Main channel for recoring snapshot");
        param.use_chn = ENCODE_MAIN_CHN;
    }
	void * encode_video = ak_venc_open(&param);
	return encode_video;
}


/* set on flight */
extern int flip_update_bt, flip_update_ud;
void video_set_flip_mirror(void *vi_handle)
{
    int ret = 0, ret_flip = 0, ret_mirror = 0;
    int flip_enable = 0, mirror_enable = 0;
    int iGetCfgFlip = 0;
    int iGetCfgMirror = 0;

     // test flip
    ret = ak_vi_get_flip_mirror(vi_handle, &flip_enable, &mirror_enable);
    if(ret != 0)
    {
        ak_print_error_ex("Get flip mirror Failed!\n");
    }
    else
    {
        ak_print_normal("SENSOR get flip: %d mirror: %d OK!\r\n", flip_enable, mirror_enable);
        ret_flip = doorbell_cfg_get_flipupdown(&iGetCfgFlip);
        ret_mirror = doorbell_cfg_get_fliptopbottom(&iGetCfgMirror);
        if((ret_flip == 0) && (ret_mirror == 0))
        {
            if((flip_enable != iGetCfgFlip) || (mirror_enable != iGetCfgMirror))
            {
                ret = ak_vi_set_flip_mirror(vi_handle, iGetCfgFlip, iGetCfgMirror);
                if(ret != 0)
                {
                    ak_print_error_ex("Set flip mirror failed!\n");
                }
                else
                {
                    ak_print_normal("Set flip: %d mirror: %d OK!\r\n", flip_enable, mirror_enable);
                }
            }
            else
            {
                ak_print_normal("No need to config! flip: %d mirror: %d\n", flip_enable, mirror_enable);
            }
        }
        else
        {
            ak_print_error_ex("Get config failed! flip(%d) mirror(%d)\n", ret_flip, ret_mirror);
        }
    }
}


/*
* Global variable
*/
void *g_camera_handle = NULL;
void *g_venc_handle_tinkle = NULL;
int iFlagTestIframeEncode = 0;
/*
*   This fucntion will replace ak_video_encode_task
*/
// merging branch 20170918_nguyen_cao_set_flicker_flip
extern int FlickerValue_update, ck;
void *ak_video_encode_task(void *arg)
{
    ak_pthread_t thread_jpeg;
    int timeout = 0;
    /* parameter for call API */
    int ret = 0;
    /* parameter for camera */
    void *vi_handle, *venc_handle, *stream_handle;
    // VIDEO_STREAM_SEND_INFO	video_send_buf;
    struct video_stream stream = {0};

    int iValueVpss = 0;
    int flip_enable = 0, mirror_enable = 0;

    /* debug first frame */
    char c_flag_first_frame = 1;
    /* array data for SPI and packetCreate */
    unsigned char *pucBufferH264 = NULL;
    unsigned char *pucBufferSendSpi = NULL;
    ENUM_SYS_MODE_t sys_get_mode;
    char cFlvClipName[128] = {0};
    char cLocalTimeStr[32] = {0};
    int  iVideoFrameCount = 15*15;

    //Tien Trung declare variable to measure time
    unsigned long ulLastTick = 0, ulCurrentTick = 0;
	int iCntData = 0;
    int iBitrateFtest = 0;
	float fAverageBitrate = 0;

#ifdef CFG_DEBUG_VIDEO_WITH_SDCARD
    // ak_pthread_t db300id;
    db300_init_ringbuf();
#endif
    // while(!g_iCdsValue){
    //     ak_sleep_ms(10);
    // }
    ak_print_normal("CDS: %d\n", g_iCdsValue);
    ak_print_normal("VI: init camera\n");
    /* Init camera */
    vi_handle = ak_video_input_init(FIRST_PATH, BACK_PATH);
	if (vi_handle == NULL) {
		ak_print_error_ex("video input init faild, exit\n");
        ak_thread_exit();
        return NULL;
	}
    //set flip and mirror
    video_set_flip_mirror(vi_handle);
    g_camera_handle = vi_handle;

    ak_print_normal("VI: venc init\n");
    /* Init encoder */
    venc_handle = ak_video_encode_init();
    g_venc_handle_tinkle = venc_handle;
    if (venc_handle == NULL) {
		ak_print_error_ex("video encode open failed!\n");
		goto exit_venc;
	}

    ak_print_normal("VI: init stream\n");

    // New SDK 51802 add new function
    ak_vi_capture_on(vi_handle);

    /* Init stream */
    stream_handle = ak_venc_request_stream(vi_handle, venc_handle);
	if (stream_handle == NULL) {
		ak_print_error_ex("request stream failed\n");
		goto exit_stream;
	}
    ak_print_normal("VI: Init OK\n");
    /*
    Issue low memory: Decrease len of buffer h264
    */
    pucBufferH264 = (unsigned char *)malloc(CFG_MAX_FRAME_LEN);
    if(pucBufferH264 == NULL)
    {
        ak_print_error_ex("malloc pucBufferH264 failed!\n");
		goto exit_stream;
    }
    /*
    Issue low memory: Decrease len of buffer wrapper spi
    */
    pucBufferSendSpi = (unsigned char *)malloc(CFG_MAX_FRAME_LEN);
    if(pucBufferSendSpi == NULL)
    {
        ak_print_error_ex("malloc pucBufferSendSpi failed!\n");
		goto exit_stream;
    }

/* RESEND VIDEO INIT */
#ifdef CFG_RESEND_VIDEO
    resend_handle = packet_resend_init();
    // LIST_HEAD(video_list_head);
    printf("1RESEND_VIDEO: pointer head: %p\n", resend_handle);
#endif

    /* We raise this flag to notify FMDrv can use I2C config FM */
    g_CfgVideoDone = 1; 
    g_video_start = 1;

// #ifndef DOORBELL_TINKLE820
#if (RECORDING_SDCARD == 1) 
    // init flv recorder
    FLVEngineInit();
#endif
// #endif

	ulLastTick = get_tick_count();

    while(g_video_start)
    {
        ret = ak_venc_get_stream(stream_handle, &stream);
        if (ret != 0) {
            ak_sleep_ms(VIDEO_TASK_NOFRAME);
            continue;
        }

		/* count data video */
		iCntData += stream.len;
        iBitrateFtest += stream.len;

        if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
        {
            if(flip_update_ud >= 0 && flip_update_ud <= 1)
            {
                video_set_flip_mirror(vi_handle);
                ak_print_normal("CALL set_flip_mirror UD DONE\r\n");
                flip_update_ud = -1;
            }
            if(flip_update_bt >= 0 && flip_update_bt <= 1)
            {
                video_set_flip_mirror(vi_handle);
                ak_print_normal("CALL set_flip_mirror BT DONE\r\n");
                flip_update_bt = -1;
            }
            if(ck == 1)
            {
                ak_vpss_effect_get(vi_handle, VPSS_POWER_HZ, &iValueVpss); //get value from system
                ak_print_normal("\nGET FLICKER: %d\r\n", iValueVpss);
                if(FlickerValue_update != iValueVpss) //check value FlickerValue_update receive from spicmd_process_flicker() different iValueVpss
                {
                   ak_vpss_effect_set(vi_handle, VPSS_POWER_HZ, FlickerValue_update); //set value for system
                   ak_print_normal("\nFlickerValue_update SET HZ DONE\r\n");
                }
                else
                {
                   ak_print_normal("\nFlickerValue_update Is OLD Has Set\r\n");
                }
                ck = 0;
            }

            if(c_flag_first_frame == 1)
            {
                ak_print_normal(">>>FIRST FRAME!\n");
                c_flag_first_frame = 0;

				//g_sTimeDebugUpload.first_i_frame = get_tick_count();
				
                ak_print_normal("time 3 %d \n", get_tick_count());
                g_iFirstFrameOK = 1;
				
				ak_print_error_ex("acDebugH264: %s\r\n", acDebugH264);
            }
        
            g_iRecordVideoFrameCount++;

        // #ifndef DOORBELL_TINKLE820
        #if (RECORDING_SDCARD == 1) 
            /* Feed data to FLV muxer before send over SPI */
            if(FLVEnginePushVideoData(stream.data, stream.len, stream.ts) != 0)
            {
                ak_print_normal("FLVEnginePushVideoData size %d, ts %lu fail\n", stream.len, stream.ts);
            }
            else
            {
                // ak_print_normal("FLVEnginePushVideoData size %d, ts %lu\n", stream.len, stream.ts);
            }
        #endif
            /* Call API encrypt and send data over spi */
            ret = video_encrypt_send_spi_queue(&stream, pucBufferH264, pucBufferSendSpi);
        
        }//if(g_iP2pSdStreamStart == 0)

        if(stream.frame_type == 1){
            spicmd_factory_test_send_bitrate(iBitrateFtest*8/1024);
            iBitrateFtest = 0;
        }

       	ulCurrentTick = get_tick_count();
		if((ulCurrentTick - ulLastTick) >= (VIDEO_MONITOR_BITRATE_INTERVAL*1000) )
		{
			//fAverageBitrate = (float)iCntData * 8 / VIDEO_MONITOR_BITRATE_INTERVAL / 1024;
			//fAverageBitrate = 10.3333;
			//ak_print_normal("Sum: %d, %5.2lf kbps\r\n", iCntData, fAverageBitrate);
			//ak_print_normal("Sum: %d Bytes, %d kbps\r\n", iCntData, iCntData*8/1024);
			ak_print_normal("Sum:%d bytes, %d kbps\r\n", iCntData/VIDEO_MONITOR_BITRATE_INTERVAL, iCntData*8/1024/VIDEO_MONITOR_BITRATE_INTERVAL);
			iCntData = 0;
			fAverageBitrate = 0;
			ulLastTick = ulCurrentTick;
		}

        ret = ak_venc_release_stream(stream_handle, &stream);
		if (ret != 0) {
			ak_print_error_ex("release stream failed\n");
		}
        ak_sleep_ms(VIDEO_TASK_DELAY);
    #ifdef CFG_DEBUG_VIDEO_WITH_SDCARD
        if(g_iCountFrame <= 0)
        {
            break;
        }
    #endif
      
        sys_get_mode = eSysModeGet();
        if(E_SYS_MODE_FWUPGRADE == sys_get_mode)
        {
            ak_print_normal("%s exit. Mode: %d\r\n", __func__, sys_get_mode);
            break;
        }
        if(ftest_uvc_get_state() != 0)
        {
            ak_print_error_ex("Receive command UVC, video will be exited...\n");
            break;
        }
    }
    ak_print_normal_ex("cancel stream ...\n");
	ak_venc_cancel_stream(stream_handle);
#ifdef CFG_RESEND_VIDEO
    packet_resend_deinit(resend_handle);
#endif
    g_video_start = 0;
exit_stream:

    /* New SDK 51802 add new function */
    ak_vi_capture_off(vi_handle);
    ak_venc_close(venc_handle); // close handle h264
exit_venc:
    ak_vi_close(vi_handle);
    ak_print_normal("Thread Video exit!\n");
	ak_thread_exit();
    return NULL;
}


/*---------------------------------------------------------------------------*/
/* TASK TAKE SNAPSHOT */
/*---------------------------------------------------------------------------*/

int video_get_system_info_groupname(void)
{
    int iRet = 0;
    char acUdid[SYS_MAX_LENGTH_KEY + 1];
    char acModelId[4];
    char acMacAddress[13];
    char acDirLocal[256];

    memset(acUdid, 0x00, SYS_MAX_LENGTH_KEY + 1);
    memset(acModelId, 0x00, 4);
    memset(acMacAddress, 0x00, 13);
    memset(acDirLocal, 0x00, 256);

    // get udid
    iRet = doorbell_cfg_get_udid(acUdid);
    if(iRet != 0)
    {
        ak_print_error_ex("Get udid failed!\r\n");
    }
    else
    {
        memcpy(acModelId, acUdid + 3, 3);
        ak_print_normal_ex("acModelId: %s\r\n", acModelId);
        memcpy(acMacAddress, acUdid + 6 + 6, 6); // the last 6 character
        ak_print_normal_ex("acMacAddress: %s\r\n", acMacAddress);
        sprintf(acDirLocal, "%s%s%s", SYS_DIR_NAME_PREFIX, acModelId, acMacAddress);
        printf("acDirLocal: %s\r\n", acDirLocal);

        memcpy(g_acDirName, acDirLocal, strlen(acDirLocal));
        /* generate directory name snapshot */
        sprintf(g_acDirName_Snapshot, "%s_%s", acDirLocal, SYS_DIR_NAME_SNAPSHOT_SUFFIX);
        /* generate directory name clip */
        sprintf(g_acDirName_Clip, "%s_%s", acDirLocal, SYS_DIR_NAME_CLIP_SUFFIX);

        memset(acMacAddress, 0x00, 13);
        memcpy(acMacAddress, acUdid + 6, 12);
        memcpy(g_acVideoMacAddress, acMacAddress, strlen(acMacAddress));
        printf("g_acVideoMacAddress: %s\r\n", g_acVideoMacAddress);
        printf("================================================\r\n");
    }
    return iRet;
}

/**
 * @brief   open a file to write a photo data
 *
 * @param   no
 * @return  the file handle
 */
void *open_file(void)
{
    char filename[256] = {0};
    char filepath[256] = {0};
    char dirname[256] = {0x00};
    struct ak_date systime;
    FILE * fDir;
    FILE *filp = NULL;

    memset(dirname, 0x00, sizeof(dirname));
    memset(filename, 0x00, sizeof(filename));
    memset(filepath, 0x00, sizeof(filepath));

    if(video_get_system_info_groupname() == 0)
    {
        // sprintf(dirname, "a:/%s", g_acDirName);
        sprintf(dirname, "a:/%s", g_acDirName_Snapshot);
    
        ak_get_localdate(&systime);
        ak_print_error_ex("acDirName=%s\n", g_acDirName);
        sprintf(filename, "%s_%02d_%04d%02d%02d%02d%02d%02d000",
            g_acVideoMacAddress, SYS_EVENT_CODE, systime.year, systime.month, systime.day, 
            systime.hour, systime.minute, systime.second);
        // sprintf(filepath, "%s/%s.jpg", g_acDirName, filename);
        sprintf(filepath, "a:/%s/%s.jpg", g_acDirName_Snapshot, filename);
        memcpy(g_acSnapshotFileName, filename, strlen(filename));

        sprintf(g_acVideoClip_TimeLocal, "%04d%02d%02d%02d%02d%02d",
                    systime.year, systime.month, systime.day, 
                    systime.hour, systime.minute, systime.second);
         
        ak_print_error_ex("g_acSnapshotFileName=%s\n", g_acSnapshotFileName);
        ak_print_error_ex("file_path=%s\n", filepath);

        /* folder name for snapshot */
    #if (SDCARD_ENABLE == 1)
        fDir = fopen(dirname,"r");
        if (NULL == fDir )
        {
            ak_print_normal("Snapshot dir don't exists!");
            if (0 != mkdir(dirname, 0))
            {
                return NULL;
            }
        }
        else
        {
            ak_print_normal("Snapshot dir exists!");
            fclose(fDir);
        }
    #endif

        /* folder name for clip */
        memset(dirname, 0x00, sizeof(dirname));
        sprintf(dirname, "a:/%s", g_acDirName_Clip);
    
    #if (SDCARD_ENABLE == 1)
        fDir = fopen(dirname,"r");
        if (NULL == fDir )
        {
            ak_print_normal("Clip dir don't exists!");
            if (0 != mkdir(dirname, 0))
            {
                return NULL;
            }
        }
        else
        {
            ak_print_normal("Clip dir exists!");
            fclose(fDir);
        }

        filp = fopen(filepath, "wb");
        if (!filp) {
            ak_print_error_ex("fopen error %s\r\n", filepath);
            return NULL;
        }
    #endif

    }
	return filp;
}


/**
 * @brief   close a file
 *
 * @param   filp[in] the file handle
 * @return  no
 */
static void close_file(FILE *filp)
{
	fclose(filp);
}

/**
 * @brief   save data to a file
 *
 * @param   filp[in] the file handle
 * @param   data[in] the data address to write
 * @param   len[in] the data size to write
 * @return  no
 */
static void save_stream(FILE *filp, unsigned char *data, int len)
{
	int ret = len;
	
	do {
		ret -= fwrite(data, 1, ret, filp);
	} while (ret > 0);
}


/* start recording flv with file name, duration */
static int system_start_recording(void)
{
    int iRet = 0;
    char cLocalTimeStr[32] = {0};
    char cFlvClipName[128] = {0};

    while(StreamMgrRecordingStatus() != 0)
    {
        ak_sleep_ms(100);
    }
    memset(cLocalTimeStr, 0, sizeof(cLocalTimeStr));
    /*<Macaddress>_<motion code>_<timestamp - up to seconds >_<clip_id>_<record_duration>.flv*/
    local_date_time_str_get(cLocalTimeStr, sizeof(cLocalTimeStr));
    memset(cFlvClipName, 0, sizeof(cFlvClipName));
    /* _last becasue we have only 1 clip */
    // snprintf(cFlvClipName, sizeof(cFlvClipName) - 1, "a:/FFFFFFFFFFFF_01_%s000_0001_15_last.flv", cLocalTimeStr);
    if(strlen(g_acVideoMacAddress) == 0)
    {
        macaddress_str_get(g_acVideoMacAddress);
    }
    snprintf(cFlvClipName, sizeof(cFlvClipName) - 1, "a:/%s/%s_%02d_%s000_0001_15_last.flv", g_acDirName_Clip, g_acVideoMacAddress, SYS_EVENT_CODE, cLocalTimeStr);
    memset(g_acClipFileName, 0x00, sizeof(g_acClipFileName));
    // memcpy(g_acClipFileName, cFlvClipName, sizeof(cFlvClipName));
    snprintf(g_acClipFileName, sizeof(g_acClipFileName) - 1, "%s_%02d_%s000_0001_15_last", g_acVideoMacAddress, SYS_EVENT_CODE, cLocalTimeStr);
    ak_print_normal("Recording %s at time %d\n", cFlvClipName, get_tick_count());
    ak_print_normal("Start recording return %d\n", StreamMgrRecordingStart(cFlvClipName, SYS_RECORD_DURATION_TIME));
    g_RecordDone = RECORD_STATE_RUNNING;
    g_iRecordVideoFrameCount = 0;
    g_iDoneClipRecord++;
    return iRet;
}

#ifdef SYS_ENCODE_JPEG

static int video_take_snapshot_one_frame(void *pHandler, void *jpeg_handle, 
    struct video_stream *pStream, int iNumSkip)
{
    int iRet = 0;
    int iSkipFrame = iNumSkip;
    struct video_input_frame input_frame;
    if(pHandler == NULL || jpeg_handle == NULL || pStream == NULL)
    {
        return -1;
    }

    // Default skip 1 frame
    if(iSkipFrame < 0)
    {
        iSkipFrame = 1;
    }

    // Skip frames after bootup
    while(g_total_frame_count<iSkipFrame){
        ak_print_normal("Skip %d frame\n",g_total_frame_count);
        ak_sleep_ms(50);
    }

    for(;;)
    {
        iRet = ak_vi_get_frame(pHandler, &input_frame);
        if (iRet != 0) {
            ak_print_normal("Fail to get frame\r\n");
            ak_sleep_ms(50);
            continue;
        }

        /*
        else
        {
            iSkipFrame--;
            if(iSkipFrame > 0)
            {
                //ak_print_normal("Snapshot:Skip frame! len: %d\r\n", input_frame.vi_frame[g_channel_video].len);
                
				ak_print_normal("Snapshot:Skip frame! len: %d\r\n", input_frame.vi_frame[VIDEO_CHN_SUB].len);
				ak_sleep_ms(10);
                ak_vi_release_frame(pHandler, &input_frame);
                continue;
            }
        }*/
        // iRet = ak_venc_send_frame(jpeg_handle , \
        //     input_frame.vi_frame[g_channel_video].data, \
        //     input_frame.vi_frame[g_channel_video].len, pStream);  

		iRet = ak_venc_send_frame(jpeg_handle , \
            input_frame.vi_frame[VIDEO_CHN_SUB].data, \
            input_frame.vi_frame[VIDEO_CHN_SUB].len, pStream);     
        
        ak_vi_release_frame(pHandler, &input_frame);
        if (0 != iRet)
        {
            ak_print_error_ex("encode JPEG error!\n");
        }
        break;
    }
    return iRet;
}

int check_status_record(void)
{
    return g_RecordDone;
}

/*
Any update in this function
CC3200 send command play dingdong.
AK play ding dong and take snapshot
When AK take snapshot, CC continue send command dingdong.
AK must continue do it, don't take snapshot again.
*/
/*
Logic of snapshot and uploader
Snapshot state: Init - On Going - Done
Uploader state: Init - On Going - Done
--------------------------------------
|Snapshot    | Uploader  | Result    |
--------------------------------------
| Init       | Init      |  OK       |
| On going   | Init      |  NG       |
| Done       | Init      |  NG       |
| Done       | On going  |  NG       |
| Done       | Done      |  OK       |
--------------------------------------
| On going   | Init      |  NG       |
--------------------------------------
*/

void *ak_video_take_snapshot_task_2(void *arg)
{
    int iSkipFrame = 3;
    void *jpeg_handle;
    ENUM_SYS_MODE_t sys_get_mode;
    int timeout = 0;
    int iRet = 0;
    int iSnapshotRunning = 0;
    struct video_stream stream = {0};
    FILE *fp = NULL;
    int iSnapshotAllow = 0;
    int iCountTORecord = 0;

#ifndef DOORBELL_TINKLE820
    /* Init sdcard */
    // ak_mount_fs(1, 0, "a:");
    iRet = sdcard_mount();
    if(iRet != 0)
    {
        ak_print_error_ex("1.Failed to initialize sdcard!(%d)\r\n", iRet);
        // return;
    }
#endif    
    // init encoder mjpeg
    jpeg_handle = video_encode_jpeg_init();
    if (jpeg_handle == NULL) {
		ak_print_error_ex("video jpeg_handle open failed!\n");
		goto exit;
    }

    //TODO: need to optimize len of jpeg
    g_sDataSnapshot4Uploader.data = (char *)malloc(CFG_MAX_FRAME_LEN);
    if(g_sDataSnapshot4Uploader.data == NULL)
    {
        ak_print_error_ex("allocate g_sDataSnapshot4Uploader failed!\n");
		goto exit;
    }
    
    // // wait data from video task
    while(g_video_start != 1)
    {
        ak_sleep_ms(50);
    }
    g_iDoneSnapshot = SNAPSHOT_STATE_INIT; // 0; // init value to pass while loop

    while(1)
    {
        //ak_print_normal("waiting for SPICMD start semaphore\n");
        //ak_thread_sem_wait(&sem_SpiCmdSnapShot);
        if (g_SpiCmdSnapshot!=1){
            ak_sleep_ms(100);
            // ak_print_normal("%s Waiting for ...\r\n", __func__);
	        continue;
        }
        ak_print_normal("%s recvcmd take snapshot state g_isDoneUpload:%d, g_SpiCmdSnapshot: %d (ts %u)\n", __func__, g_isDoneUpload, g_SpiCmdSnapshot, get_tick_count());
        
    #if (RECORDING_SDCARD == 1)  // we dont record flv for model 003  
        // if(check_status_record() != RECORD_STATE_DONE)
        if(g_RecordDone == RECORD_STATE_RUNNING)
        {
            ak_sleep_ms(100);
            iCountTORecord++;
            if(iCountTORecord >= 160){
                ak_print_error_ex("Time out recording!\r\n");
                // iCountTORecord = 0;
            }
            else
            {
                ak_print_normal("%s Waiting for recording...\r\n", __func__);
                continue;
            }
        }
        iCountTORecord = 0;
    #endif

        /* Debug state of snapshot and uploader */
        if((g_iDoneSnapshot == SNAPSHOT_STATE_INIT) && \
            (g_isDoneUpload == UPLOADER_STATE_INIT) )
        {
            ak_print_normal("OK: Snapshot and Uploader stayed init_state!\r\n");
            iSnapshotAllow = 1;
            g_SpiCmdSnapshot = 0; // avoid flow continue running
            g_iDoneSnapshot = SNAPSHOT_STATE_RUNNING; // set here to avoid in case of uploader not sync
        }
        else if((g_iDoneSnapshot == SNAPSHOT_STATE_WAIT) && \
            (g_isDoneUpload == UPLOADER_STATE_DONE) )
        {
            ak_print_normal("OK: Snapshot wait and Uploader stayed done_state!\r\n");
            iSnapshotAllow = 1;
            g_SpiCmdSnapshot = 0; // avoid flow continue running
            g_iDoneSnapshot = SNAPSHOT_STATE_RUNNING; // set here to avoid in case of uploader not sync
        }
        else
        {
            ak_print_error_ex("NG: Snapshot: %d, Uploader: %d\r\n", \
                        g_iDoneSnapshot, g_isDoneUpload);
            iSnapshotAllow = 0;
            g_SpiCmdSnapshot = 0; // avoid flow continue running
        }
        /* --------------------------------------------- */

        if(g_iP2pSdStreamStart == 0) // p2p sdcard is streaming
        {
            if(iSnapshotAllow != 1)       
            {
                ak_print_normal("Sw...\r\n");
                g_SpiCmdSnapshot = 0;
                continue;
            }
            // g_iDoneSnapshot = 0;
            g_iDoneSnapshot = SNAPSHOT_STATE_RUNNING;
            ak_print_normal("Let take snapshot. state g_isDoneUpload:%d g_iDoneSnapshot:%d (ts %u)\n", g_isDoneUpload, g_iDoneSnapshot, get_tick_count());
            // encode
            while(1)
            {
                ak_print_normal("start video_take_snapshot_one_frame\r\n");
                iRet = video_take_snapshot_one_frame(g_camera_handle, jpeg_handle, &stream, 2);
                if (0 != iRet)
                {
                    ak_print_error_ex("encode JPEG error!\n");
                }
                else
                {
                    ak_print_normal("%s $$$$ get stream, size: %d,  ts=%d\n", __func__, stream.len, stream.ts);
					//g_sTimeDebugUpload.take_snapshot = get_tick_count();
                    /*copy to structure and pass to uploader*/
                    if(g_sDataSnapshot4Uploader.data)
                    {
                        g_sDataSnapshot4Uploader.length = stream.len;
                        // copy data
                        memcpy(g_sDataSnapshot4Uploader.data, stream.data, stream.len);
                        printf("length snapshot: %u\r\n", g_sDataSnapshot4Uploader.length);
                    }
                    else
                    {
                        ak_print_error("Cannot allocate memory data snapshot4uploader!\r\n");
                    }
/* No longer need NTP because it's done in CC MQTT PUSH
                    while(g_ntp_updated == 0)
                    {
                        ak_sleep_ms(100);
                        timeout++;
                        if(timeout >= 20)
                        {
                            ak_print_normal("%s timeout\r\n", __func__);
                            goto exit;
                        }

                        sys_get_mode = eSysModeGet();
                        if(E_SYS_MODE_FTEST == sys_get_mode || \
                            E_SYS_MODE_FWUPGRADE == sys_get_mode)
                        {
                            ak_print_normal("%s exit. Mode: %d\r\n", __func__, sys_get_mode);
                            break;
                        }
                    }*/
                    // ak_sleep_ms(1000);

                    // g_iDoneSnapshot = 1; // Workaround for uploader
                    g_iDoneSnapshot = SNAPSHOT_STATE_DONE;
                    printf("SNAPSHOT_STATE_DONE: time (%u)\r\n", get_tick_count());

                    // NOTE: We use open file to create file name for snapshot.
                    //open a file to save photos

                    fp = open_file(); // To generate filename
                    ak_sleep_ms(50); // Relax for uploading thread
#if (SDCARD_ENABLE == 1)
                    if (fp == NULL)
                    {
                        ak_print_error_ex("open file failed!\r\n");
                        // break;  
                    }
                    else
                    {
                        //save a video frame     			 
                        save_stream(fp, stream.data, stream.len);
                        if (NULL !=stream.data )
                        {
                            free(stream.data);
                        }
                        close_file(fp);
                    }
#endif
                    printf("Save snapshot done: time (%u)\r\n", get_tick_count());
#if (RECORDING_SDCARD == 1)             
                    system_start_recording();
                    g_sd_update = CK_STATE_SD_RECORD_START ;
#endif
                    ak_print_normal("start send ak pwr\r\n");
                    ak_print_normal("Done take snapshot, save to sdcard. state g_isDoneUpload:%d g_iDoneSnapshot:%d (ts %u)\n", g_isDoneUpload, g_iDoneSnapshot, get_tick_count());
                    
                    break;
                } // if (0 != iRet)

                if(E_SYS_MODE_FTEST == sys_get_mode || \
                    E_SYS_MODE_FWUPGRADE == sys_get_mode)
                {
                    ak_print_normal("%s exit2. Mode: %d\r\n", __func__, sys_get_mode);
                    break;
                }
            } //while(1) //2
        }   
        // sys_get_mode = eSysModeGet();
        if(E_SYS_MODE_FTEST == sys_get_mode || \
            E_SYS_MODE_FWUPGRADE == sys_get_mode)
        {
            ak_print_normal_ex("Break 2 exit\r\n");
            break;
        }
        // iRet = sdcard_umount();
    }  // while (1)// 1
exit:
    iRet = sdcard_umount();
    if(jpeg_handle)
    {
        ak_venc_close(jpeg_handle); // close handle jpeg
    }
    ak_print_normal("Thread Take Snapshot exit!\n");
	ak_thread_exit();
    return NULL;
}
#endif


unsigned int snapshot_get_size_data(void)
{
    return g_sDataSnapshot4Uploader.length;
}

int snapshot_get_file_name(char *pName)
{
    int iRet = 0;
    // char file[100] = {0};
    // struct ak_date systime;

    if(pName == NULL)
    {
        ak_print_error_ex("Parameter is NULL!\r\n");
        iRet = -1;
    }
    else
    {
        if(strlen(g_acSnapshotFileName) == 0)
        {
            ak_print_error_ex("No NTP, so there is no filename!\r\n");
            iRet = -1;
        }
        else
        {
            memcpy(pName, g_acSnapshotFileName, strlen(g_acSnapshotFileName));
            ak_print_error_ex("File name: %s\r\n", pName);
        }
    }
    return iRet;
}

/*
* return 0 if clip ready and filename clip
*/
int clip_get_file_name(char *pName)
{
    int iRet = 0;
    if(pName == NULL)
    {
        ak_print_error_ex("Parameter is NULL!\r\n");
        iRet = -1;
    }
    else
    {
        if(g_iDoneClipRecord)
        {
            if(strlen(g_acClipFileName) == 0)
            {
                ak_print_error_ex("Clip file name didn't exist\r\n");
                iRet = -1;
            }
            else
            {
                memcpy(pName, g_acClipFileName, strlen(g_acClipFileName));
                ak_print_error_ex("ClipFilename: %s\r\n", pName);
            }
        }
        else
        {
            iRet = -1;
        }
    }
    return iRet;
}


int clip_filesize;
void thirdparty_record_done(int duration, char* filename, int filesize, int errorcode)
{
    g_RecordDone = RECORD_STATE_DONE;
    clip_filesize = filesize;
}



/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/

