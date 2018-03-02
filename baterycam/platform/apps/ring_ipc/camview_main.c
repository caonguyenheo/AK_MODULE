/*
*
*ring_main.c  first app statarted for ring call
*author:wmj
*date: 20161214
*/

#include "command.h"

#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#include "ak_common.h"
#include "ak_thread.h"
#include "ak_vi.h"
#include "ak_venc.h"
#include "ak_drv_ircut.h"
#include "ak_ao.h"
#include "ak_ai.h"
#include "ak_drv_vtimer.h"
#include "dev_info.h"
#include "kernel.h"

#include "cmd_api.h"
#include "photo.h"
#include "wifi.h"

#include "drv_gpio.h"
#include "sky_wifi_cfg.h"


#define LIB_RING_IPC_VERSION "ring_ipc_1.0.00"


// debug information config
#define AO_SAVE_TO_SD       0
#define AI_SAVE_TO_SD       0
#define AO_READ_FROM_SD     0
static bool sd_mount_ok = 0;
#define START_TIME_DEBUG    0

#define WIFI_KEEP_ALIVE     1

#define RING_RESTART_EN             1   // 0:disable 1:enable
#define SOCKET_CONN_TIMEOUT_EN      1   // 0:disable 1:enable
#define WDT_EN                      1   // 0:disable 1:enable


// debug information config
#define VIDEO_SAVE_PATH        "a:"
#define VIDEO_FILE_NAME        "video.h264"

#define FIRST_PATH       "ISPCFG"
#define BACK_PATH        "ISPCFG"

#define VIDEO_VGA_WIDTH				640
#define VIDEO_VGA_HEIGHT			360
#define VIDEO_VGA_HEIGHT_960P		480


/*task priority */
#define MAIN_TASK_PRIO 				40
#define VIDEO_TASK_PRIO 			75
#define WIFI_CONN_TASK_PRIO	    	78
#define SOUND_PLAY_TASK_PRIO		60
//#define AO_TASK_PRIO		        82
#define AO_TASK_PRIO		        74
#define SOCKET_RECV_TASK_PRIO       79
#define SOCKET_CONN_TASK_PRIO       78
#define CMD_TASK_PRIO 				80
#define RESEVER_TASK_PRIO           50  // battery and keep alive to stm8



/*max frame data len ENC_OUT_MAX_SZ(320K)+sizeof(VIDEO_STREAM_HEADER) + sizeof(VIDEO_STREAM_END */
#define FRAME_SEND_BUF_LEN          (320*1024+1024)//(320*1024)

/*  frame  type define */
enum frame_type {
    FRAME_TYPE_VIDEO_P = 0,
    FRAME_TYPE_VIDEO_I,
    FRAME_TYPE_AUDIO,
    FRAME_TYPE_PICTURE_PIR,
    FRAME_TYPE_PICTURE_RING

};

#define FRAME_HEADER_MAGIC    "FRAM" //0x4652414D  //"FRAM"
#define FRAME_END_MAGIC       "FEND" //0x46454E44  //"FEND"

#define CMD_HEADER_MAGIC      "VCMD" //0x56434D44  //"VCMD"
#define CMD_END_MAGIC         "CEND" //0x43454E44  //"CEND"

#define VIDEO_TCP_PORT  5005;
#define VIDEO_TCP_ADDR  "192.168.0.2"

#define TCP_SERVER_TASK_SIZE  (4096)


#define CMD_VIDEO_ARG       0x01
#define CMD_VIDEO_START     0x02
#define CMD_VIDEO_STOP      0x03

#define RESOLU_VGA          0
#define RESOLU_720P         1
#define RESOLU_960P         2

// audio 
#define AUDIO_RECV_BUF_SIZE         (4096)
#define AUDIO_RECV_BUF_NUM          16

#define AUDIO_RECV_BUF_EMPTY        0
#define AUDIO_RECV_BUF_READY        1
#define AUDIO_RECV_BUF_USE          2

typedef struct audio_recv_buf{
    unsigned char data[AUDIO_RECV_BUF_SIZE];
    unsigned int time;
    unsigned int len;
    int cnt;
    unsigned char state;
}audio_recv_buf_t;

typedef struct audio_recv_ctrl {
    unsigned int wait_recv_ok;
        
    // cyc buf ctrl
    unsigned int num;
    unsigned int index;
    audio_recv_buf_t buf[AUDIO_RECV_BUF_NUM];
}audio_recv_ctrl_t;
audio_recv_ctrl_t audioRecv;

// audio send
typedef struct audio_send_ctrl {
    int  cnt; 
    char data[4096+16];
}audio_send_ctrl_t;
audio_send_ctrl_t audioSend; 

struct pcm_param g_ao_info;


// tcp server
static int             g_tcp_socket_id;


static char *help_camview[]={
	"camview demo.",
	"usage: camview\n"
};


/** Find minimum value */
#define MIN(a, b) ((a) < (b) ? (a) : (b))


/*globle flag for video reques*/
// net stats machine 
#define NET_STA_WIFI_SLEEP          0
#define NET_STA_WIFI_CHECK          1
#define NET_STA_SOCKET_CHECK        2
#define NET_STA_NORMAL              3
const char net_stats_str[][20] = {
    "wifi init",
    "wifi conn",
    "socket conn",
    "net ok",
};
static volatile int g_net_conn_sta = NET_STA_WIFI_SLEEP;
// net stats semaphore 
static ak_sem_t  sem_wifi_conn_start;
static ak_sem_t  sem_wifi_conn_complete;
static ak_sem_t  sem_socket_conn_complete;


// preview control
static int       video_run_flag = 0;        // 0:stop 1:run
static ak_sem_t  sem_dingdong_start;
static ak_sem_t  sem_video_start;           // 
static ak_sem_t  sem_wakeup_src_recved;     // had recved wakeup source
static ak_mutex_t tcp_socket_mutex;         // tcp socket mutex


// audio semaphore
static ak_sem_t sem_audio_start;            // start audio
static ak_sem_t sem_aec_syc;                // use to aec synchron
static ak_mutex_t mutex_ao_dev;


#define VIDEO_MODE_RESERVER             0
#define VIDEO_MODE_PREVIEW              1
#define VIDEO_MODE_PHOTO_PIR            2
#define VIDEO_MODE_PHOTO_RING           3

struct  video_param
{
    unsigned char  mode;                    // 0:preview 1:photo
    unsigned char  sensor_fps;
    
    unsigned char  resolution;              // 0:vga 1:720p
    unsigned char  vi_use_ch;               // 0: main 1:sub
    unsigned char  fps;
    unsigned short kbps;
    
    unsigned short exp_time;
    char           day_night;               // 0: day  1:night
    
    unsigned char  para_change;             // 0: no 1:yes
    
};
static struct video_param g_video_para = {
    .sensor_fps = 25,
    .resolution =  1,                       // may change by mode 0:preview 1:photo
    .fps = 12,
    .kbps = 500,
    .exp_time = 859,
    .day_night = 0,
    .para_change = 0
};

struct  venc_param
{
    int width;
    int height;
    int fps;
    int kbps;
};
static struct venc_param g_rec_para;            // 视频设置中间参数
static struct video_resolution g_main_ch_res;
static struct video_resolution g_sub_ch_res;


keep_alive_t keep_alive_info;

#define  MAIN_APP_TICK_TIME          (100)
#define  MAIN_APP_TIME_UNIT_SEC      ((1000)/MAIN_APP_TICK_TIME)

typedef struct ring_device {
    // state flag
    volatile unsigned char wifi_init_flag;              // 0: init 1:fastlink
    volatile unsigned char sleep_en;                    // sleep flag
    volatile unsigned char wakeup_src;                  // wakeup source
    volatile unsigned char wakeup_cnt;                  // wakeup count
    volatile unsigned char speak_en;                    // 0: disable 1:enable
    volatile unsigned short bat;                        // battery

    // time count
    volatile int          app_delay;                    // 主应用延时计数
    volatile unsigned int dd_cnt;                       // dingdong count
    volatile int          app_num;                      // app num   
#if SOCKET_CONN_TIMEOUT_EN
    volatile int          net_conn_cnt;                 // socket connect fail timeout count
#endif
    // handle

}ring_device_t;

static ring_device_t ring_dev = {
    .wifi_init_flag = 0,
    .sleep_en  = 0,
    .wakeup_cnt = 0,
    .speak_en  = 0,
    .bat       = 0xffff,
    .app_delay = 0,
    .dd_cnt    = 0,
    .app_num   = 0,
    
#if SOCKET_CONN_TIMEOUT_EN
    .net_conn_cnt = 0,
#endif
};


static short audio_doorbell_buf[]=
{
	#include "4204.txt"
};

static void sky_speaker_enable(void *ao_handle)
{
    if(ring_dev.speak_en == 0) {
        ring_dev.speak_en = 1;
        ak_ao_enable_speaker(ao_handle,1);
    }

}
static void sky_speaker_disable(void *ao_handle)
{
    if(ring_dev.speak_en) {
        ring_dev.speak_en = 0;
        ak_ao_enable_speaker(ao_handle,0);
    }
}

static unsigned long sky_get_time_ms(void)
{
    struct ak_timeval time;
    unsigned long ms = 0;
    
    ak_get_ostime(&time);
    ms = time.sec * 1000 + time.usec/1000;
    return ms;
}

static void *sky_ao_open(void)
{
    static void *ao_handle= NULL;
	void *fd = NULL;

	g_ao_info.sample_bits = 16;
	g_ao_info.sample_rate = 8000;
	g_ao_info.channel_num = 1;

    // because ao_open have thread switch
    // must lock ao_open and get ao_handle
    ak_thread_mutex_lock(&mutex_ao_dev);
    if(ao_handle == NULL) {
        //open ao
        fd = ak_ao_open(&g_ao_info);
        if(fd == NULL)
        {
            ak_print_error("ao open false\n");
        }
        ao_handle = fd;

        //set volume
        ak_ao_set_volume(fd, 0x05);
    }
    ak_thread_mutex_unlock(&mutex_ao_dev);
    return ao_handle;

}

static void *open_file()
{
	char file[100] = {0};
	sprintf(file, "%s/%s", VIDEO_SAVE_PATH, VIDEO_FILE_NAME);

	FILE *filp = NULL;
	
	if (0 !=ak_mount_fs(DEV_MMCBLOCK, 0, ""))
    {
		ak_print_error("sd card mount failed!\n");
        return ;
    }
	
	filp = fopen(file, "w");
	if (!filp) {
		ak_print_error_ex("fopen, error\n");
	}

	return filp;
}

static void close_file(FILE *filp)
{
	fclose(filp);
	ak_unmount_fs(DEV_MMCBLOCK, 0, "");
}

