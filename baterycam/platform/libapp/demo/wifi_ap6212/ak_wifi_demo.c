/*
*wifi_demo.c    wifi operation demo
*/
#include "stdint.h"
#include "wifi.h"
#include "command.h"
#include "ak_common.h"
#include "ak_thread.h"
#include "akos_error.h"

#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"

#include "dev_info.h"
#include "drv_gpio.h"

static char *help_ap[]={
	"wifi ap tools",
	"usage:ap start [ch] [ssid] <password>\n"
	"      ap stop\n"
};
static char *help_sta[]={
	"wifi sta tools",
	"usage: sta conn [ssid] <password>\n"
	"       sta disconn\n"
	"		sta init\n"
};

static char *help_scan[]={
	"wifi scan",
	"usage: scan\n"
};

static char *help_tcpka[]={
	"wifi tcp keep alive",
	"usage:\n"\ 
	"tcpka on ip port\n"\
	"tcpka off\n"
	
};

static char *help_wowl[]={
	"wifi wowl command",
	"usage: \n"\
			"wowl add,  add wakeup filter default is 0x983B16F8F39C\n"\
	        "wowl del,  del wakup filter\n" \
	        "wowl on,   sleep on\n" \
			"wowl off   sleep off\n"\
			"wowl dtim [n], set dtim value \n"
            "wowl test, test thread resume\n"
};

static char *help_sdio[]={
	"sdio back test",
	"usage: \n"\
	    "sdio bk, backup\n"\
        "sdio res, restore\n"			
};

static char *help_es[]={
	"wifi easy setup",
	"usage: es cmds\n"
};


typedef struct tcpka_timer {
	uint16_t interval;		    /* interval timer */
	uint16_t retry_interval;	/* retry_interval timer */
	uint16_t retry_count;		/* retry_count */
} tcpka_timer_t;

typedef struct {
    uint8_t len;     /**< SSID length */
    uint8_t val[32]; /**< SSID name (AP name)  */
} ssid_t;

typedef struct {
    uint8_t octet[6]; /**< Unique 6-byte MAC address */
} mac_t;

typedef struct
{
	ssid_t SSID;
	mac_t  BSSID;
	uint8_t channel;
	uint8_t security;
	uint8_t security_key_length;
	char security_key[64];
} es_ap_info_t;

struct _conn_param
{
	unsigned char essid[MAX_SSID_LEN];
	unsigned char password[MAX_KEY_LEN];
};

int sta_reconn_flag = AK_TRUE;
ak_pthread_t  g_sta_conn_thread_id = AK_INVALID_TASK;
struct _conn_param conn_param;
bool flash_value(char mode, char *data, int len);

/*keep connection thread*/
static void sta_conn_thread(void *args)
{
	struct _conn_param *conn_param = (struct _conn_param*)args;
	
	while(sta_reconn_flag == AK_TRUE)
	{
		if (!wifi_isconnected())
		{	
			ak_print_normal("Connect to SSID: < %s >  with password:%s\n", conn_param->essid,  conn_param->password);
			wifi_connect(conn_param->essid, conn_param->password);
			if (wifi_isconnected())
			{
				ak_print_normal("wifi connect ok\n");
				wifi_power_cfg(0);
				wifistation_netif_init();
			}
		}
		ak_sleep_ms(1000);
	}
	ak_print_normal("sta conn thread exit.\n");
	ak_thread_exit();
}


/**
 *测试WIFI连接,
 *连接到指定名字的路由器，加密模式和频道自动适应
 *密码长度在WPA或WPA2模式下8 <= len <= 64;在WEP模式下必须为5或13
 */
