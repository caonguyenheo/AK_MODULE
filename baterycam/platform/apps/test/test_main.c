/******************************************************
 * @brief  test tool for pc IPCTest tool
 
 * @Copyright (C), 2016-2017, Anyka Tech. Co., Ltd.
 
 * @Modification  2017-11-19
*******************************************************/
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
#include "ak_ai.h"
#include "dev_info.h"

#include "wifi.h"

#include "drv_gpio.h"
#include "test_wifi_cfg.h"
#include "ak_drv_key.h"
#include "ak_login.h"
#include "ak_ini.h"
#include "ak_drv_led.h"
#include "kernel.h"

/******************************************************
  *                    Constant         
  ******************************************************/

static const char test_version[] = "test_1.0.00";
static char *help_test[]={
	"test demo for pc IPCtest tool.",
	"usage: testdemo\n"
};
/******************************************************
  *                    Macro         
  ******************************************************/
#define TCP_SERVER_TASK_SIZE  (4096)
#define UDP_PORT 8192
#define TCP_PORT 6789



#define ISP_CFG_PATH       "ISPCFG"
#define INI_PATH "APPINI"

#define VIDEO_VGA_WIDTH				640
#define VIDEO_VGA_HEIGHT			360
#define VIDEO_VGA_HEIGHT_960P		480

#define NORMAL_KEY 47
#define RESET_KEY 50 //TODO 
/*task priority */
#define MAIN_TASK_PRIO 				40
#define VIDEO_TASK_PRIO 			90
#define WIFI_CONN_TASK_PRIO	    	78


#define SOCKET_CONN_TASK_PRIO       78
#define CMD_TASK_PRIO 				80
#define RESEVER_TASK_PRIO           50  // battery and keep alive to stm8

/*max frame len allowed on CC3200*/
#define FRAME_SEND_BUF_LEN          (200*1024)

#define FRAME_HEADER_MAGIC    "FRAM" //0x4652414D  //"FRAM"
#define FRAME_END_MAGIC       "FEND" //0x46454E44  //"FEND"

#define SEND_PACKAGE_OTHER_LEN sizeof(FRAME_HEADER_MAGIC) + 4 + 4 + 4+ 4+ 4+sizeof(FRAME_HEADER_MAGIC)


/*globle flag for video reques*/
// wifi reconnect

#define NET_STA_WIFI_SLEEP          0
#define NET_STA_WIFI_CHECK          1
#define NET_STA_SOCKET_CHECK        2
#define NET_STA_NORMAL              3

#define OTHER_KEY_NUM 47
#define RESET_KEY_NUM 20 // TODO
/******************************************************
  *                    Type Definitions         
  ******************************************************/
/*  frame  type define */
enum frame_type {
    FRAME_TYPE_VIDEO_P = 0,
    FRAME_TYPE_VIDEO_I,
    FRAME_TYPE_AUDIO,
    FRAME_TYPE_PICTURE,
    FRAME_TYPE_TEST_MODE, //测试类型
};

enum data_type 
{
	TEST_MODE_SD = 0,
	TEST_MODE_WIFI,
	TEST_MODE_OTHER_KEY,
	TEST_MODE_RECOVER_KEY,
};

//sd
typedef struct
{
	unsigned int  sd_size_low;
	unsigned int  sd_size_high;
}T_SD_TEST_RESULT;

 //wifi
typedef struct
{
	char  wifi_name[32];
	unsigned int  wifi_quality;
}T_WIFI_INFO;
 
typedef struct
{
	unsigned int  wifi_num;
	T_WIFI_INFO  *wifi_info;
}T_WIFI_TEST_RESULT;


//测试结果信息
typedef struct
{
	char  test_mode;
	char  test_result;
	char  rev1;
	char  rev2;
	unsigned int buf_len;
	char  *buf;
}T_TEST_RESULT;

// audio send
typedef struct audio_send_ctrl {
    int  cnt; 
    char data[4096+16];
}audio_send_ctrl_t;

typedef struct _sock_cfg{
  unsigned long  ipaddr;
  unsigned short port;
  int 			 sock_id;
}sock_cfg_t;

struct netsend_arg
{
	sock_cfg_t       sock;
    unsigned long    count; 
	char             string[128];
};

typedef struct test_result
{
 	int status_wifi;
	int status_sd;
	int status_led;
	int status_key;
	int status_tcp;
	char ip[20]; //OK
	char mac[20];//OK
	char version[20]; //OK
	
}T_TEST_RESULT_SELF;

typedef struct
{
	short len;		
	char commad_type;      
	char *data_param;
	short check_sum;
}T_PACK_INFO;

enum
{
     TYPE_TEST_FINISH = 1,
}E_TEST_CMD_TYPE;

struct  venc_param
{
    int width;
    int height;
    int fps;
    int kbps;
};

struct  video_param
{
    unsigned char  mode;                    // 0:preview 1:photo
    