static unsigned int crc_checksum(char *data, int len)
{
	int i = 0;
	unsigned int checksum = 0;
	for(i = 0; i < len; i++)
	{
		checksum += data[i];
	}
	ak_print_normal("crc begin %0x %0x %0x %0x len %d\n", data[0], data[1], data[2], data[3], len);
	return checksum;
}

//================ uart1 cmd =======================
static int send_cmd_bat_req()
{
	int ret;
	CMD_INFO cmd;
	cmd.preamble = CMD_PREAMBLE;
	cmd.cmd_id = CMD_BATTERY_GET_VAL;
	cmd.cmd_len = (EVENT_CMD_LEN);
	cmd.cmd_seq = (0);
	cmd.cmd_result = (0);
	cmd.param.event = (0);

    //ak_print_error("battery req\n");
	ret = send_one_cmd((char *)&cmd, EVENT_CMD_LEN + CMD_HEAD_LEN);
	if(ret < 0)
	{
		ak_print_error_ex("send cmd sleep error\n");
	}
	return ret;
}
static int send_cmd_sleep(void)
{
    int ret;
    CMD_INFO cmd;
    cmd.preamble = CMD_PREAMBLE;
    cmd.cmd_id = CMD_SLEEP_REQ;
    cmd.cmd_len = (EVENT_CMD_LEN);
    cmd.cmd_seq = (0);
    cmd.cmd_result = (0);
    cmd.param.event = (1);

	// power down
	cmd.cmd_id = CMD_SLEEP_REQ;
    ak_print_notice("ak39 power down req!\n");
	ret = send_one_cmd((char *)&cmd, EVENT_CMD_LEN + CMD_HEAD_LEN);
	if(ret < 0)
	{
		ak_print_error_ex("send cmd sleep error\n");
	}
    //console_free();
	return ret;
}
	
int send_cmd_wakeup_source_check()
{
	int ret;
	CMD_INFO cmd;
	cmd.preamble = CMD_PREAMBLE;
	cmd.cmd_id = CMD_WAKEUP_SRC_REQ;
	cmd.cmd_len = (EVENT_CMD_LEN);
	cmd.cmd_seq = (0);
	cmd.cmd_result = (0);
	cmd.param.event = (0);

	ak_print_normal("send cmd wakeup source\n");
	ret = send_one_cmd((char *)&cmd, EVENT_CMD_LEN + CMD_HEAD_LEN);
	if(ret < 0)
	{
		ak_print_error("send wake up source req error\n");
	}
	return ret;
}


#if RING_RESTART_EN
static int send_cmd_restart_req(unsigned char status)
{
#define APP_RESTART_CNT_MAX     3
	int ret;
	CMD_INFO cmd;
	cmd.preamble = CMD_PREAMBLE;
	cmd.cmd_id = CMD_SYS_RESTART_REQ;
	cmd.cmd_len = (EVENT_CMD_LEN);
	cmd.cmd_seq = (0);
	cmd.cmd_result = (0);
	cmd.param.event = (0);

    ring_dev.wakeup_cnt++;
    cmd.param.wakeup_para.status = ring_dev.wakeup_cnt;

    if(ring_dev.wakeup_cnt <= APP_RESTART_CNT_MAX) {
        ak_print_error("system restart req %d\n",ring_dev.wakeup_cnt); 
        ret = send_one_cmd((char *)&cmd, EVENT_CMD_LEN + CMD_HEAD_LEN);
    } else {
        ak_print_error("system restart too many,stop !\n");
        ret = send_cmd_sleep(); // wakeup_cnt clean at stm8
    }
	return ret;
}
#endif
//================ uart1 cmd end =======================


static int app_wifi_sleep(void *args)
{
    int ret;
    unsigned long t;
    int socket_s = -1;

    g_net_conn_sta = NET_STA_WIFI_SLEEP;  

    //ak_sleep_ms(1000);
#if WIFI_KEEP_ALIVE
#if START_TIME_DEBUG
        t = sky_get_time_ms();
        ak_print_warning("keep alive a %lu\n",t);
#endif

    memset(&keep_alive_info, 0, sizeof(keep_alive_info));
    keep_alive_info.socket = -1;
    socket_s = socket(AF_INET, SOCK_STREAM, 0);//tcp socket
    if(socket_s < 0) {
        ak_print_error("create socket error.\n");
    } else {
        unsigned int opt = 6000;
        struct sockaddr_in net_cfg;
        memcpy(&net_cfg,(struct sockaddr_in *)args,sizeof(struct sockaddr_in));
        net_cfg.sin_port = htons(5006);
        setsockopt(socket_s, SOL_SOCKET, SO_SNDTIMEO, &opt, sizeof(unsigned int)) ;
        ak_print_normal("keep alive conn...\n");
        int err = connect(socket_s, (struct sockaddr*)&net_cfg, sizeof(struct sockaddr_in));
        if (err != 0) {
            ak_print_error("keep alive connect error\n");
        } else {
            keep_alive_info.socket = socket_s;
        }
    }
    if(keep_alive_info.socket != -1) {
        keep_alive_info.enable = 1;
        keep_alive_info.mode = 1;
        keep_alive_info.send_interval = 10;
        keep_alive_info.retry_interval = 3;
        keep_alive_info.retry_count = 5;
        wifi_keepalive_set(&keep_alive_info);
    }
#endif
#if START_TIME_DEBUG
        t = sky_get_time_ms();
        ak_print_warning("keep alive b %lu\n",t);
#endif

    if(g_tcp_socket_id >= 0) {
        closesocket(g_tcp_socket_id);  // close socket,sever must check and close too
        //shutdown(keep_alive_info.socket, SHUT_RDWR); 
    }

    // power down prepare 
    //cmd.cmd_id = CMD_SLEEP_REQ_PRE;
    //ret = send_one_cmd((char *)&cmd, EVENT_CMD_LEN + CMD_HEAD_LEN);
    //ak_sleep_ms(1000); // wait wifi send data complete  
#if START_TIME_DEBUG
    t = sky_get_time_ms();
    ak_print_warning("sleep ta %lu\n",t);
#endif
    ak_print_normal("wifi sleep...\n");
    ret = wifi_sleep();
    if(ret != 0)
    {
        //wifi sleep fail,notice STM8 to reboot AK
        ak_print_error("wifi sleep error, reboot system\n");
        send_cmd_restart_req(0);
        while(1); //wait STM8 to reboot AK
    }
    ak_print_normal("wifi sleep complete\n");
#if START_TIME_DEBUG
    t = sky_get_time_ms();
    ak_print_warning("sleep tb %lu\n",t);
#endif
    ak_sleep_ms(300); // wait wifi sleep,otherwise may cause  wifi init next fail

    return ret;
}

static unsigned short battery_get()
{
	/*return battery*/
	return ring_dev.bat;
}

static void start_video(void)
{
    ak_print_notice("video start\n");
    ak_thread_sem_post(&sem_video_start);
    ak_thread_sem_post(&sem_audio_start);
    //ring_dev.app_delay = 0;
}

static void stop_video(void)
{
    ak_print_notice("video stop\n");
    video_run_flag = 0;
    ring_dev.app_delay = 0;
#if START_TIME_DEBUG
    long t = sky_get_time_ms();
    ak_print_warning("video stop t %lu\n",t);
#endif
    //ring_dev.sleep_en = 1;

}

static int set_video_param(void *enc_handle, struct  video_param *p_video_param )
{
	int ret;
	
	if(enc_handle != NULL)
	{
		ret = ak_venc_set_fps(enc_handle, p_video_param->fps); // must lesser than vi reality fps(lesser than 25)
		ret = ak_venc_set_gop_len(enc_handle,ak_venc_get_fps(enc_handle)*2);
		ret = ak_venc_set_rc(enc_handle, p_video_param->kbps);
	}
	else
	{
		ak_print_error("video encode handle no opened\n");
		ret = -1;
	}
	return ret;
}

/*
包格式:
包头标识 | 帧类型 | 时间戳 | 帧id | 帧长度 |   数据   | CRC校验 | 包尾标识
4Bytes     4Bytes   4Bytes   4Bytes 4Bytes    4Bytes    4Bytes    4Bytes
*/
static void packet_data_frame(char *send_buf, int type, int ts, int id, int len, char*p_data)
{
    unsigned int check_sum = 0;
    char *opt_ptr;

    opt_ptr = send_buf;
    memcpy(opt_ptr, FRAME_HEADER_MAGIC, 4);
    opt_ptr += 4;
    //*(int*)opt_ptr = type;
    memcpy(opt_ptr, &type, 4);
    opt_ptr += 4;

    memcpy(opt_ptr, &ts, 4);
    opt_ptr += 4;

    memcpy(opt_ptr, &id, 4);
    opt_ptr += 4;

    memcpy(opt_ptr, &len, 4);
    opt_ptr += 4;

    memcpy(opt_ptr, p_data, len);
    opt_ptr = send_buf + sizeof(VIDEO_STREAM_HEADER) + len;
    check_sum++;
    memcpy(opt_ptr, &check_sum, 4);
    opt_ptr += 4;
    memcpy(opt_ptr, FRAME_END_MAGIC, 4);

}

static int send_picture_data(struct video_stream *p_stream,int type)
{
    int ret = 0;
    // send
    SPI_STREAM_SEND_INFO    spi_send_buf;
    int i;
    int transfer_size, transfer_progress;
    char *send_buf = NULL;
    char *opt_ptr;
    int check_sum;
    int frame_len;
    int id = 0;

    send_buf = malloc(FRAME_SEND_BUF_LEN);
    if(send_buf == NULL)
    {
     ak_print_error("malloc buff for sending photo error\n");
     return;
    }

    frame_len = p_stream->len + sizeof(VIDEO_STREAM_HEADER) + sizeof(VIDEO_STREAM_END);

    //ak_thread_mutex_lock(&tcp_socket_mutex);

    // frame packet
    packet_data_frame(send_buf,type,p_stream->ts,id,p_stream->len,p_stream->data);
    id++;
    opt_ptr = send_buf;

#if 0
    ak_print_normal("send picture type%d time %d lenght %d\n", spi_send_buf.reserve,spi_send_buf.pack_id,frame_len);
    for(i =0;i < 16;i++)
       ak_print_normal(" %0x",opt_ptr[i]);
    ak_print_normal("\n");
#endif

    ret = 0;
    int cnt = 0;
    int send_len;
    do{
        ak_print_normal("send picture type%d time %d lenght %d\n", spi_send_buf.reserve,spi_send_buf.pack_id,frame_len);
        send_len = send_data(opt_ptr, frame_len);
        if(send_len !=  frame_len) {
            ak_print_warning("send picture frame data error\n");
            cnt++;
            if(cnt >= 3) {
                ret = -1;
                break;
            }
            ak_print_normal("send picture try again %d\n",cnt);
        }
        //ak_sleep_ms(300);
    }while(send_len != frame_len);

    
#if 0
    for ( transfer_progress = 0; transfer_progress < frame_len; transfer_progress += transfer_size, opt_ptr += transfer_size)
    {
      transfer_size = MIN( 30*1024, ( frame_len - transfer_progress ) );
      //ak_print_normal("send frame data %d\n", transfer_size);
      if(send_data(opt_ptr , transfer_size) != transfer_size)
      {
          ak_print_error("send picture frame data error\n");
          break;
      }
    }
    // ak_thread_mutex_unlock(&tcp_socket_mutex);
#endif

    free(send_buf);
    return ret;
}