static void cmd_wifi_sta(int argc, char **args)
{
	char *essid = args[1];
	char *password =args[2];

	if (strcmp(args[0], "conn") == 0)
	{
		if (argc != 2 && argc != 3)
		{
			ak_print_normal("%s",help_sta[1]);
			return;
		}
		if(strlen(args[1]) > MAX_SSID_LEN)
		{
			ak_print_normal("ssid should less than 32 characters\n");
			return;
		}
		if(argc == 3 && strlen(args[2]) > MAX_KEY_LEN)
		{
			ak_print_normal("password should less than 64 characters\n");
			return;
		}
		
		strcpy(conn_param.essid, args[1]);
		
		if(argc == 3)
		{
			strcpy(conn_param.password, args[2]);
		}
		else
		{
			memset(conn_param.password, 0, MAX_KEY_LEN);
		}
		/*create a task to connetc AP,  so the STA can reconnect AP case the AP unavailable temporily for some reason*/
		if(g_sta_conn_thread_id == AK_INVALID_TASK)
		{
			wifi_set_mode(WIFI_MODE_STA);
			sta_reconn_flag = AK_TRUE;
			ak_thread_create(&g_sta_conn_thread_id , (void*)sta_conn_thread , &conn_param, 4096, 10);
		}
		else
		{
			ak_print_normal("sta is connecting, please disconnect it first\n");
		}
		
	}
	else if (strcmp(args[0], "disconn") == 0)
	{
		if (argc != 1)
		{
			ak_print_normal("%s",help_sta[1]);
			return;
		}

		ak_print_normal("disconnect wifi \n");
		
		sta_reconn_flag = AK_FALSE;
		wifi_disconnect();
		if(g_sta_conn_thread_id != AK_INVALID_TASK)
		{
			ak_thread_join(g_sta_conn_thread_id);
			g_sta_conn_thread_id = AK_INVALID_TASK;
		}
		
	}
    else if (strcmp(args[0], "init") == 0)
	{
		int fd;
		int tmp;
		fd = dev_open(DEV_WIFI);
		dev_read(fd,  &tmp, 1);
		gpio_share_pin_set( ePIN_AS_SDIO , tmp);
		if(fd < 0)
		{
			printk("open wifi faile\r\n");
			return;
		}
		wifi_init(NULL);
		
	}
	else
	{
		ak_print_normal("%s",help_sta[1]);
	}
}

static void cmd_wifi_ap(int argc, char **args)
{
	struct _apcfg  ap_cfg;

	if (strcmp(args[0], "start") == 0)
	{
		if (argc != 3 && argc != 4)
		{
			ak_print_normal("%s",help_ap[1]);
			return;
		}
		
		memset(&ap_cfg, 0, sizeof(struct _apcfg));
		ap_cfg.mode = 0; //0:bg mode  1:bgn mode
		
		ap_cfg.channel = atoi(args[1]);
		if(ap_cfg.channel < 1 || ap_cfg.channel > 13)
		{
			ak_print_normal("channel should be 1 ~ 13\n");
			return;
		}
		strcpy(ap_cfg.ssid, args[2]);
		if(strlen(ap_cfg.ssid) > MAX_SSID_LEN)
		{
			ak_print_normal("AP SSID should be less than 32 characters \n");
			return;
		}
		if (argc == 4)
		{
			if(strlen(args[3]) < MIN_KEY_LEN || strlen(args[3]) > MAX_KEY_LEN)
			{
				ak_print_normal("AP key should be %d ~ %d characters \n", MIN_KEY_LEN, MAX_KEY_LEN);
				return;
			}	
			strcpy(ap_cfg.key, args[3]);
			ap_cfg.enc_protocol = KEY_WPA2;
		}
		else
		{
			ap_cfg.enc_protocol = KEY_NONE;
		}
		
		if(g_sta_conn_thread_id != AK_INVALID_TASK)
		{	
		    sta_reconn_flag = AK_FALSE;
			wifi_disconnect();
			ak_thread_join(g_sta_conn_thread_id);
			g_sta_conn_thread_id = AK_INVALID_TASK;
		}
		ak_print_normal("Create AP SSID:%s, 11n: %s, key mode:%d, key:%s, channel:%d\n", 
			ap_cfg.ssid, ap_cfg.mode?"enable":"disable", 
			ap_cfg.enc_protocol, ap_cfg.enc_protocol?(char*)ap_cfg.key:"",
			ap_cfg.channel);
		
		wifi_set_mode(WIFI_MODE_AP);
		wifi_create_ap(&ap_cfg);
		
		wifi_netif_init();
	}
	else if (strcmp(args[0], "stop") == 0)
	{
		if (argc != 1)
		{
			ak_print_normal("%s",help_ap[1]);
			return;
		}
		ak_print_normal("stop wifi ap...\n");
		wifi_destroy_ap();
	}
	else
	{
		ak_print_normal("%s",help_ap[1]);
	}
}