    unsigned char  resolution;              // 0:vga 1:720p
    unsigned char  vi_use_ch;               // 0: main 1:sub
    unsigned char  fps;
    unsigned short kbps;
    
    unsigned short exp_time;
    
};

/*video stream define*/
typedef struct _stream_header {
	unsigned int header_magic;
    unsigned int frameType;
    unsigned int time;
    unsigned int id;
	unsigned int frame_len;
} VIDEO_STREAM_HEADER,AUDIO_STREAM_HEADER;

/*video stream define*/
typedef struct _stream_end {
	unsigned int frame_crc;
	unsigned int end_magic;
}  VIDEO_STREAM_END,AUDIO_STREAM_END;

/******************************************************
  *                    Global Variables         
  ******************************************************/
static audio_send_ctrl_t audioSend; 

// tcp server
static int             g_tcp_socket_id;

static T_TEST_RESULT_SELF result;

//static volatile int g_net_conn_sta = NET_STA_WIFI_CHECK;
static ak_sem_t  sem_wifi_conn_start;
static ak_sem_t  sem_wifi_conn_tcp;
static ak_sem_t sem_wifi_conn_udp;
static ak_sem_t  sem_socket_conn_complete;


// preview control
//static int       video_run_flag = 0;        // 0:stop 1:run
//static ak_sem_t  sem_dingdong_start;
static ak_sem_t  sem_video_start;           // 
static ak_sem_t  sem_video_end;           // 
static ak_sem_t  sem_sd;     // had recved wakeup source
static ak_sem_t sem_sd_result_ok;
static ak_mutex_t tcp_socket_mutex;         // tcp socket mutex

// audio semaphore
static ak_sem_t sem_audio_start;            // start audio
static ak_sem_t sem_aec_syc;                // use to aec synchron
//static ak_mutex_t mutex_ao_dev;

static struct net_sys_cfg g_wifi_info;

static struct video_param g_video_para = {
    .resolution =  1,                       // may change by mode 0:preview 1:photo
    .fps = 12,
    .kbps = 500,
    .exp_time = 859
};


static struct venc_param g_rec_para;            // 视频设置中间参数
static struct video_resolution g_main_ch_res;
static struct video_resolution g_sub_ch_res;
static T_SD_TEST_RESULT sd_result;

/******************************************************
*               Function prototype                           
******************************************************/
static int tcp_send_data(char *data, int data_len);
static int recv_data(char *buff, int buff_len);
static int test_end(void);

/******************************************************
*               Function Declarations
******************************************************/

/**
  * init upd or tcp server.
  * @param type :SOCK_STREAM or SOCK_DGRAM
  * @param addr :
  * @param alen :
  * @param qlen :
  * @return socked_id or -1 in failed
  */
static int init_server(int type, const struct sockaddr *addr,
		socklen_t alen, int qlen)
{

	int fd;
	int reuse = 1;
	ak_thread_mutex_lock(&tcp_socket_mutex);
	
	if((fd = socket(addr->sa_family, type, 0)) < 0)
	{
		ak_print_error("create socket error.\n");
		return AK_FAILED;
	}
	ak_print_normal("create socket(%d) success.\n",fd);

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
			&reuse, sizeof(reuse)) < 0)
	{
		ak_print_error("set socket opt err.\n");
		goto errout;
	}
	ak_print_normal("set socket(%d) socket opt  success.\n",fd);

	if(bind(fd, addr, alen) < 0)
	{
		ak_print_error("socket bind err.");
		goto errout;		
	}
	ak_print_normal("set socket(%d) bind  success.\n",fd);
	if( type == SOCK_STREAM)
	{
		if(listen(fd, qlen) < 0)
		{
			ak_print_error("socket listen err.");
			goto errout;
		}
		ak_print_normal("set socket(%d) listen  success.\n",fd);
	}

	ak_thread_mutex_unlock(&tcp_socket_mutex);
	return fd;
errout:
	close(fd);
	ak_thread_mutex_unlock(&tcp_socket_mutex);
	return -1;
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

/*
包格式:
包头标识 | 帧类型 | 时间戳 | 帧id | 帧长度 |   数据   | CRC校验 | 包尾标识
4Bytes     4Bytes   4Bytes   4Bytes 4Bytes    4Bytes    4Bytes    4Bytes
*/
static void packet_result_data(char *send_buf, int type, int ts, int id, T_TEST_RESULT * result)
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

	int result_buf_len = sizeof(T_TEST_RESULT) + result->buf_len;
	if(result->buf_len != 0)
		--result_buf_len;
    memcpy(opt_ptr, &result_buf_len, 4);
    opt_ptr += 4;

	
    memcpy(opt_ptr, result, sizeof(T_TEST_RESULT));
    opt_ptr += sizeof(T_TEST_RESULT);

	if(result->buf_len != 0)
	{
		opt_ptr -= sizeof(result->buf);
		memcpy(opt_ptr, result->buf, result->buf_len);

		opt_ptr += result->buf_len;
	}

	check_sum++;
    memcpy(opt_ptr, &check_sum, 4);
    opt_ptr += 4;
    memcpy(opt_ptr, FRAME_END_MAGIC, 4);


}