/*
*wakeup src process rutine
*
*/
static int wakeup_process(CMD_INFO *cmd)
{
	if(cmd->cmd_id == CMD_WAKEUP_SRC_RESP)
	{
	    if(cmd->cmd_result == RESULT_WIFI_CONFIG_CLEAR) {
            // clear wifi config
            int ret = sky_wifi_clear_config();
            if(ret < 0) {
                ak_print_warning("clear wifi config fail\n");
            } else {
                ak_print_warning("clear wifi config ok\n");
            }
            ring_dev.wakeup_src = EVENT_NULL_WAKEUP;
        } else {
            // 曝光值设置
            g_video_para.exp_time = cmd->cmd_result;
            g_video_para.day_night =  cmd->param.wakeup_para.day_night;
            ring_dev.wifi_init_flag = cmd->param.wakeup_para.wifi_init_flag;
            ring_dev.wakeup_src = cmd->param.wakeup_para.wakeup_source;
            ring_dev.wakeup_cnt = cmd->param.wakeup_para.status;
            ak_print_normal("exp_time: %d\n",g_video_para.exp_time);           
#if 1
            switch(cmd->param.wakeup_para.wakeup_source) {
                case EVENT_SYS_INIT:
                /*system init process*/
                    ak_print_notice("sysinit WAKEUP\n");
                    break;
                    
    
                case EVENT_RTC_WAKEUP:
                /*RTC wake up process*/
                    ak_print_notice("RTC WAKEUP\n");
                    break;
                case EVENT_PIR_WAKEUP:
                    ak_print_notice("PIR WAKEUP\n");
                    break;
                case EVENT_RING_CALL_WAKEUP:
                    ak_print_notice("Ring call WAKEUP\n");
                    break;
                    
                case EVENT_VIDEO_PREVIEW_WAKEUP:
                    ak_print_notice("net WAKEUP\n");
                    break;
                //case EVENT_SYS_CONFIG_WAKEUP:
                //  sys_config();
                    break;
                default:
                    ring_dev.wakeup_src = EVENT_NULL_WAKEUP;   // no source,may stm8 error
                    ak_print_warning("unknown wakeup source, sleep again\n");
            }
#endif

        }
		
	}
	
}


/*
*read cmd from uart and process cmd
*/
static void* cmd_process_task(void *arg)
{
	char buf[128], *pbuf;
	int ret;
	CMD_INFO *cmd;
    unsigned int frameType;
    int i;

	ak_print_normal("cmd recv task start \n");
	while(1)
	{
		memset(buf, 0 ,sizeof(buf));
		ret = read_one_cmd(buf, sizeof(buf));

        //ak_print_normal("recv ret %d\n",ret);
        //ak_print_normal("recv cmd 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x\n",
        //buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
        //buf[6], buf[7], buf[8], buf[9], buf[10], buf[11] );

		if(ret == AK_OK)
		{
		    cmd = (CMD_INFO *)buf;
			ak_print_info("recv cmd %d event 0x%0x\n", cmd->cmd_id,cmd->param.event);
			switch(cmd->cmd_id)
			{
				case CMD_WAKEUP_SRC_RESP:
					wakeup_process(cmd);
                    ak_print_normal("post wakeup src semaphore\n");
                    ak_thread_sem_post(&sem_wakeup_src_recved);
					break;
                case CMD_RING_DINGDONG:
                    if(ring_dev.dd_cnt <= 1) {
                        ak_print_normal("post ring sem\n");
                        ak_thread_sem_post(&sem_dingdong_start);
                        ring_dev.dd_cnt++;
                    }
                    break;
                    
                case CMD_BATTERY_VAL:
                    ring_dev.bat = cmd->cmd_result;
                    g_video_para.day_night = cmd->param.wakeup_para.day_night; //动态设置白天黑夜模式
					break;

				default:
					ak_print_normal("unknown cmd id %d\n", cmd->cmd_id);
				
			}
		}
		else
		{
            continue;
		}
		//ak_sleep_ms(200);
	}
	
}


static void socket_conn_timeout_cb(signed long timer_id, unsigned long delay)
{
    ak_print_error("socket conn timeout\n");
    send_cmd_restart_req(0);
}


/*
*@brief:  tcp_client_task
*@return; socket id
*/
int socket_connect_task(void *args)
{
    unsigned int  t;
    int socket_s = -1;
	int err = -1;
	struct sockaddr_in *net_cfg = (struct sockaddr_in *)args;
		
	net_cfg->sin_family = AF_INET;
	net_cfg->sin_len = sizeof(struct sockaddr_in);

    while (1) {
        if(ak_thread_sem_wait(&sem_wifi_conn_complete)) {
            continue;
        }
        if(socket_s >= 0) {
            closesocket(socket_s);
        }
			
        socket_s = socket(AF_INET, SOCK_STREAM, 0);//tcp socket
        if(socket_s < 0)
        {
            ak_print_error("create socket error.\n");
            //g_net_conn_sta = NET_STA_WIFI_CHECK;
            //ak_thread_sem_post(&sem_wifi_conn_start);
            send_cmd_restart_req(0);
        }
        else
        {
            unsigned int opt = 3000;
            setsockopt(socket_s, SOL_SOCKET, SO_SNDTIMEO, &opt, sizeof(unsigned int)) ;
            
            //opt = 3000; // lwip no support
            //setsockopt(socket_s, SOL_SOCKET, SO_CONTIMEO, &opt, sizeof(unsigned int)) ;
            
            //opt =1;//  1: enable tcp_nodelay , 0: diable tcp_nodelay
            //setsockopt(socket_s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(unsigned int)) ;
#if START_TIME_DEBUG
        t = sky_get_time_ms();
        ak_print_warning("socket conn a %lu\n",t);
#endif 
            ak_print_notice("socket conn...\n");
            long vtimer_id = ak_drv_vtimer_open(6000,false,socket_conn_timeout_cb);
            err = connect(socket_s, (struct sockaddr*)net_cfg, sizeof(struct sockaddr_in));
            //ak_sleep_ms(4000); // test conn timeout
            ak_drv_vtimer_close(vtimer_id);

            if (err != 0)
            {
                ak_print_error("connect to %s:%d error %d\n", inet_ntoa(net_cfg->sin_addr), net_cfg->sin_port,err);
                closesocket(socket_s);
                socket_s = -1;

#if SOCKET_CONN_TIMEOUT_EN
                ring_dev.net_conn_cnt++;
                if(ring_dev.net_conn_cnt >= 6) {  // continuous fail restart system
                    //int init_flag = 0;
                    //wifi_init(&init_flag);

                     // 重启设备，重新初始化wifi
                    ak_print_error("socket conn cnt overflow\n");
                    send_cmd_restart_req(0);
                    
                } else 
#endif
                {
                    ak_sleep_ms(500);
                    g_net_conn_sta = NET_STA_WIFI_CHECK;
                    ak_thread_sem_post(&sem_wifi_conn_start);
                }
            }
            else
            {
#if SOCKET_CONN_TIMEOUT_EN
                ring_dev.net_conn_cnt = 0; // reset socket connect fail count
#endif

                ak_print_notice("socket conn ok!\n");
                
#if START_TIME_DEBUG
        t = sky_get_time_ms();
        ak_print_warning("socket conn b %lu\n",t);
#endif  
                g_tcp_socket_id = socket_s;
                g_net_conn_sta = NET_STA_NORMAL;
                ak_thread_sem_post(&sem_socket_conn_complete);
            }
        }
    }
	
	return socket_s;
}



/*********************************
*@brief:     tcp_server_task
*@param:  args  (struct sockaddr_in)
*@return: 
*********************************/
int tcp_server_task(void *args)
{
	
    int socket_s = -1;
	int new_socket = -1;
	struct sockaddr remote_addr;
	struct sockaddr_in *net_cfg;
	int sockaddr_len;
	unsigned long  sendcnt=0;
	int opt;
	int err = -1;
	
	net_cfg = (struct sockaddr_in *)args;
	
	net_cfg->sin_family = AF_INET;
	net_cfg->sin_len = sizeof(struct sockaddr_in);
	net_cfg->sin_addr.s_addr = INADDR_ANY;
	
	socket_s = socket(AF_INET, SOCK_STREAM, 0);//tcp socket
	if (socket_s < 0)
	{
		ak_print_error("create socket error.\n");
		
	}
	else
	{
	    //使server 能立即重用
		opt = SOF_REUSEADDR;
		if (setsockopt(socket_s, SOL_SOCKET, SOF_REUSEADDR, &opt, sizeof(int)) != 0) 
		{
			printf("set opt error\n");
		}
        else
        {
			err = bind(socket_s, (struct sockaddr*) net_cfg, sizeof(struct sockaddr_in));
			if (err !=0)
			{
				ak_print_error("bind socket error\n");
			}
			else
			{
				err = listen(socket_s, 4);
				if (err !=0 )
				{
					ak_print_error("listen socket error\n");
				}
	            else
	            {
	            	while(1)
	            	{
		                sockaddr_len = sizeof(struct sockaddr);
						ak_print_normal("waiting client to connect...\n");
						new_socket = accept((int)socket_s, (struct sockaddr*) &remote_addr, (socklen_t*) &sockaddr_len);
						if (new_socket < 0)
						{
							ak_print_error("accept socket error\n");
						}
		                else
		                {
		                	ak_print_debug("remote addr:%s. connected\n", remote_addr.sa_data);
		                	//return new_socket;
		                	g_tcp_socket_id = new_socket;
							tcp_recv_task(NULL);
		                }
	            	}
				}
			}
        }
	}
	
	if (-1 != socket_s)
	{
		closesocket(socket_s);
	}	
}

/*********************************
*@brief:     tcp_recv_task
*@param:  args  
*@return: 
*********************************/