void print_scan_ap_list(wifi_ap_info_t *ap, int count)
{
	int k;
	printf("  # BSSID             RSSI Chan  Security    SSID\n");
	printf("----------------------------------------------------------------\n");

	for (k = 0; k < count; k++) {
		printf("%3d ", k+1 );
		printf("%02X:%02X:%02X:%02X:%02X:%02X ", ap[k].bssid[0], ap[k].bssid[1], ap[k].bssid[2], 
												 ap[k].bssid[3], ap[k].bssid[4], ap[k].bssid[5]);
		printf(" %d ", ap[k].rssi);    
		printf(" %3d  ", ap[k].channel);
		printf("%-10s ", SECURITY_STR(ap[k].security));
		printf(" %-32s ", ap[k].ssid);
		printf("\n");
	}
}

static void cmd_wifi_scan(int argc, char **args)
{
    int count = 30;
    wifi_ap_info_t *ap;
    ap = malloc(sizeof(wifi_ap_info_t) * count);
	mhd_scan();
    mhd_get_scan_results(ap, &count);
	print_scan_ap_list(ap, count);
    free(ap);
}

typedef struct tcpka_conn_info {
    uint32_t tcpka_sess_ipid;
    uint32_t tcpka_sess_seq;
    uint32_t tcpka_sess_ack;
} tcpka_conn_sess_info_t;

static void cmd_wifi_tcpka(int argc, char **args)
{
	
	
	if (strcmp(args[0], "on") == 0)
	{
		uint32_t server_ip = inet_addr("10.10.28.227");
		int port = 8887;
		struct sockaddr_in tcps_sockaddr;
		int socket_s = -1, err = -1;

		//ipaddr
		if (argc > 1 && (char *)args[1] != NULL)
		{
				server_ip = inet_addr((char *)args[1]);
				if (IPADDR_NONE == server_ip)
				{
				   ak_print_error("set remote_ipaddr wrong.\n");
				   return;
				}
		}

		//port
		if (argc > 2 && (int *)args[2] != NULL)
		{
				port = atoi(args[2]);
				if(port > 65535)
				{
					ak_print_error("port should less than 65535\n");
					return;
				}
		}
		tcps_sockaddr.sin_family = AF_INET;
		tcps_sockaddr.sin_len = sizeof(struct sockaddr_in);
		tcps_sockaddr.sin_port = htons(port);
		tcps_sockaddr.sin_addr.s_addr = server_ip;

		socket_s = socket(AF_INET, SOCK_STREAM, 0);
		if (socket_s < 0)
		{
			closesocket(socket_s);
			ak_print_error("create socket error.\n");
			return;
		}
		else
		{
			ak_print_normal("connect to %s:%d \n", inet_ntoa(tcps_sockaddr.sin_addr), tcps_sockaddr.sin_port);
			err = connect(socket_s, (struct sockaddr*)&tcps_sockaddr, sizeof(struct sockaddr_in));
			if (err != 0)
			{
				ak_print_error("connect socket error\n");
			}
			else
			{
				tcpka_timer_t timer;
				char payload[] = "keepalive\n";
				timer.interval = 10;
				timer.retry_interval = 3;
				timer.retry_count = 5;
				mhd_tcpka_start(socket_s, payload, &timer);
			}
		}
	}
	else if (strcmp(args[0], "off") == 0)
	{
		mhd_tcpka_stop();
	}
	else if (strcmp(args[0], "info") == 0)
	{
		tcpka_conn_sess_info_t aaa;
		aaa.tcpka_sess_ipid = 1;
		mhd_tcpka_get_info(&aaa);
		printf("ipid: %u seq: %u, ack: %u\r\n", aaa.tcpka_sess_ipid, aaa.tcpka_sess_seq, aaa.tcpka_sess_ack);
	}
	else
	{
		ak_print_error("cmd error\n");
		ak_print_normal("%s",help_tcpka[1]);
		
	}
}