/*
*@brief:  tcp_send_data tcp send data to IPCTest tool
*@param: data
*@param: data_len
*@return; send len
*/
static int tcp_send_data(char *data, int data_len)
{
	int sendlen = 0;
    int cnt = 0;
    static int error_cnt = 0;

    ak_thread_mutex_lock(&tcp_socket_mutex);
    //if(g_net_conn_sta == NET_STA_NORMAL) 
	{
	    sendlen = send(g_tcp_socket_id, data, data_len, MSG_WAITALL);
        //ak_print_normal("tcp %d\n",sendlen);

        if (sendlen < 0)
    	{   
    	    error_cnt++;
            if(sendlen == ERR_TIMEOUT) {
                ak_print_warning("send tcp data timeout\n");
            } else {
    		    ak_print_error("send data from socket error %d\n", sendlen);
				result.status_tcp = 0;
            }
           // if(g_net_conn_sta != NET_STA_WIFI_CHECK && error_cnt > 5) {
			if(error_cnt > 5) {
                //g_net_conn_sta = NET_STA_WIFI_CHECK;
                ak_print_normal("net reconn...\n");
                ak_thread_sem_post(&sem_wifi_conn_start);
            }
    	} else {
            error_cnt = 0;
			result.status_tcp = 1;
        }
    }   
    ak_thread_mutex_unlock(&tcp_socket_mutex);

	return sendlen;

}


/*
*@brief:  prepare audio data and send audio data to IPCtest tool
*@time: audio frame ts
*@param data :audio data
*@param  data_len: data length
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
        ret = tcp_send_data(&audioSend.data[0],audioSend.cnt + sizeof(AUDIO_STREAM_HEADER)+8);
        audioSend.cnt = 0;
    }
#endif

	//ak_print_normal("ymx audio send len : %d.\n",audioSend.cnt);
    return ret;
}


/*
*@brief:  recv_data, tcp recv data
*@param buff[out]
*@param: buff_len[out]
*@return; recv len
*/
static int recv_data(char *buff, int buff_len)
{
	int len;
	
	len = recv(g_tcp_socket_id, buff, buff_len, MSG_WAITALL);
	return len;
	
}

/**
  *@brief encode handle init
  *
  *@param type: encode type
  *@return encode handle
  */
static void *video_encode_initial(enum encode_output_type type)
{
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
	param->enc_out_type = type;
	void * encode_video = ak_venc_open(param);
	free(param);

	return encode_video;
}

/**
  *@brief video handle init
  *
  *@return video handle
  */