int tcp_recv_task(void *args)
{
	char recv_buf[1024+32] = {0};
	int len;
    unsigned int frameType;
	unsigned short cmd_id;
	unsigned short cmd_arg_len;
	int i;
#if 0
#if AO_SAVE_TO_SD
    static int recv_frame_cnt = 0;
    struct ak_date systime;
      unsigned long free_size = 0; 
      char * path = "a:/";
      char file_path[255] = {0};
      
      free_size = ai_get_disk_free_size(path);//get T_card free size  
      if(free_size < 1024)
      {       
          ak_print_error_ex("free_size < AI_TCARD_FREE_SIZEKB\n",1024);
          //goto pcai_unmount;
      }

      memset(&systime, 0, sizeof(systime));
      ak_get_localdate(&systime);
      sprintf(file_path, "%sPCAI_%04d%02d%02d_%02d%02d%02d.pcm", path,systime.year, systime.month + 1, 
      systime.day + 1, systime.hour, systime.minute, systime.second);
      
      FILE *p;
      p = fopen(file_path, "w+");
#endif
#endif
    ak_print_normal("start tcp_recv_task\n");
    memset(&audioRecv,0,sizeof(audio_recv_ctrl_t));

	while(1)
	{
        if(ak_thread_sem_wait(&sem_socket_conn_complete)) {
            continue;
        }

        while(g_net_conn_sta == NET_STA_NORMAL) {
            len = recv_data(recv_buf, sizeof(recv_buf));
            if(len > 0)
            {
                audio_recv_buf_t *aRecv = &audioRecv.buf[audioRecv.index];
                /*parse data check header  "VCMD" + cmd id + cmd arg len + cmd arg + crc + "CEND"*/
        
            //ak_print_normal("recv socket len %d\n",len);
        #if 0 
            i = 0;
            ak_print_normal("recv from socket 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x\n",
            recv_buf[i], recv_buf[i+1], recv_buf[i+2], recv_buf[i+3], recv_buf[i+4], recv_buf[i+5],
            recv_buf[i+6], recv_buf[i+7], recv_buf[i+8], recv_buf[i+9], recv_buf[i+10], recv_buf[i+11] );
        #endif   
                for(i = 0; i < len; i++)
                {
                    if(AUDIO_RECV_BUF_READY == aRecv->state && audioRecv.wait_recv_ok){
                        if(aRecv->len > 0) {
                           if( recv_buf[i] == 'F' && recv_buf[i + 1] == 'E'\
                            && recv_buf[i + 2] == 'N' && recv_buf[i + 3] == 'D') 
                           {
                                aRecv->cnt -= 4;    //crc out
                                aRecv->state = AUDIO_RECV_BUF_USE;
                                
                                if(audioRecv.num < AUDIO_RECV_BUF_NUM) {
                                    audioRecv.num++;
                                    audioRecv.index++;
                                    audioRecv.index %= AUDIO_RECV_BUF_NUM; 
                                    //ak_print_normal("audio recv buf num %d\n", audioRecv.num);
                                } else {
                                    ak_print_warning("audio recv buffer flowOver\n");
                                }
#if 0
#if AO_SAVE_TO_SD
                                if(sd_mount_ok)
                                {
                                    if(recv_frame_cnt < 300)
                                    {
                                        //ak_print_normal("audio get frame cnt %d\n",recv_frame_cnt);
                                        if(NULL == p) 
                                        {
                                            ak_print_error_ex("create pcm file err\n");
                                        }
                                        if(fwrite(aRecv->data,aRecv->len,1,p) <= 0)
                                        {
                                            ak_print_error_ex("write file err\n");        
                                        }
                                    } else if(recv_frame_cnt == 300){
                                         ak_print_normal("####################################\n");
                                         ak_print_normal("         ao file close           \n");
                                         ak_print_normal("####################################\n");
                                         fclose(p);
                                    }
                                    recv_frame_cnt++;
                                }
#endif
#endif
                           }
                           else 
                           {
                                if(aRecv->cnt < AUDIO_RECV_BUF_SIZE)
                                    aRecv->data[aRecv->cnt++] = recv_buf[i];
                                else
                                    ak_print_error_ex("audio frame lenger than recv buffer\n");
                           }
                        } else {
                           aRecv->state = AUDIO_RECV_BUF_EMPTY;
                            ak_print_error("no far recv buf\n");
                        }
                      
                    }
                    else if(recv_buf[i] == 'F' && recv_buf[i + 1] == 'R' && recv_buf[i + 2] == 'A' && recv_buf[i + 3] == 'M') 
                    {
                        i += 4; // header(4) and time(4) = 0
                        memcpy(&frameType,&recv_buf[i],4); // get frame type
                        i += 4;
                        if(frameType == FRAME_TYPE_AUDIO)
                        {
                            memcpy(&aRecv->time,&recv_buf[i],4);
                            i += 4;
                            
                            /*audio fram lenght*/
                            memcpy(&aRecv->len,&recv_buf[i],4);
                            i += 3;   // for{}  have one i++
                            if(aRecv->state == AUDIO_RECV_BUF_EMPTY) {
                                aRecv->state = AUDIO_RECV_BUF_READY;
                                aRecv->cnt   = 0;  
                            }
                 
                        }
    
                    }
                    else if(recv_buf[i] == 'V' && recv_buf[i + 1] == 'C' && recv_buf[i + 2] == 'M' && recv_buf[i + 3] == 'D') 
                    {
                        i += 4;
                        /*check crc and cmd end*/
                        /*parse cmd id*/
                        cmd_id = recv_buf[i];
                        ak_print_normal("tcp cmd id 0x%0x\n", cmd_id);
                        i += 1;  /*1 bytes cmd id */
                        cmd_arg_len = recv_buf[i];
                        i += 1;  /*1 bytes cmd arg len*/
                        switch(cmd_id)
                        {
                            case CMD_VIDEO_ARG:
                                if( recv_buf[i] != g_video_para.resolution){
                                    g_video_para.resolution = recv_buf[i];
                                    g_video_para.para_change = 1;
                                }
    
                                i += 1;
                                if(recv_buf[i]  != g_video_para.fps) {
                                    g_video_para.fps = recv_buf[i];
                                    g_video_para.para_change = 1;
                                }
                                i += 1;
                                if(*(unsigned short*)&recv_buf[i] != g_video_para.kbps) {
                                    g_video_para.kbps = *(unsigned short*)&recv_buf[i];
                                    g_video_para.para_change = 1;
                                }
                                break;
                            case CMD_VIDEO_START:
                                start_video();
                                break;
                                
                            case CMD_VIDEO_STOP:
                                stop_video();
                                break;
                            default:
                                ak_print_error("unkown cmd\n");
                                break;  
                        }
                    }
                    else 
                    {
                        continue;
                    }
    
                }
                
            }
            else
            {
                ak_print_error("recv data from tcp socket error\n");
                break;
            }
        }
		
	}
}


/*
*@brief:  send_data
*@param: data
*@param: data_len
*@return; send len
*/
int send_data(char *data, int data_len)
{
    int ret = 0;
	int sendlen = 0;
    int cnt = 0;
    static int error_cnt = 0;

    // wait net connect
    while(g_net_conn_sta <= NET_STA_SOCKET_CHECK) {
        ak_sleep_ms(10);
        cnt++;
        if(cnt > 100) {
            cnt = 0;
            ak_print_normal("send tcp data wait %s\n",net_stats_str[g_net_conn_sta]);
        }
    }
    
#if 1
    ak_thread_mutex_lock(&tcp_socket_mutex);
    if(g_net_conn_sta == NET_STA_NORMAL) {

        //send max 20K onece,because haven set send timeout=3S.
        volatile int transfer_progress,transfer_size;
        for ( transfer_progress = 0; transfer_progress < data_len; transfer_progress += sendlen, data += transfer_size)
    	{
			transfer_size = MIN( 20*1024, ( data_len - transfer_progress ) );

            //ak_print_normal("tcp %d \n",transfer_size);
            long t1 = sky_get_time_ms();
            sendlen = send(g_tcp_socket_id, data, transfer_size, MSG_WAITALL);
            long t2 = sky_get_time_ms();     
            if((t2-t1) > 2000) {
                ak_print_normal("send time is too long,len %d time %lu\n",transfer_size,(t2-t1));
            }
            if(sendlen == 0) ak_print_warning("send tcp data len 0\n");
            if(sendlen < 0) break; // drop the frame if send error
        }

        // check send return
        if (sendlen < 0)
    	{
    	    error_cnt++;
            if(sendlen == ERR_TIMEOUT) {
                ak_print_warning("send tcp data timeout\n");
            } else {
    		    ak_print_error("send data from socket error %d\n", sendlen);
            }
            if(g_net_conn_sta != NET_STA_WIFI_CHECK && error_cnt > 5) {
                g_net_conn_sta = NET_STA_WIFI_CHECK;
                ak_print_normal("net reconn...\n");
                ak_thread_sem_post(&sem_wifi_conn_start);
            }
            ret = -1;
    	} else {
            error_cnt = 0;
            ret = data_len;
        }
    }   
    ak_thread_mutex_unlock(&tcp_socket_mutex);

	return ret;
#else 
    return data_len;
#endif 
}


/*
*@brief:  send audio data
*@param: data
*@param: data_len
*@return; send len
*/
static int send_audio_data(int time,int *p_id,int data_len,char *data)
{
    static char *opt_ptr;
    unsigned int check_sum = 0;
    int ret = 0;

#if 1
    if(audioSend.cnt == 0) {
        opt_ptr = &audioSend.data[0];
        memcpy(opt_ptr, FRAME_HEADER_MAGIC, 4);
        opt_ptr += 4;
        *(int*)opt_ptr  = FRAME_TYPE_AUDIO;
        opt_ptr += 4;
        memcpy(opt_ptr, &time, 4);
        opt_ptr += 4;
        memcpy(opt_ptr, p_id, 4);
        opt_ptr += 4;
        memcpy(opt_ptr, &data_len, 4);
    	opt_ptr += 4;
    } 
    
    memcpy(opt_ptr, data, data_len);
    audioSend.cnt += data_len;
    opt_ptr += data_len;

    if(audioSend.cnt >= 512) {
        memcpy(&audioSend.data[ sizeof(AUDIO_STREAM_HEADER)-4], &audioSend.cnt, 4);
        
        //ak_print_normal("ai send id %d len %d\n",*p_id,audioSend.cnt);
        (*p_id)++;
        check_sum++;
        memcpy(opt_ptr, &check_sum, 4);
        opt_ptr += 4;
        memcpy(opt_ptr, FRAME_END_MAGIC, 4);
        ret = send_data(&audioSend.data[0],audioSend.cnt + sizeof(AUDIO_STREAM_HEADER)+8);
        audioSend.cnt = 0;
    }
#endif


    return ret;
}


/*
*@brief:  recv_data
*@param: buff
*@param: buff_len
*@return; recv len
*/
int recv_data(char *buff, int buff_len)
{
	int len;
	
	len = recv(g_tcp_socket_id, buff, buff_len, MSG_WAITALL);
	return len;
	
}