static void cmd_wifi_wowl(int argc, char **args)
{
	int dtim;
	if (strcmp(args[0], "status") == 0)
	{
		mhd_wowl_sta_check_status();
	}
	else if (strcmp(args[0], "add") == 0)
	{
		mhd_wowl_sta_add("0x983B16F8F39C", 42);
	}
	else if (strcmp(args[0], "del") == 0)
	{
		mhd_wowl_sta_del("0x983B16F8F39C", 42);
	}
	else if (strcmp(args[0], "on") == 0)
	{
		mhd_wowl_sta_enable();
	}
	else if (strcmp(args[0], "off") == 0)
	{
		mhd_wowl_sta_disable();
		wifi_power_cfg(0);
	}
	else if (strcmp(args[0], "dtim") == 0)
	{
		if(argc != 2)
		{
			ak_print_normal("%s",help_wowl[1]);
		}
		dtim = atoi(args[1]);
		if(dtim > 0)
		{
			ak_print_normal("set dtim to %d\n", dtim);
			mhd_sta_set_bcn_li_dtim(dtim);
		}
	}
	else
	{
		ak_print_normal("%s",help_wowl[1]);
	}
}

static void cmd_wifi_sdio(int argc, char **args)
{
	#if 0
	char *sdio_parti = "SDIOPA";
    char data[2] = {0};
	char *buffer;
	char *backup_data;
	int backup_size;

	if (strcmp(args[0], "bk") == 0)
	{		
		//sdio_backup(sdio_parti);

		backup_size = mhd_deep_sleep_backup_size();
		backup_data = mhd_deep_sleep_backup_data();

		buffer = malloc(backup_size + 2); //mode & back size
		buffer[0] = 1;
		buffer[1] = backup_size;
		memcpy(buffer+2, backup_data, backup_size);

		flash_value(1, buffer, backup_size+2);
		free(buffer);
	}
	else if (strcmp(args[0], "res") == 0)
	{
		//sdio_restore(sdio_parti);
	}
	else if (strcmp(args[0], "clr") == 0)
	{
	    data[0] = 0;
        flash_value(1, data, 1);
	}	
	else
	{
		ak_print_normal("%s", help_sdio[1]);
		return;
	}

	#endif
}


bool flash_value(char mode, char *data, int len)
{
	char *flag_parti = "FLAGPA";
	void *handle = NULL;
	int ret;

	if (data == NULL)
		return;

	handle = ak_partition_open(flag_parti);
	if(handle == NULL)
	{
		printk("open %s error\n", flag_parti);
        memset(data, 0, len);
        ret = false;
	}
	else
	{
		if (mode == 1)
		{
			ret = ak_partition_write(handle, data, len);
			if(ret < 0)
			{
				printk("write %s error\n", flag_parti);
                ret = false;
			}
			else
			{
				printk("write %s ok\n", flag_parti);
                ret = true;
			}
		}
		else if (mode == 0)
		{
			ret = ak_partition_read(handle, data, len);
			if(ret < 0)
			{
				printk("read %s error\n", flag_parti);
                ret = false;
			}
			else
			{
				printk("read %s ok\n", flag_parti);
                ret = true;
			}
		}

		ak_partition_close(handle);
	}

	return ret;
}

static void cmd_wifi_flash(int argc, char **args)
{
	uint8_t x;
	
	if (strcmp(args[0], "1") == 0)
	{
		x = 1;
		flash_value(1, &x, 1);	
	}
	else if (strcmp(args[0], "0") == 0)
	{
		x = 0;
		flash_value(1, &x, 1);
	}
	else
	{
		flash_value(0, &x, 1);
		ak_print_normal("Current Value %d\n", x);
	}
}

static void smartlink_finished(es_ap_info_t *target_ap)
{
	mhd_sta_connect((char*)target_ap->SSID.val, (char*)target_ap->BSSID.octet, target_ap->security, target_ap->security_key, target_ap->channel);	
}

static void cmd_wifi_es(int argc, char **args)
{
	if (strcmp(args[0], "on") == 0)
	{
		mhd_sta_down();
		mhd_easy_setup_start(smartlink_finished);
	}
	else if (strcmp(args[0], "off") == 0)
	{
		mhd_easy_setup_stop();
	}
}


int wifi_demo_init()
{
	cmd_register("ap", cmd_wifi_ap, help_ap);
	cmd_register("sta", cmd_wifi_sta, help_sta);
	cmd_register("scan", cmd_wifi_scan, help_scan);
	cmd_register("tcpka", cmd_wifi_tcpka, help_tcpka);
	cmd_register("wowl", cmd_wifi_wowl, help_wowl);
	cmd_register("sdio", cmd_wifi_sdio, help_sdio);
	cmd_register("flash", cmd_wifi_flash, help_sdio);


	cmd_register("es", cmd_wifi_es, help_es);
	return 0;
}

cmd_module_init(wifi_demo_init)