static void *camera_initial(void)
{

	/* open device */
	void *handle = ak_vi_open(VIDEO_DEV0);
	if (handle == NULL) {
		ak_print_error_ex("vi open failed\n");
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
	{
		ak_print_error_ex("set channel attribute failed\n");
		return NULL;
	}

	/* get crop */
	struct video_channel_attr cur_attr ;
	if (ak_vi_get_channel_attr(handle, &cur_attr)) {
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
	
	/* don't set camera fps in demo */
	ak_vi_set_fps(handle, g_rec_para.fps); // change to set venc fps
	ak_print_normal("capture fps: %d\n", ak_vi_get_fps(handle));

	return handle;
}

/**
  *@brief video init
  *
  *@return video handle
  */
void *video_init(void)
{
    unsigned long t;
    void *vi_handle;

    /* match sensor*/
    if (ak_vi_match_sensor(ISP_CFG_PATH) < 0)
    {
        ak_print_error_ex("match sensor failed\n");
		return NULL;
    }
 
	g_rec_para.width  = 1280; 
	g_rec_para.height= 720;
	g_rec_para.fps = 12;
	g_rec_para.kbps = 512;

    /* vi handle init */
    vi_handle = camera_initial();
    if (vi_handle == NULL) {
        ak_print_error_ex("video input init faild, exit\n");
    }
 
    return vi_handle;
}

/**
  *@brief send wifi test result to IPCTest tool
  *
  *@return send length
  */
int send_wifi_result(void)
{
	T_TEST_RESULT wifi_result_send;
	char send_buf[100];

	wifi_result_send.test_mode = TEST_MODE_WIFI;
	wifi_result_send.test_result = result.status_wifi;
	wifi_result_send.buf_len = 0;

	packet_result_data(send_buf, FRAME_TYPE_TEST_MODE, 0, 0, &wifi_result_send);
	return tcp_send_data(send_buf, sizeof(wifi_result_send) + SEND_PACKAGE_OTHER_LEN);
	
}

/**
  *@brief send sd test result to IPCTest tool
  *
  *@param sd_result: sd size
  *@return send length
  */
int send_sd_result(T_SD_TEST_RESULT *sd_result)
{
	T_TEST_RESULT sd_result_send;
	T_TEST_RESULT wifi_result_send;
	char send_buf[100];
	int data_len = 0;

	sd_result_send.test_mode = TEST_MODE_SD;
	sd_result_send.test_result = result.status_sd;
	sd_result_send.buf_len = sizeof(T_SD_TEST_RESULT);
	sd_result_send.buf = (char*)sd_result;

	packet_result_data(send_buf, 
		FRAME_TYPE_TEST_MODE, 0, 0, 
		&sd_result_send);

	data_len = sizeof(sd_result_send)
				+ sd_result_send.buf_len-1
				+ SEND_PACKAGE_OTHER_LEN;
 	ak_print_notice("sd_data:0x%x,0x%x,0x%x,0x%x \n 0x%x,0x%x,0x%x,0x%x\n 0x%x,0x%x,0x%x,0x%x\n 0x%x,0x%x,0x%x,0x%x\n",
		send_buf[0],send_buf[1],send_buf[2],send_buf[3],send_buf[4]
		,send_buf[5],send_buf[6],send_buf[7],send_buf[8],send_buf[9]
		,send_buf[10],send_buf[11],send_buf[12],send_buf[13],send_buf[14],send_buf[15]);
 	return tcp_send_data(send_buf, data_len);

	
}

/**
  *@brief send wifi  and sd result to IPCTest tool
  */
void send_result_to_pc(void)
{
		ak_print_notice("send wifi result to pc.\n");
		send_wifi_result();
		ak_print_notice("send wifi result to pc ok.\n");

		ak_thread_sem_post(&sem_sd);
		ak_thread_sem_wait(&sem_sd_result_ok);
		ak_print_notice("sd result ok get sem.\n");
		send_sd_result(&sd_result);
}

/**
  *@brief keep wifi connection thread
  */
static void* wifi_connect_thread(void *args)
{

	int wifi_connect_counts = 5;
    struct net_sys_cfg *p_cfg = (struct net_sys_cfg*)&g_wifi_info;

	wifi_set_mode(WIFI_MODE_STA);
	while(1)
	{
		ak_print_notice("wifi wait sem\n");
        ak_thread_sem_wait(&sem_wifi_conn_start);
		ak_print_notice("wifi get sem\n");
        while(1) {
            if (!wifi_isconnected()) // not connected
            {   
                ak_print_normal("Connect to SSID: < %s >  with password:%s\n", p_cfg->ssid,  p_cfg->key);
                wifi_connect(p_cfg->ssid, p_cfg->key);   // wifi_connect需要时间，下面的if不一定成立
                if (wifi_isconnected())     
                {
                    ak_print_normal("wifi connect ok\n");

                    int ret = wifistation_netif_init();
                    if(0 == ret) {
							struct ip_info wifi_ip_info;

						/* get ip result */
						memset(&wifi_ip_info, 0, sizeof(struct ip_info));
						test_wifi_get_ip(&wifi_ip_info);
						if(wifi_ip_info.ipaddr.addr != 0 
								&& wifi_ip_info.netmask.addr != 0 
								&& wifi_ip_info.gw.addr != 0)
							sprintf(result.ip,"%u.%u.%u.%u",
									wifi_ip_info.ipaddr.addr & 0xff,
									(wifi_ip_info.ipaddr.addr & 0xff00)>>8,
									(wifi_ip_info.ipaddr.addr &0xff0000) >>16,
									wifi_ip_info.ipaddr.addr>>24);

						ak_print_notice("ip:%s\n",result.ip);

						/* get mac result */
						unsigned char mac[6] = {0,0,0,0,0,0};
						wifi_get_mac(mac);
						sprintf(result.mac,"%x:%x:%x:%x:%x:%x"
								, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

						ak_print_notice("mac:%s\n",result.mac);
						
                        //g_net_conn_sta = NET_STA_SOCKET_CHECK;
                        ak_thread_sem_post(&sem_wifi_conn_udp);
						ak_thread_sem_post(&sem_wifi_conn_tcp);
						result.status_wifi = 1;
                        break;
                    }
					result.status_wifi = 0;
                } else {
                    ak_print_normal("wifi connect++ \n");
                }
            }
			
            else {
                ak_print_normal("wifi is connected\n");
                result.status_wifi = 1;
				ak_thread_sem_post(&sem_wifi_conn_udp);
				ak_thread_sem_post(&sem_wifi_conn_tcp);
				break;
            }
            ak_sleep_ms(100);
        }

	}
ERR:	
	ak_print_normal("sta conn thread exit.\n");
	ak_thread_exit();
}


/*
*@brief:  udp_server_thread, for IPCTest broadcast command
*			and send ip, mac, version to IPCTest tool
*@return; 
*/
static void* udp_server_thread(void *args)
{
	int t;
    int ret;
    int socket_s = -1;
	int err = -1;
	int recv_bytes;
	int send_bytes;
	char recv_buf[128];
	char send_buf[128] ;
	struct sockaddr remote_addr;
	struct sockaddr_in recv_addr ;//= (struct sockaddr_in *)args;
	socklen_t retval;
	struct netsend_arg *ns_arg;
	char * flgs = "Anyka IPC ,Get IP Address!";
	
	//recv_addr.sin_len = sizeof(struct sockaddr_in);


	memset(&recv_addr, 0x0, sizeof(struct sockaddr_in));
	
	recv_addr.sin_family = AF_INET;
	recv_addr.sin_port = htons(UDP_PORT);
	recv_addr.sin_addr.s_addr = INADDR_ANY;


	if( (socket_s = init_server(SOCK_DGRAM, 
			(struct sockaddr*) &recv_addr, 
			sizeof(struct sockaddr_in), 0)) < 0 )
	{
		ak_print_error("create udp server error!");
		goto ERR;
	}
WAIT_WIFI_OK:
	ak_print_notice("udp wait wifi.\n");
	ak_thread_sem_wait(&sem_wifi_conn_udp);
	ak_print_notice("udp get wifi ok.\n");
    while (1) {

		ak_print_normal("udp wait cmd.\n");
		recv_bytes = recvfrom(socket_s, recv_buf, sizeof(recv_buf) - 1, MSG_WAITALL, &remote_addr, (socklen_t*) &retval);
		if (recv_bytes > 0)
		{

			/*
			 * compare the flag to make sure that
			 * the broadcast package was sent from our software \
			 */
			ak_print_normal("udp:recv data=[%s] len=%d.\n", recv_buf, recv_bytes);
			if (strncmp(recv_buf, flgs, strlen(flgs)) != 0) {
				ak_print_normal_ex("recv data is not match\n");
				memset(recv_buf, 0x0, sizeof(recv_buf));
				continue;
			}
			
			recv_buf[recv_bytes]=0;
			ak_print_normal("udp:recv data=[%s] len=%d.\n", recv_buf, recv_bytes);
			{
				sprintf(send_buf,"@%s@1@2@3@4@5@6@%s@%s@9@10",
						result.ip,
						result.version,
						result.mac);

				ak_print_normal("udp:send data=[%s] len=%d.\n", send_buf, strlen(send_buf));
				int send_len = sendto(socket_s, send_buf, strlen(send_buf), 0, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
				if (send_len <= 0)
				{
					ak_print_error("send error\n");
					ak_thread_sem_post(&sem_wifi_conn_start);
					goto WAIT_WIFI_OK;
				}

				ak_print_normal("%ld bytes data sent success, %d times send error\n", send_len);
			}

    	}
    }
ERR:
	return NULL;
}

/*
*@brief:  tcp_server_thread prepare socket for send test result
*@return;
*/
static void* tcp_server_thread(void *args)
{
    unsigned int  t;
    int socket_s = -1;
	int err = -1;
	char recv_buf[1024+32] = {0};
	int len;
	unsigned int frameType;
	unsigned short cmd_id;
	unsigned short cmd_arg_len;

	/** initialize and fill in the message to the sfdet addr infor struct **/
	struct sockaddr_in server_addr;	/** server socket addr information struct **/
	memset(&server_addr, 0, sizeof(struct sockaddr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(TCP_PORT);
	server_addr.sin_addr.s_addr =INADDR_ANY;

	if( (socket_s = init_server(SOCK_STREAM, (struct sockaddr*) &server_addr, 
			sizeof(struct sockaddr_in), 1)) < 0 )
	{
		ak_print_error("create tcp server error!");
		goto ERR;
	}
	struct sockaddr_in peer_addr;		/** server socket addr information struct **/

WAIT_WIFI_OK:
	ak_print_notice("tcp init wait wifi...\n");
	ak_thread_sem_wait(&sem_wifi_conn_tcp);
	ak_print_notice("tcp init...\n");
	while(1)
	{
		ak_print_notice_ex("waiting for connect ...\n");

		int addr_len = sizeof(struct sockaddr_in);
		memset(&peer_addr, 0, sizeof(struct sockaddr));

		
		/** accept the client to connect **/
		g_tcp_socket_id = accept(socket_s, (struct sockaddr *)&peer_addr, (socklen_t *)&addr_len);
		if (g_tcp_socket_id < 0) {
			close(socket_s);
			//exit_err("accept");
			ak_print_error_ex("accept error\n");

			return NULL;
		}
		ak_print_normal("Client connected, socket cfd = %d\n", g_tcp_socket_id);
		ak_print_normal("free:%d\n", Fwl_GetRemainRamSize());

		ak_thread_sem_wait(&sem_video_end);
		ak_thread_sem_post(&sem_video_start);
		ak_thread_sem_post(&sem_audio_start);

		result.status_tcp = 1;

		send_result_to_pc();

		while(1)
		{
			ak_print_normal("tcp wait cmd\n");
            len = recv_data(recv_buf, sizeof(recv_buf));
            if(len > 0)
            {

                ak_print_normal("tcp rec data,%s,len:%d\n",recv_buf,len);
				char *tmp_recv_buf = recv_buf;
				T_PACK_INFO* result = (T_PACK_INFO*)tmp_recv_buf;
				int i = 0;
				ak_print_normal("ymx:%c,%c,%c,%c, == %c,%c,%c\n",
					recv_buf[0],
					recv_buf[1],
					recv_buf[3],
					recv_buf[4],
					recv_buf[5],
					recv_buf[6],
					recv_buf[7]);
				ak_print_normal("ymx:%x,%d\n",recv_buf[2],result->commad_type);
				if(result->commad_type == TYPE_TEST_FINISH)
				{
					test_end();
					ak_print_notice("test finish.\n");
				}
            }
            else
            {
                ak_print_error("recv data from tcp socket error\n");
				ak_thread_sem_post(&sem_wifi_conn_start);
				//video_run_flag = 0;
				result.status_tcp = 0;
				close (g_tcp_socket_id);
                goto WAIT_WIFI_OK;
            }
        }
	}
	g_tcp_socket_id = socket_s;
ERR:	
	return NULL;
}

/**
  *@brief start a thread to encode and send video
  */
static void *test_video_thread(void *arg)
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
    int send_len = 0;
    

	ak_print_normal("create a encode task\n");

    vi_handle = video_init();
    if (vi_handle == NULL) {
        ak_thread_exit();
        return NULL;
    }

    venc_handle = video_encode_initial(H264_ENC_TYPE);
    if (venc_handle == NULL) {
		ak_print_error_ex("video encode open failed!\n");
        ak_vi_capture_off(vi_handle);
        ak_vi_close(vi_handle);
        ak_thread_exit();
        return NULL;
    }

	send_buf = malloc(FRAME_SEND_BUF_LEN);
    if(send_buf == NULL)
    {
        ak_print_error("malloc buff for sending frame error\n");
        ak_venc_close(venc_handle);
        ak_vi_close(vi_handle);
        ak_thread_exit();
        return NULL;
    }
	
	ak_vi_capture_on(vi_handle);
	
WAIT_VIDEO_SEM:
	// wait video start sem
	ak_print_notice("video waiting tcp...\n");
    if(ak_thread_sem_wait(&sem_video_start))
		goto WAIT_VIDEO_SEM;

	ak_print_notice("video get sem.\n");
	
    stream_handle = ak_venc_request_stream(vi_handle,venc_handle);
    if (stream_handle == NULL) {
	    ak_print_error_ex("request stream failed\n");
        ak_venc_close(venc_handle);
        ak_vi_capture_off(vi_handle);
        ak_vi_close(vi_handle);
        ak_thread_exit();
        return NULL;
    }

	struct video_stream stream = {0};
    struct video_stream *p_stream = &stream;

	while(1){
            
            ret = ak_venc_get_stream(stream_handle, &stream);
            if(ret != 0) {
                ak_venc_release_stream(stream_handle, &stream);
                ak_sleep_ms(20);
                // ak_print_warning("88\n");
                continue;
            }

           // p_stream = &stream;
#if 1
            // frame packet
            packet_data_frame(send_buf,stream.frame_type,stream.ts,id,stream.len,stream.data);

            // frame send
            send_len = tcp_send_data(send_buf, stream.len + sizeof(VIDEO_STREAM_HEADER) + sizeof(VIDEO_STREAM_END));
            if(send_len < 0)
            {
                ak_print_error("send video frame error\n");
				break;
            }
			
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
	ak_thread_sem_post(&sem_video_end);
	goto WAIT_VIDEO_SEM;
exit_venc:
        ak_venc_close(venc_handle);

exit_vi:	
    ak_print_normal("video task exit \n");
	ak_vi_capture_off(vi_handle);
	ak_vi_close(vi_handle);
	ak_thread_exit();
	free(send_buf);
	send_buf = NULL;
	return NULL;
}

/**
  *@brief start a thread to get and send audio
  */
//unsigned char *readpcmBuf = NULL;
static void* test_ai_thread(void* param)
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

	//g_rec_end = 0;
	//ak_print_normal("wait ai open...\n");
	
	void *adc_drv = ak_ai_open(&input);
	
	/*
		AEC_FRAME_SIZE	is	256，in order to prevent droping data,
		it is needed  (frame_size/AEC_FRAME_SIZE = 0), at  the same 
		time, it  need	think of DAC  L2buf  , so  frame_interval  should  
		be	set  32ms。
	*/

	if(adc_drv == NULL)
		ak_print_error("ai open error.\n");
	else
		ak_print_normal("ai open success.\n");

	if(ak_ai_set_source(adc_drv, AI_SOURCE_MIC)< 0)
	{
		ak_print_error("MIC open error.\n");
	}
	else
		ak_print_normal("MIC open success.\n");
	
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
WAIT_AUDIO_SEM:
    ak_print_notice("ai waiting test tool...\n");
    ak_thread_sem_wait(&sem_audio_start);

	ak_ai_set_aec(adc_drv, 1);
    ak_print_normal("ai open ...\n");
	//ak_thread_sem_post(&sem_aec_syc);
	while(1)
	{
        
		ai_ret = ak_ai_get_frame(adc_drv, &frame, 1);       
		if(ai_ret == -1)
		{
			ak_sleep_ms(5);
			continue;							
		}
		//memcpy(&aecBuf[aec_index], frame.data, frame.len);


        if(frame.len > 0) {
            if(send_audio_data((long)frame.ts,&id,frame.len,frame.data)< 0)
            {
                ak_print_error("send audio frame to pc error\n");
				goto WAIT_AUDIO_SEM;
            }
        }
  
		ak_ai_release_frame(adc_drv, &frame);		
	}

ai_close:
	ak_ai_close(adc_drv);
	adc_drv = NULL;
	ak_print_normal("ai task exit !\n");
	ak_thread_exit();

}

/**
  *@brief prepare for key test
  */
static void* test_keypad_thread(void *arg)
{

	void * key = NULL;
	struct key_event event = {0};	
	static int press_count = 0;
	T_TEST_RESULT key_result_send;
	char send_buf[100];

	key = ak_drv_key_open();
	if(NULL == key)
	{	
		ak_print_error("key open fail!\n\n\n\n");
		goto ERR;
	}

	while(1)
	{
		
		if (ak_drv_key_get_event(key,&event,-1) == 0)
		{	
			if(event.stat == 0)
			ak_print_normal("key_id = %d  PRESS\n",event.code);
			else if(event.stat == 2)
			ak_print_normal("key_id = %d  RELEASE\n",event.code);

			if(event.code == OTHER_KEY_NUM)
				key_result_send.test_mode = TEST_MODE_OTHER_KEY;
			else if(event.code == RESET_KEY_NUM)
				key_result_send.test_mode = TEST_MODE_RECOVER_KEY;
			key_result_send.test_result = 1;
			key_result_send.buf_len = sizeof(press_count);
			key_result_send.buf = &press_count;

			packet_result_data(send_buf, 
				FRAME_TYPE_TEST_MODE, 0, 0, 
				&key_result_send);
			ak_print_notice("keypad send result to pc\n",event.code);
			tcp_send_data(send_buf, sizeof(key_result_send) + key_result_send.buf_len+SEND_PACKAGE_OTHER_LEN);
			ak_print_notice("keypad send result to pc ok\n",event.code);
		}	
	}
	ak_drv_key_close(key);
ERR:	
	ak_print_normal("keypad test end.\n");
	ak_thread_exit();
}

/**
  *@brief led test
  */
static void* test_led_thread(void *arg)
{

	void *f_led = ak_drv_led_open(0);
	if(f_led == NULL)
	{
		result.status_led = 0;
		ak_print_error("open led fail\n");
		return NULL;
	}

	result.status_led = 1;
	ak_drv_led_blink(f_led, 100,100);


ERR:	
	ak_print_normal("LED test end.\n");
	ak_thread_exit();
}

/**
  *@brief sd test
  */
static void* test_sd_thread(void *arg)
{
	unsigned int sd_size_high = 0;
	unsigned long sd_size_low = 0;

	char send_buf[100];
	int ret = 0;
TEST_START:
	ak_print_notice("sd wait sem...\n");
	ak_thread_sem_wait(&sem_sd);
	ak_print_notice("sd get sem.\n");
	if(AK_SUCCESS == ak_mount_fs(DEV_MMCBLOCK, 0, ""))
	{
		result.status_sd = 1;
		sd_size_low = FS_GetDriverCapacity(DEV_MTDBLOCK, &sd_size_high);
		sd_result.sd_size_low= sd_size_low;
		sd_result.sd_size_high= sd_size_high;
		
		ak_print_normal("sd: high:%d, low:%u.\n",sd_size_high,sd_size_low);

	}
	else
	{
		result.status_sd = 0;
		ak_print_normal("sd: high:%d, low:%d.\n",sd_size_high,sd_size_low);
	}

	ak_print_notice("sd test over.\n");
	ak_thread_sem_post(&sem_sd_result_ok);
	goto TEST_START;
ERR:	
	ak_print_normal("sd test end.\n");
	ak_thread_exit();
}

/*
*main process to start up
*/
void* test_main(void *arg)
{

	int ret;
	ak_pthread_t wifi_p_id,id;
    struct video_resolution resolution = {0};
	
	memset(&result, 0, sizeof(T_TEST_RESULT));

 	/*init semaphore */
	ret |= ak_thread_sem_init(&sem_video_start, 0);
	ret |= ak_thread_sem_init(&sem_video_end, 1);
	ret |= ak_thread_sem_init(&sem_audio_start,0);
	ret |= ak_thread_sem_init(&sem_wifi_conn_start, 1);     // start wifi conn at first 
	ret |= ak_thread_sem_init(&sem_wifi_conn_udp, 0);
	ret |= ak_thread_sem_init(&sem_wifi_conn_tcp, 0);
	ret |= ak_thread_sem_init(&sem_sd, 0);
	ret |= ak_thread_sem_init(&sem_sd_result_ok, 0);
	ret |= ak_thread_mutex_init(&tcp_socket_mutex);

    if(ret < 0)
	{
		ak_print_error("init sem error \n");
		goto camview_exit;
	}

#ifndef WIFI_INIT_IN_KERNEL
	/* init wifi */
	if(test_wifi_init()== AK_FAILED)
	{
		ak_print_error("wifi init error.\n");
		goto START_TEST_NO_NEED_WIFI;
	}
		
#endif

	ak_print_normal("%s\n",test_version);
	/* get version result */
	memcpy(result.version, test_version, strlen(test_version));

	ak_print_normal("wifi:ssid %s,password %s\n", 
			g_wifi_info.ssid, g_wifi_info.key);

    /* setp 2: init WIFI */
    ak_thread_create(&wifi_p_id , (void*)wifi_connect_thread, NULL, 4096, WIFI_CONN_TASK_PRIO);
	ak_thread_create(&id, udp_server_thread, NULL, 4*TCP_SERVER_TASK_SIZE, SOCKET_CONN_TASK_PRIO);
    ak_thread_create(&id, tcp_server_thread, NULL, TCP_SERVER_TASK_SIZE, SOCKET_CONN_TASK_PRIO);

	/* setp 2: prepare video and audio */
    ak_thread_create(&id, test_video_thread, NULL, 32*1024, VIDEO_TASK_PRIO);
	ak_thread_create(&id, test_ai_thread, NULL, ANYKA_THREAD_MIN_STACK_SIZE, 85);
  
START_TEST_NO_NEED_WIFI:

	ak_sleep_ms(100);
	/*test led */
	ak_thread_create(&id, test_led_thread, NULL, TCP_SERVER_TASK_SIZE, 50);

	/*test key */
	ak_thread_create(&id, test_keypad_thread, NULL, TCP_SERVER_TASK_SIZE, 80);

	/*test sd */
	ak_thread_create(&id, test_sd_thread, NULL, TCP_SERVER_TASK_SIZE, 70);

	ak_thread_join(wifi_p_id);
camview_exit:
   
    ak_print_normal("****camview main exit******\n");

 	ak_thread_sem_destroy(&sem_video_start);
 	ak_thread_sem_destroy(&sem_video_end);
    ak_thread_sem_destroy(&sem_audio_start);
    ak_thread_sem_destroy(&sem_wifi_conn_start);  
	ak_thread_sem_destroy(&sem_wifi_conn_tcp);  
	ak_thread_sem_destroy(&sem_wifi_conn_udp);
	ak_thread_sem_destroy(&sem_sd);
	ak_thread_sem_destroy(&sem_sd_result_ok);
	ak_thread_mutex_destroy(&tcp_socket_mutex);
	ak_thread_exit(); 
    return NULL;
}

static int test_end(void)
{
	
	void* fd;
	char* title = "test";

	// get test mode
	if( (fd = ak_ini_init(INI_PATH)) == NULL)
		return 0;

	if( ak_ini_del_title(fd, title) < 0 )
		return 0;

	ak_ini_destroy(fd);
	return 1;
		
}

static int is_test_mode(void)
{
	
	void* fd;
	char* title = "test";
	char* item = "test_mode";
	char item_value[2] ={0};
	char need_test[] = "0";
	// get test mode
	if( (fd = ak_ini_init(INI_PATH)) == NULL)
		return 0;

	if( ak_ini_get_item_value(fd, title, item, &item_value) < 0
			|| strncmp(need_test, item_value, sizeof(need_test)) != 0 )
		return 0;

	ak_ini_destroy(fd);
	return 1;
		
}

/*regist module start into _initcall */
void* test_main_start(int argc, char **args)
//static int test_main_start(void) 
{

	if(!is_test_mode()){
		return NULL;
	}

	/*  is test mode */
	/* get wifi config */
	test_wifi_get_config(&g_wifi_info);
	
	// is test mode, start to test
	ak_pthread_t id;

    ak_login_g711_decode();
    ak_login_g711_encode();
	ak_login_all_filter();
    
    ak_thread_create(&id, test_main, NULL, ANYKA_THREAD_MIN_STACK_SIZE, MAIN_TASK_PRIO);
    ak_thread_join(id);

	return 0;
}

/*regist module start into _initcall */
static int test_cmd_register(void) 
{

	cmd_register("testdemo", test_main_start, help_test);
	
	return 0;
}
cmd_module_init(test_cmd_register)