static void save_stream(FILE *filp, unsigned char *data, int len)
{
	int ret = len;
	
	do {
		ret -= fwrite(data, 1, ret, filp);
	} while (ret > 0);
}



static int  set_dayornight_cfg(void * vi_handle)
{
    int status;
    int fd;
    int ret;

#if 0
	ret = ak_drv_ir_init();
	if (0 != ret) {
		ak_print_error_ex("ircut init fail\n");
		return -1;
	}
    
    fd = dev_open(DEV_CAMERA);
    if (fd <0)
    {
		ak_print_error_ex("dev_open fail!\n");
		return -1;
    }
    
    ret = dev_ioctl(fd, IO_CAMERA_IRFEED_GPIO_OPEN, &status);
    if (0 !=ret)
    {
		ak_print_error_ex("dev_ioctl IO_CAMERA_IRFEED_GPIO_OPEN fail!\n");
		return -1;
    }
    
    ret = dev_ioctl(fd, IO_CAMERA_IRFEED_GPIO_GET, &status);
    if (0 !=ret)
    {
		ak_print_error_ex("dev_ioctl IO_CAMERA_IRFEED_GPIO_GET fail!\n");
        dev_ioctl(fd, IO_CAMERA_IRFEED_GPIO_CLOSE, &status); 
		return -1;
    }
    
    
    //if (status == CAMERA_IRFEED_GPIO_STATUS_DAY) {
    if (g_video_para.exp_time >= 500) {
    //if (1) {
        ak_print_notice("set day isp config and ircut!\n");

        ak_drv_ir_set_ircut(IRCUT_STATUS_DAY);
        ak_vi_switch_mode(vi_handle, VI_MODE_DAYTIME); 
    } else {
        ak_print_notice("set night isp config and ircut!\n");
        ak_vi_switch_mode(vi_handle, VI_MODE_NIGHTTIME); 
        ak_drv_ir_set_ircut(IRCUT_STATUS_NIGHT);
    }
    dev_ioctl(fd, IO_CAMERA_IRFEED_GPIO_CLOSE, &status); 
#endif
    if (g_video_para.day_night) {
        ak_print_normal("set night mode\n");
        ak_vi_switch_mode(vi_handle, VI_MODE_NIGHTTIME); 
    } else {
        ak_print_notice("set day mode\n");
        ak_vi_switch_mode(vi_handle, VI_MODE_DAYTIME);
    }

    return 0;
}


static void video_para_init(struct video_param *p_para)
{
    // set resolution by mode
    if(p_para->mode == VIDEO_MODE_RESERVER) {
        p_para->resolution = RESOLU_VGA;
    } else {
        p_para->resolution = RESOLU_720P;
    }

	switch(p_para->resolution)
	{
		case RESOLU_VGA:
			g_rec_para.width  = 640; 
			g_rec_para.height= 360;
			break;
		case RESOLU_720P:
			g_rec_para.width  = 1280; 
			g_rec_para.height= 720;
			break;
		case RESOLU_960P:
			g_rec_para.width  = 1280; 
			g_rec_para.height= 720;
			break;
		default:
			g_rec_para.width  = 1280; 
			g_rec_para.height= 720;
			break;
	}
	g_rec_para.fps = p_para->fps;
	g_rec_para.kbps = p_para->kbps;
}


static void *camera_initial(const char *first, const char *second)
{

	/* open device */
	void *handle = ak_vi_open(VIDEO_DEV0);
	if (handle == NULL) {
		ak_print_error_ex("vi open failed\n");
		return NULL;
	}
    
#if START_TIME_DEBUG
        unsigned long t = sky_get_time_ms();
        ak_print_warning("vi open t %lu\n",t);
#endif

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
		ak_print_normal("sensor resolution height:%d,width:%d.\n",resolution.height,resolution.width);


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
    if (g_rec_para.width > 640 ) //use  main channel
    {
        g_video_para.vi_use_ch = VIDEO_CHN_MAIN;
        attr.res[VIDEO_CHN_MAIN].width = g_rec_para.width;
        attr.res[VIDEO_CHN_MAIN].height = g_rec_para.height;
    }
    else  //use sub channel
    {
        g_video_para.vi_use_ch = VIDEO_CHN_SUB;
        attr.res[VIDEO_CHN_SUB].width  = g_rec_para.width;
        attr.res[VIDEO_CHN_SUB].height = g_rec_para.height;
#if 1
        //change vga height if sensor height is 960 or 480
        if (g_rec_para.height==360 &&  
            (resolution.height == 960 || resolution.height==480))
        {           
            attr.res[VIDEO_CHN_SUB].height = 480;
            g_rec_para.height    =480;
        }
#endif        
    }

    g_main_ch_res.width  =  attr.res[VIDEO_CHN_MAIN].width;
    g_main_ch_res.height =  attr.res[VIDEO_CHN_MAIN].height;

    g_sub_ch_res.width  =  attr.res[VIDEO_CHN_SUB].width;
    g_sub_ch_res.height =  attr.res[VIDEO_CHN_SUB].height;

    
	if (ak_vi_set_channel_attr(handle, &attr))
		ak_print_error_ex("set channel attribute failed\n");

	/* get crop */
    ak_print_normal("====camera info====\n");
	struct video_channel_attr cur_attr ;
	if (ak_vi_get_channel_attr(handle, &cur_attr)) {
		ak_print_normal("ak_vi_get_channel_attr failed!\n");
	}else
	{
		ak_print_normal("crop:left=%d,top=%d, width=%d,height=%d\n"
			,cur_attr.crop.left,cur_attr.crop.top,cur_attr.crop.width,cur_attr.crop.height);

		ak_print_normal("pixel:main channel width=%d,height=%d;sub channel width=%d,height=%d\n"
			,attr.res[VIDEO_CHN_MAIN].width,attr.res[VIDEO_CHN_MAIN].height
			,attr.res[VIDEO_CHN_SUB].width,attr.res[VIDEO_CHN_SUB].height);
	}	

	
	/* don't set camera fps in demo */
	//ak_vi_set_fps(handle, g_rec_para.fps); // change to set venc fps
	ak_print_normal("capture fps: %d\n", ak_vi_get_fps(handle));
    ak_print_normal("===================\n");

	return handle;
}


static void *video_encode_initial(enum encode_output_type type,int res)
{
#if 1
	struct encode_param *param = (struct encode_param *)calloc(1,
			sizeof(struct encode_param));

    if(RESOLU_VGA == res) {
        param->width = g_sub_ch_res.width;
        param->height = g_sub_ch_res.height;
    } else {
        param->width = g_main_ch_res.width;
        param->height = g_main_ch_res.height;
    }

	param->minqp = 20;
	param->maxqp = 51;
	param->fps = g_rec_para.fps;
	param->goplen = param->fps * 2;
	param->bps = g_rec_para.kbps ; 
	param->profile = PROFILE_MAIN;
	if (param->width <=640) {
	    param->use_chn = ENCODE_SUB_CHN ; 
        param->enc_grp = ENCODE_SUBCHN_NET;
    } else {
    	param->use_chn = ENCODE_MAIN_CHN;
        param->enc_grp = ENCODE_MAINCHN_NET;
    }
	param->br_mode = BR_MODE_CBR;
	param->enc_out_type = type;
	void * encode_video = ak_venc_open(param);
	ak_print_normal("YMX : free 2: size = %d\n", sizeof(struct encode_param));
	free(param);
#else
struct encode_param *param = (struct encode_param *)calloc(1,
			sizeof(struct encode_param));


        param->width = g_main_ch_res.width;
        param->height = g_main_ch_res.height;

        

	param->minqp = 20;
	param->maxqp = 51;
	param->fps = g_rec_para.fps;
	param->goplen = param->fps * 2;
	param->bps = g_rec_para.kbps ; 
	param->profile = PROFILE_MAIN;
	if (param->width <=640) {
	    param->use_chn = ENCODE_SUB_CHN ; 
        param->enc_grp = ENCODE_SUBCHN_NET;
    } else {
    	param->use_chn = ENCODE_MAIN_CHN;
        param->enc_grp = ENCODE_MAINCHN_NET;
    }

	param->br_mode = BR_MODE_CBR;
	param->enc_out_type = H264_ENC_TYPE;
	void * encode_video = ak_venc_open(param);
	ak_print_normal("YMX : free 2: size = %d\n", sizeof(struct encode_param));
	free(param);

#endif
	return encode_video;
}

//unsigned char *readpcmBuf = NULL;

#if 1
void* ao_task()
{
	//ao parma
	int ret = 0;
	unsigned long pcm_indx = 0;
	
	unsigned int tmpBuf_size = 0;
	int *handle = NULL;
	int send_size, size;
	unsigned char *tmpBuf = NULL;



    ak_print_normal("ao task start...\n");

    //ao operate
    handle = sky_ao_open();
    if(handle == NULL) {
        ak_print_error("ao open fail!\n");
        ak_thread_exit();   
        return NULL;
    }

	tmpBuf_size = ak_ao_get_frame_size(handle);
	ak_print_normal("ao frame size =%d\n", tmpBuf_size);
	
	tmpBuf = (unsigned char *)malloc(tmpBuf_size);
	if(NULL == tmpBuf) {
	    ak_print_error("ao demo malloc false!\n");
        ak_ao_close(handle);
	    ak_thread_exit();
        return NULL;
	}
	
	sky_speaker_enable(handle);
	
#if AO_SAVE_TO_SD
	static int recv_frame_cnt = 0;
	struct ak_date systime;
	  unsigned long free_size = 0; 
	  char * path = "a:/";
	  char file_path[255] = {0};
	  
	  free_size = ai_get_disk_free_size(path);//get T_card free size  
	  if(free_size < 1024)
	  { 	  
		  ak_print_error_ex("free_size < AI_TCARD_FREE_SIZEKB\n",1024);
		  //goto pcai_unmount;
	  }

	  memset(&systime, 0, sizeof(systime));
	  ak_get_localdate(&systime);
	  sprintf(file_path, "%sPCAI_%04d%02d%02d_%02d%02d%02d.pcm", path,systime.year, systime.month , 
	  systime.day , systime.hour, systime.minute, systime.second);
	  
	  FILE *p;
	  p = fopen(file_path, "w+");
#endif

	ak_print_normal("begin ao............\n");
	ak_thread_sem_wait(&sem_aec_syc);
    ring_dev.app_num++;
    memset(&audioRecv,0,sizeof(audio_recv_ctrl_t));
    audioRecv.num = 0;
    audioRecv.index = 0;
    audioRecv.wait_recv_ok = 1;
	while(1)
	{		
		if(!video_run_flag)
		{
			break;
		}
        ret = 0;
      
    #if 1
        if(audioRecv.num > 0) {
            audio_recv_buf_t *aRecv = &audioRecv.buf[(audioRecv.index+\
                AUDIO_RECV_BUF_NUM-audioRecv.num)%AUDIO_RECV_BUF_NUM];
			
			//ak_print_error_ex("cnt %d\n",aRecv->cnt);
            if(aRecv->state == AUDIO_RECV_BUF_USE) 
            {
               if(1 == g_ao_info.channel_num)    
               {
				   
                   memcpy(tmpBuf, aRecv->data+pcm_indx, tmpBuf_size/2);
                   ret = tmpBuf_size/2;    
					
                   pcm_indx += tmpBuf_size/2;            
                   aRecv->cnt -= tmpBuf_size/2;
               }
               else
               {
                   memcpy(tmpBuf, aRecv->data+pcm_indx, tmpBuf_size);
                   ret = tmpBuf_size;
                   pcm_indx += tmpBuf_size;
                   aRecv->cnt -= tmpBuf_size;
               }
               
               if(aRecv->cnt <= 0) {
                   aRecv->state = AUDIO_RECV_BUF_EMPTY;
                   memset(aRecv->data,0,AUDIO_RECV_BUF_SIZE);
				   pcm_indx = 0;
                   audioRecv.num--;
               }
            }
        }
		else
		{
			ak_sleep_ms(5);
			continue;
		}
	#endif
        
#if AO_SAVE_TO_SD
		//if(sd_mount_ok)
		{
			if(recv_frame_cnt < 500)
			{
				//ak_print_normal("audio get frame cnt %d\n",recv_frame_cnt);
				if(NULL == p) 
				{
					ak_print_error_ex("create pcm file err\n");
				}
				if(fwrite(tmpBuf,ret,1,p) <= 0)
				{
					ak_print_error_ex("write file err\n");		  
				}
			} else if(recv_frame_cnt == 500){
				 ak_print_normal("####################################\n");
				 ak_print_normal("		   ao file close		   \n");
				 ak_print_normal("####################################\n");
				 fclose(p);
			}
			recv_frame_cnt++;
		}
#endif
#if AO_READ_FROM_SD
        //ret = fread(tmpBuf, ret,1,fd);
        //if(ret <= 0) {
       //   fd = fopen("001.pcm", "r" );
       // }
#endif
        //ak_print_normal("audio speaker lenght %d\n",ret);
		send_size = 0;
		while(ret > 0)
		{
		    //long tick1;
            //long tick2;
            //tick1 = sky_get_time_ms();
			size = ak_ao_send_frame(handle, tmpBuf + send_size, ret, 10);
            //tick2 = sky_get_time_ms();
            //ak_print_normal("ao time %d\n",tick2-tick1);
            
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
	}

	//close SPK
	sky_speaker_disable(handle);
    
ao_end:	
    ring_dev.app_num--;
    ak_print_notice("ao task exit!\n");
    if(tmpBuf != NULL) {	
		free(tmpBuf);
	}
    memset(&audioRecv,0,sizeof(audio_recv_ctrl_t));
    pcm_indx = 0;
	ak_ao_close(handle);
	ak_thread_exit();
    return NULL;
}		
#endif
#if 1
void* ai_task()
{
	//ai parma	
	int ai_ret = -1;
	struct pcm_param input;
	struct frame frame = {0};	
	int id = 0;


	//ai operate	
	input.sample_bits = 16;
	input.channel_num = 1;
	input.sample_rate = 8000;

#if AI_SAVE_TO_SD  
    static int ai_frame_cnt = 0;
    struct ak_date systime;
    unsigned long free_size = 0; 
    char * path = "a:/";
    char file_path[255] = {0};

    free_size = ai_get_disk_free_size(path);//get T_card free size  
    if(free_size < 1024)
    {       
      ak_print_error_ex("free_size < AI_TCARD_FREE_SIZEKB\n",1024);
      //goto pcai_unmount;
    }

    memset(&systime, 0, sizeof(systime));
    ak_get_localdate(&systime);
    sprintf(file_path, "%sAOPC_%04d%02d%02d_%02d%02d%02d.pcm", path,systime.year, systime.month , 
    systime.day , systime.hour, systime.minute, systime.second);

    FILE *p;
    p = fopen(file_path, "w+");
#endif


	//g_rec_end = 0;
	//ak_print_normal("wait ai open...\n");
	
	void *adc_drv = ak_ai_open(&input);
	ak_ai_set_source(adc_drv, AI_SOURCE_MIC);
	/*
		AEC_FRAME_SIZE	is	256，in order to prevent droping data,
		it is needed  (frame_size/AEC_FRAME_SIZE = 0), at  the same 
		time, it  need	think of DAC  L2buf  , so  frame_interval  should  
		be	set  32ms。
	*/
	if(ak_ai_set_frame_interval(adc_drv, 32) != 0)
	{
		ak_print_error_ex("set_frame_size error!\n");
        ak_ai_close(adc_drv);
    	ak_thread_exit();
        return NULL;
	}

	ak_ai_clear_frame_buffer(adc_drv);

	if(ak_ai_set_volume(adc_drv, 7)!=0)
	{	
		ak_print_error_ex("set gain error!\n");
        ak_ai_close(adc_drv);
    	ak_thread_exit();
        return NULL;
		
	}
    
    ak_thread_sem_wait(&sem_audio_start);
    ring_dev.app_num++;

	ak_ai_set_aec(adc_drv, 1);
    ak_print_normal("ai open ...\n");
	ak_thread_sem_post(&sem_aec_syc);
	while(1)
	{
		if(!video_run_flag)
		{
            ak_print_normal("video_run_flag is 0\n");
			break;
		}
        
		#if 1
		ai_ret = ak_ai_get_frame(adc_drv, &frame, 1);       
		if(ai_ret == -1)
		{
			ak_sleep_ms(5);
			continue;							
		}
		#endif
		//memcpy(&aecBuf[aec_index], frame.data, frame.len);
#if AI_SAVE_TO_SD
                    if(sd_mount_ok)
                    {
                    
                        if(ai_frame_cnt < 200)
                        {
                            //ak_print_normal("ai frame cnt %d\n",file_cnt);
                            if(NULL == p) 
                            {
                                ak_print_error_ex("create pcm file err\n");
                                //goto pcai_unmount ;
                            }
                            if(fwrite(frame.data,frame.len,1,p) <= 0)
                            {
                                ak_print_error_ex("write file err\n");
                                //goto pcai_close;          
                            }
                        } else if(ai_frame_cnt == 200){
                             ak_print_normal("####################################\n");
                             ak_print_normal("         ai file close           \n");
                             ak_print_normal("####################################\n");
                             fclose(p);
                        }
                        ai_frame_cnt++;
                    }
#endif


#if 1
        if(frame.len > 0) {
            if(send_audio_data((long)frame.ts,&id,frame.len,frame.data)< 0)
            {
                ak_print_error("send audio frame to pc error\n");
            }
        }
#endif
  
		ak_ai_release_frame(adc_drv, &frame);		
	}

ai_close:
    ring_dev.app_num--;
    ak_print_notice("ai task exit !\n");
	ak_ai_close(adc_drv);
	adc_drv = NULL;
	ak_thread_exit();
    return NULL;


}

#endif


static int video_encode_manual(void *vi_handle,void *venc_handle, struct video_stream *stream_handle)
{
    //struct video_stream stream = {0};
    struct video_input_frame input_frame;
    unsigned int use_chn = g_video_para.vi_use_ch;
    
    //get a video frame from video input
    int ret = ak_vi_get_frame(vi_handle, &input_frame);
    if (ret != 0) {
        ak_print_error_ex("get vi frame failed\n");
        return -1;
    }
    else
    {
        //encode a frame
        ret = ak_venc_send_frame(venc_handle , input_frame.vi_frame[use_chn].data,input_frame.vi_frame[use_chn].len
            , stream_handle);                            
        stream_handle->ts = input_frame.vi_frame[use_chn].ts;

        ak_vi_release_frame(vi_handle,&input_frame);
        return 0;
    }
}

static int video_photo_proc(void *vi_handle,int type)
{
    int ret = -1;
    void *venc_handle = NULL;
    struct video_stream stream;
    int photo_get_cnt = 0;
    
    ak_print_normal("photo...\n");
    venc_handle  = video_encode_initial(MJPEG_ENC_TYPE,g_video_para.resolution);
    if(venc_handle == NULL) {
        return -1;
    }

    while(1) {
        ret = video_encode_manual(vi_handle,venc_handle,&stream);
        if(ret) {
            ak_sleep_ms(5);
            photo_get_cnt++;
            if(photo_get_cnt > 25) {
                ret = -1;
                ak_print_error("photo encode fail!\n");
                break;
            } 
            else continue;
        } else {
             // send picture
            ak_print_normal("photo encode ok\n");
            ret = send_picture_data(&stream,type);     
            if (NULL != stream.data ) {
                // use ak_venc_send_frame,must free stream.data
                free(stream.data);
            }
            if(ret) {
                ak_print_error("photo send fail !\n");
            } else {
                ak_print_notice("photo send ok\n");
            }

            break;
        }
    }
    ak_venc_close(venc_handle);
    return ret;

}

void *video_init(void)
{
    unsigned long t;
    void *vi_handle = NULL;

#if START_TIME_DEBUG
    t = sky_get_time_ms();
    ak_print_warning("sensor match ta %lu\n",t);
#endif
    // match sensor
    if (ak_vi_match_sensor(FIRST_PATH) < 0)
    {
        ak_print_error_ex("match sensor failed\n");
        return NULL;
    }
    
#if START_TIME_DEBUG
    t = sky_get_time_ms();
    ak_print_warning("sensor match tb%lu\n",t);
#endif

    video_para_init(&g_video_para);
    cam_set_sensor_ae_info(&g_video_para.exp_time);

    // vi init
    vi_handle = camera_initial(FIRST_PATH, BACK_PATH);
#if START_TIME_DEBUG
    t = sky_get_time_ms();
    ak_print_warning("vi init t %lu\n",t);
#endif
    return vi_handle;
}

/*
*start a thread to encode and send video
*/

volatile unsigned long g_bps_test = 0;

void *video_encode_task(void *arg)
{
	void *vi_handle, *venc_handle, *stream_handle;
	struct video_resolution resolution = {0};
	char *send_buf = NULL;
	char *opt_ptr;
	int check_sum;
	int i;
    int ret;
    unsigned long t;
    int id = 0;
    

	ak_print_notice("vi init\n");
    vi_handle = video_init();
    if (vi_handle == NULL) {
        ak_print_error_ex("vi init fail!\n");
        goto exit_task;
    }
    ak_print_notice("vi init ok\n");
    ak_vi_capture_on(vi_handle);


    if(g_video_para.mode == VIDEO_MODE_RESERVER) {
        // photo only
        ak_print_normal("photo only\n");
        if(video_photo_proc(vi_handle,FRAME_TYPE_PICTURE_PIR)) {
            ak_print_error("photo fail\n");
        }
        goto exit_vi;
    }
    else  
    // photo and preview
    {
        ak_print_normal("photo and preview\n");
        //======photo=======
        if(g_video_para.mode == VIDEO_MODE_PHOTO_PIR || g_video_para.mode == VIDEO_MODE_PHOTO_RING) {
            //photo at first
            int photo_type;
            if(g_video_para.mode == VIDEO_MODE_PHOTO_PIR) {
                ring_dev.app_num++;
                photo_type = FRAME_TYPE_PICTURE_PIR;
                ret = video_photo_proc(vi_handle,photo_type);
                ring_dev.app_num--;
                goto exit_vi;
            } else {
                ring_dev.app_num++;
                photo_type = FRAME_TYPE_PICTURE_RING;
                if(video_photo_proc(vi_handle,photo_type)) {
                    ring_dev.app_num--;
                    goto exit_vi;
                }
                ring_dev.app_num--;
            }

        }

        //======preview=======
        // wait video sem to start preview
        ak_thread_sem_wait(&sem_video_start);
        ring_dev.app_num++;
        
        ak_print_notice("preview venc init...\n");

        venc_handle = video_encode_initial(H264_ENC_TYPE,g_video_para.resolution);
        if (venc_handle == NULL) {
    		ak_print_error_ex("video encode open failed!");
            goto exit_prew_vi;
	    }
        stream_handle = ak_venc_request_stream(vi_handle,venc_handle);
        if (stream_handle == NULL) {
		    ak_print_error_ex("request stream failed\n");
            goto exit_prew_venc;
	    }

        ak_print_notice("start video transfer");
        send_buf = malloc(FRAME_SEND_BUF_LEN);
        if(send_buf == NULL)
        {
            ak_print_error("malloc buff for sending frame error\n");
            ak_venc_cancel_stream(stream_handle);
            goto exit_prew_venc;
        }
#if START_TIME_DEBUG
        t = sky_get_time_ms();
        ak_print_warning("venc get stream ta %lu\n",t);
#endif
        struct video_stream stream = {0};
        struct video_stream *p_stream = &stream;
        unsigned int res_temp = g_video_para.resolution; // temp resolution
        char         day_night = g_video_para.day_night;
        int   get_stream_cnt = 0;
        
        while(video_run_flag) {
            
            // change video para reinit venc
            if(g_video_para.para_change){
                 ak_print_normal("set video para\n");
                if(res_temp != g_video_para.resolution) {
                    res_temp = g_video_para.resolution;
                    ak_venc_cancel_stream(stream_handle);
                    ak_venc_close(venc_handle);

                    ak_print_normal("change resolution,venc reInit \n");
                    venc_handle = video_encode_initial(H264_ENC_TYPE,g_video_para.resolution);
                    if (venc_handle == NULL) {
                		ak_print_error_ex("video encode open failed!");
                        free(send_buf);
                		goto exit_prew_vi;
            	    }
                    stream_handle = ak_venc_request_stream(vi_handle,venc_handle );
                    if (stream_handle == NULL) {
            		    ak_print_error_ex("request stream failed\n");
            		    goto exit_prew_venc;
            	    }
                }
                
                // preview set vi fps to get low power
                if(g_video_para.sensor_fps != g_video_para.fps) {
                    g_video_para.sensor_fps = g_video_para.fps;
                    ret = ak_vi_set_fps(vi_handle, g_video_para.sensor_fps);
                    if(ret) {
                        ak_print_error("capture fps set fail %d\n",g_video_para.fps);
                    } else {
                        ak_print_normal("capture fps set %d\n",g_video_para.fps);
                    }
                }
                set_video_param(venc_handle, &g_video_para);
                g_video_para.para_change = 0;
            }
            if(day_night != g_video_para.day_night) //动态设置白天黑夜模式
            {
				day_night = g_video_para.day_night;
				if (0 != set_dayornight_cfg(vi_handle))
    			{
					 ak_print_error_ex("set_dayornight_cfg failed\n");
			    }
			}
            
            ret = ak_venc_get_stream(stream_handle, &stream);
            if(ret != 0) {
                get_stream_cnt++;
                ak_venc_release_stream(stream_handle, &stream);
                ak_sleep_ms(10);
                if(get_stream_cnt >= 300) { // get stream fail in 3s,stop
                    get_stream_cnt = 0;
                    ak_print_error("venc get stream fail!\n");
                    stop_video();
                }
                continue;
            }
            get_stream_cnt = 0;            // get stream ok,clear count

            // calculate bps
            g_bps_test += p_stream->len;

            p_stream = &stream;
#if 1
            // frame packet
            packet_data_frame(send_buf,p_stream->frame_type,p_stream->ts,id,p_stream->len,p_stream->data);

            // frame send
            int send_len = 0;
            send_len = send_data(send_buf, p_stream->len + sizeof(VIDEO_STREAM_HEADER) + sizeof(VIDEO_STREAM_END));
            if(send_len < 0)
            {
                ak_print_error("send video frame error\n");
            }
            i = 4;
            //ak_print_normal("video type %d ts %lu id %d len %d data %0x %0x %0x %0x %0x \n",p_stream->frame_type,(long)p_stream->ts,id,send_len,send_buf[i+0],send_buf[i+1],send_buf[i+2],send_buf[i+3],send_buf[i+4],send_buf[i+5]);
            
            id++;
            check_sum++;
#endif 	
            ret = ak_venc_release_stream(stream_handle, &stream);
            if (ret != 0) {
                ak_print_error_ex("release stream failed\n");
            }
                    
        }

        ak_print_normal_ex("cancel stream ...\n");
        if(stream_handle != NULL) {
            ak_venc_cancel_stream(stream_handle);
        }
            
exit_prew_venc:
        if(send_buf != NULL)free(send_buf);
        ak_venc_close(venc_handle);
    }

exit_prew_vi:
    ring_dev.app_num--;
    ak_print_notice("video task exit !\n");
exit_vi:
    ak_vi_capture_off(vi_handle);
	ak_vi_close(vi_handle);
exit_task:
    if(video_run_flag){
        video_run_flag = 0;
        ak_print_normal("stop other thread\n");
    }
	ak_thread_exit();
	return NULL;
}



/*keep connection thread*/
static void wifi_conn_thread(void *args)
{
    int get_ip_cnt = 0;
    int ret;
    struct net_sys_cfg *p_cfg = (struct net_sys_cfg*)args;

    // init wifi
#if START_TIME_DEBUG
    unsigned long t = sky_get_time_ms();
    ak_print_warning("wifi init a%lu\n",t);
#endif

    //ak_thread_sem_wait(&sem_wifi_init);     // wait wifi init para wifi_init_flag from stm8
    ak_print_notice("wifi init para %d\n",ring_dev.wifi_init_flag);
    if(wifi_init(&ring_dev.wifi_init_flag) == 0)  {
        ak_print_notice("init wifi ok!!!\n");
    } else {
        ak_print_error("init wifi error!!!\n");
        // 重启设备，重新初始化wifi
        send_cmd_restart_req(0);
    }

#if START_TIME_DEBUG
    t = sky_get_time_ms();
    ak_print_warning("wifi init b%lu\n",t);
#endif

    // if wakeup by beacon miss or keep alive timeout sleep
    #if WIFI_KEEP_ALIVE
    if(wifi_check_wakeup_status() < 0)
    {
        ak_print_error("wifi wakeup abnormal, stop\r\n");
        // 停止，待连接上服务器，直接睡眠，无需等待唤醒
        stop_video();
    }
    #endif

    wifi_set_mode(WIFI_MODE_STA);
    g_net_conn_sta = NET_STA_WIFI_CHECK;
	while(1)
	{
        if(ak_thread_sem_wait(&sem_wifi_conn_start)) {
            continue;
        }
        while(1) {
            if (!wifi_isconnected()) // not connected
            {   
                ak_print_normal("Connect to SSID: < %s >  with password:%s\n", p_cfg->ssid,  p_cfg->key);
                wifi_connect(p_cfg->ssid, p_cfg->key);   // wifi_connect需要时间，下面的if不一定成立
                if (wifi_isconnected())     
                {
                    ak_print_notice("wifi connect ok\n");
                  	
                } else {
                    ak_print_normal("wifi connect++ \n");
		            ak_sleep_ms(50);
					continue;
                }
			}

			wifi_power_cfg(0); // enable power save
			
			ret = wifistation_netif_init();
             if(0 == ret) {
                get_ip_cnt = 0;
			 	ak_print_notice("get ip OK\n");
                g_net_conn_sta = NET_STA_SOCKET_CHECK;
                ak_thread_sem_post(&sem_wifi_conn_complete);
                break;
             }
			 else
			 {
			    ak_print_error("get ip fail\n");
			    get_ip_cnt++;
                if(get_ip_cnt >= 3) {
					get_ip_cnt = 0;
                    send_cmd_restart_req(0);
                }
			 }
            
        }

	}
	ak_print_normal("sta conn thread exit.\n");
	ak_thread_exit();
}


static void* doorbell_thread(void *arg)
{
	int ret;
	unsigned int fram_size = 0;
	void *fd = NULL;
	unsigned char file_over = 0;
	int send_size, size;
	
	unsigned char* curPtr = (unsigned char*)audio_doorbell_buf;
	unsigned char* endPtr = (unsigned char*)audio_doorbell_buf + sizeof(audio_doorbell_buf);
	unsigned char *tmpBuf;

    fd = sky_ao_open();
    if(fd == NULL) {
        ak_thread_exit();
        return NULL;
    }
    
    sky_speaker_enable(fd);

    // get ao fram size
    fram_size = ak_ao_get_frame_size(fd);
    tmpBuf = malloc(fram_size);
    if(NULL == tmpBuf)
    {
        ak_print_error("ao demo malloc false!\n");
        goto doorbell_exit ;
    }
      
#if START_TIME_DEBUG
    unsigned long t = sky_get_time_ms();
    ak_print_warning("dingdong ao open t %lu\n",t);
#endif

    while (1) {
        ak_thread_sem_wait(&sem_dingdong_start);
        if(ring_dev.dd_cnt > 0)ring_dev.dd_cnt--;
#if START_TIME_DEBUG
        t = sky_get_time_ms();
        ak_print_warning("dingdong start t %lu\n",t);
#endif           
        ak_print_normal("dingdong start\n");
        curPtr = (unsigned char*)audio_doorbell_buf;
        endPtr = (unsigned char*)audio_doorbell_buf + sizeof(audio_doorbell_buf);
        file_over = 0;

        while (1) {
            if(endPtr - curPtr>fram_size)
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
                size = ak_ao_send_frame(fd, tmpBuf + send_size, ret, 10);
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
                break;
            }
          
        }
        mini_delay(900);
    }
	
	//close ao
	//ak_ao_close(fd);
doorbell_exit:	
    sky_speaker_disable(fd);
    free(tmpBuf);
    ak_print_normal("*****doorbell exit ******\n");
    ak_thread_exit();
    return NULL;
}

static void* battery_thread(void *arg)
{
    unsigned int tick = 0;
    unsigned short bat;

    
    tick = 3000; // must lesser wdt time
    while(1) {
#if 1
    #if WDT_EN
        if(g_net_conn_sta == NET_STA_NORMAL){
            ak_drv_wdt_feed();
        }
    #endif
        send_cmd_bat_req();
        ak_sleep_ms(tick);
        bat = battery_get();

        //ak_print_normal("battery %d\n", bat);
    	if( bat < BATTERY_ALARM_THRESHOLD) {
            //
    	}
        //send to net  
        //ak_sleep_ms(10*1000-cmd_recv_time);
        //ak_sleep_ms(cmd_recv_time);
        //ak_print_normal("bb\n");
        //ak_print_normal("%d\r\n", 100-idle_get_cpu_idle());
        //ak_print_normal("%lu\n",g_bps_test);
        //g_bps_test = 0;
        //ak_sleep_ms(200);
#endif 
       
    }
}


/*
*main process to start up
*/
void* camview_main(int argc, char **args)
{
	int ret;
	int id;
    ak_pthread_t doorbell_id;
	struct sockaddr_in sockaddr;
    struct video_resolution resolution = {0};

    //ak_print_set_level(LOG_LEVEL_NOTICE);
#if START_TIME_DEBUG
    unsigned long t = sky_get_time_ms();
    ak_print_warning("camview main %lu\n",t);
#endif
    ak_print_normal("%s\n",LIB_RING_IPC_VERSION);

    /*init semaphore*/
    ret = ak_thread_sem_init(&sem_wakeup_src_recved, 0);
	ret |= ak_thread_sem_init(&sem_video_start, 0);
	ret |= ak_thread_sem_init(&sem_audio_start,0);
    ret |= ak_thread_sem_init(&sem_aec_syc,0);
    ret |= ak_thread_sem_init(&sem_wifi_conn_start, 1);     // start wifi conn at first 
    ret |= ak_thread_sem_init(&sem_wifi_conn_complete, 0);      
    ret |= ak_thread_sem_init(&sem_socket_conn_complete, 0);  
    ret |= ak_thread_sem_init(&sem_dingdong_start, 0); 
    ret |= ak_thread_mutex_init(&mutex_ao_dev); 
    ret |= ak_thread_mutex_init(&tcp_socket_mutex);         // mutex audio video data send to net 
	if(ret < 0)
	{
		ak_print_error("init sem error \n");
		goto camview_exit;
	}
    
	/*init uart for cmd communication*/
	if(0 == cmd_uart_init()) {
        ak_print_normal("uart1 init ok\n");
    }
    /*start cmd recv thread */
    ret |= ak_thread_create(&id, cmd_process_task, NULL, 4096, CMD_TASK_PRIO);
    /*send cmd to get wakeup source */
    send_cmd_wakeup_source_check();

    // wait wakeup source get back
    ak_thread_sem_wait(&sem_wakeup_src_recved);
    
    // get wifi config 
    struct net_sys_cfg sys_cfg;
#if 1
        /*read wifi config file*/
        ret = sky_wifi_get_config(&sys_cfg);
        ak_print_normal("ssid %s\n",sys_cfg.ssid);
        ak_print_normal("password %s\n",sys_cfg.key);
        
        if(ret < 0 || (strlen(sys_cfg.ssid) == 0) \
            ||(sys_cfg.server_ip == 0) || (IPADDR_NONE == sys_cfg.server_ip)\
            || (sys_cfg.port_num > 65535)|| (sys_cfg.port_num == 0)) {
            // create battery thread to keep alive with stm8
            //ret |= ak_thread_create(&id, battery_thread, NULL, 4*1024, RESEVER_TASK_PRIO); 
            sky_wifi_set_with_net();
            ak_print_normal("wifi config is null\n");
            ak_print_normal("pl use 'wifista config' cmd or connect ap 'ssid=anyka_sky key=123456789' to config \n");
            goto camview_exit;
        }
#else
        #define WIFI_SSID  "cql"           //"tp-wifi"   "H3C_22" 
        #define WIFI_KEY  "1234567890"   //"ak123456789"    "anyka123456" 
            
        memcpy(sys_cfg.ssid,WIFI_SSID,sizeof(WIFI_SSID));
        memcpy(sys_cfg.key,WIFI_KEY,sizeof(WIFI_KEY));
        sys_cfg.server_ip = inet_addr((char *)"192.168.1.100");
        sys_cfg.port_num  = atoi("5005");
#endif

#if WDT_EN
    // watch dog
    ak_drv_wdt_open(30000);
#endif

    if(ring_dev.wakeup_src == EVENT_NULL_WAKEUP)    
    {
        ak_print_error("wakeup source get null! sleep\n");   
    } 
    else // wakeup source get OK 
    { 

        if(ring_dev.wakeup_src != EVENT_SYS_INIT) { 
            video_run_flag = 1; // init video run enable default
            
            //if(ring_dev.wakeup_src == EVENT_PIR_WAKEUP){
            if(ring_dev.wakeup_src == EVENT_PIR_WAKEUP ) {
                g_video_para.mode = VIDEO_MODE_PHOTO_PIR;
                ring_dev.app_delay = 3*MAIN_APP_TIME_UNIT_SEC;
                //ak_thread_sem_post(&sem_video_start);
            } else if(ring_dev.wakeup_src == EVENT_RING_CALL_WAKEUP) {
                // send msg to sever and wait 30s 
                g_video_para.mode = VIDEO_MODE_PHOTO_RING;
                ring_dev.app_delay = 30*MAIN_APP_TIME_UNIT_SEC;
                ak_thread_sem_post(&sem_dingdong_start);
                //ak_thread_sem_post(&sem_video_start);
            } else {// EVENT_VIDEO_PREVIEW_WAKEUP
                g_video_para.mode = VIDEO_MODE_PREVIEW;
                ring_dev.app_delay = 10*MAIN_APP_TIME_UNIT_SEC;
                //start_video();
            }

            ret |= ak_thread_create(&id,  video_encode_task, NULL, 32*1024, VIDEO_TASK_PRIO);
            if(g_video_para.mode == VIDEO_MODE_PREVIEW) {
                // start audio thread must befor doorbell_thread,otherwise,may cause ao ai synchron fail
                ret |= ak_thread_create(&id, ao_task, NULL, ANYKA_THREAD_MIN_STACK_SIZE, AO_TASK_PRIO);
                ret |= ak_thread_create(&id, ai_task, NULL, ANYKA_THREAD_MIN_STACK_SIZE, AO_TASK_PRIO);
            }
        }

        // connect to wifi
        ret |= ak_thread_create(&id , (void*)wifi_conn_thread , &sys_cfg, 4096, WIFI_CONN_TASK_PRIO);
    
        // connet to server
        sockaddr.sin_addr.s_addr = sys_cfg.server_ip;
        sockaddr.sin_port = htons(sys_cfg.port_num);
#if 1
        ret |= ak_thread_create(&id, socket_connect_task, (void*)&sockaddr, TCP_SERVER_TASK_SIZE, SOCKET_CONN_TASK_PRIO);
        if(ring_dev.wakeup_src != EVENT_SYS_INIT) {
            ret |= ak_thread_create(&id, tcp_recv_task, NULL, 4*TCP_SERVER_TASK_SIZE, SOCKET_RECV_TASK_PRIO);
        }
#endif


        //if(ring_dev.wakeup_src == EVENT_RING_CALL_WAKEUP) { // net wake also need dingdong
            /*start dingdong thread */
            ret |= ak_thread_create(&doorbell_id, doorbell_thread, NULL, 16*1024, SOUND_PLAY_TASK_PRIO);
        //}

        /*start battery thread */
        ret |= ak_thread_create(&id, battery_thread, NULL, 4*1024, RESEVER_TASK_PRIO);
        if(ret < 0) {
            ak_print_error("create task error \n");
            goto camview_exit;
        }
        
        // use while(): tcp recv thread had wait sem_socket_conn_complete
        //if(ring_dev.wakeup_src == EVENT_SYS_INIT) {
            //ak_thread_sem_wait(&sem_socket_conn_complete);
        //}
        while(g_net_conn_sta <= NET_STA_SOCKET_CHECK) {
            ak_sleep_ms(5);
        }

    }


	while(1)
	{ 
	    //ak_print_normal("app delay %d\n",ring_dev.app_delay);
        //ak_print_normal("app num %d\n",ring_dev.app_num);
	    if(ring_dev.app_delay <= 0) {
            if(0 == ring_dev.app_num){
                ak_print_normal("no app thread,sleep!\n");
                ring_dev.sleep_en = 1;
            }
        } else {
            ring_dev.app_delay--;
        }

        if(ring_dev.sleep_en) {
            app_wifi_sleep((void*)&sockaddr);
            if(send_cmd_sleep() >= 0) {
                // clear flag or may cause sleep repeat
                ring_dev.sleep_en = 0;
                ring_dev.app_num = -1;
            }
        }
		ak_sleep_ms(MAIN_APP_TICK_TIME); // 100
        
	}

camview_exit:
    ak_thread_sem_destroy(&sem_wakeup_src_recved);
    ak_thread_sem_destroy(&sem_video_start);
    ak_thread_sem_destroy(&sem_audio_start);
    ak_thread_sem_destroy(&sem_aec_syc);
    ak_thread_sem_destroy(&sem_wifi_conn_start);  
    ak_thread_sem_destroy(&sem_wifi_conn_complete);  
    ak_thread_sem_destroy(&sem_socket_conn_complete);  
    ak_thread_sem_destroy(&sem_dingdong_start); 
    ak_thread_mutex_destroy(&mutex_ao_dev);
    ak_thread_mutex_destroy(&tcp_socket_mutex);
    ak_print_normal("****camview main exit******\n");
    ak_thread_exit(); 
    return NULL;
}


/*regist module start into _initcall */
static int camview_main_start(void) 
{
	ak_pthread_t id;

#if START_TIME_DEBUG
        unsigned long t = sky_get_time_ms();
        ak_print_warning("camview main start t %lu\n",t);
#endif

    ak_login_g711_decode();
    ak_login_g711_encode();
	ak_login_all_filter();
    
	//cmd_register("camview", camview_main, help_camview);
    ak_thread_create(&id, camview_main, NULL, ANYKA_THREAD_MIN_STACK_SIZE, MAIN_TASK_PRIO);
    //ak_thread_join(id);

	return 0;
}


cmd_module_init(camview_main_start)


