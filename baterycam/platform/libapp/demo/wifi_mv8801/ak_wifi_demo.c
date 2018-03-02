/*
*wifi_demo.c    wifi operation demo
*/
#include <stdint.h>
#include "wifi.h"
#include "command.h"
#include "ak_common.h"
#include "ak_thread.h"
#include "akos_error.h"
#include "lwip/ip_addr.h"
#include "dev_info.h"
#include "drv_gpio.h"


#include "mlanutl.h"

/********************************************************
*			 Macro
********************************************************/
#define SDIO_RESTORE_FLAG_PARTI  "FLAGPA"
#define SDIO_BACK_PARTI          "SDIOPA"
#define WIFI_IP_PARTI			 "WIFIIP"
#define SERVER_IP_PARTI			 "SERVER"


/******************************************************
*                    Constant         
******************************************************/


/******************************************************
*                    Type Definitions         
******************************************************/
struct _conn_param
{
	unsigned char essid[MAX_SSID_LEN];
	unsigned char password[MAX_KEY_LEN];
};

struct ip_info
{
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
};

struct server_info
{
	unsigned char essid[MAX_SSID_LEN]; /*ap ssid*/
	unsigned char password[MAX_KEY_LEN];/*ap password*/
	unsigned int server_ip;             /*p2p server ip*/
	unsigned short server_port;         /*p2p server port*/ 

};

/** Character, 1 byte */
typedef signed char t_s8;
/** Unsigned character, 1 byte */
typedef unsigned char t_u8;

/** Short integer */
typedef signed short t_s16;
/** Unsigned short integer */
typedef unsigned short t_u16;

/** Integer */
typedef signed int t_s32;
/** Unsigned integer */
typedef unsigned int t_u32;

/** Long long integer */
typedef signed long long t_s64;
/** Unsigned long long integer */
typedef unsigned long long t_u64;


/** Type definition of mlan_ds_ps_cfg for MLAN_OID_PM_CFG_PS_CFG */
typedef struct _mlan_ds_ps_cfg {
    /** PS null interval in seconds */
	t_u32 ps_null_interval;
    /** Multiple DTIM interval */
	t_u32 multiple_dtim_interval;
    /** Listen interval */
	t_u32 listen_interval;
    /** Adhoc awake period */
	t_u32 adhoc_awake_period;
    /** Beacon miss timeout in milliseconds */
	t_u32 bcn_miss_timeout;
    /** Delay to PS in milliseconds */
	t_s32 delay_to_ps;
    /** PS mode */
	t_u32 ps_mode;
} mlan_ds_ps_cfg, *pmlan_ds_ps_cfg;

/** Type definition of mlan_ds_hs_cfg for MLAN_OID_PM_CFG_HS_CFG */
typedef struct _mlan_ds_hs_cfg {
    /** MTRUE to invoke the HostCmd, MFALSE otherwise */
	t_u32 is_invoke_hostcmd;
    /** Host sleep config condition */
    /** Bit0: broadcast data
     *  Bit1: unicast data
     *  Bit2: mac event
     *  Bit3: multicast data
     */
	t_u32 conditions;
    /** GPIO pin or 0xff for interface */
	t_u32 gpio;
    /** Gap in milliseconds or or 0xff for special
     *  setting when GPIO is used to wakeup host
     */
	t_u32 gap;
    /** Host sleep wake interval */
	t_u32 hs_wake_interval;
    /** GPIO pin for indication wakeup source */
	t_u32 ind_gpio;
    /** Level on ind_gpio pin for indication normal wakeup source */
	t_u32 level;
} mlan_ds_hs_cfg, *pmlan_ds_hs_cfg;


typedef struct _mlan_ds_auto_assoc
{
	/*1/2/3 driver auto assoc/driver auto re-connect/FW auto re-connect		                   
	auto assoc takes effect in new connection (e.g. iwconfig essid),		                   
	driver will auto retry if association failed;		                   
	auto re-connect takes effect when link lost, driver/FW will try		                   
	to connect to the same AP
	*/
	t_u8 auto_assoc_type;
	/*1/0 on/off*/
	t_u8 auto_assoc_enable;
	/*0x1-0xff The value 0xff means retry forever (default 0xff)*/
	t_u8 auto_assoc_retry_count;
	/*0x0-0xff Time gap in seconds (default 10)*/
	t_u8 auto_assoc_retry_interval;
	/*: Bit 0:		                   
	Set to 1: Firmware should report link-loss to host if AP rejects	
	authentication/association while reconnecting		                  
	Set to 0: Default behavior: Firmware does not report link-loss		                             
	to host on AP rejection and continues internally		                   
	Bit 1-15: Reserved		                  
	The parameter flag is only used for FW auto re-connect
	*/
	t_u8 auto_assoc_flags;
}mlan_ds_auto_assoc, *pmlan_ds_auto_assoc;

typedef struct _mlan_ds_smc_chan_list {
	/** channel number*/
	t_u8 chan_number;
	/** channel width 0: 20M, 1: 10M, 2: 40M, 3: 80Mhz*/
	t_u8 chanwidth;
	/** min scan time */
	t_u16 minScanTime;
	/**max scan time */
	t_u16 maxScanTime;
} mlan_ds_smc_chan_list;
typedef struct _mlan_ds_smc_frame_filter {
	/** destination type */
	t_u8 destType;
	/** Frame type */
	t_u8 frameType;
} mlan_ds_smc_frame_filter;
typedef struct _mlan_ds_smc_mac_addr {
	/** start address */
	t_u8 startaddr[MLAN_MAC_ADDR_LENGTH];
	/**end address */
	t_u8 endaddr[MLAN_MAC_ADDR_LENGTH];
	/** filter type */
	t_u16 filterType;
} mlan_ds_smc_mac_addr;
typedef struct _mlan_ds_smc {
	/** smc action, 0: get, 1: set, 2: start, 3: stop */
	t_u16 action;
	/** channel list number*/
	t_u8 chan_num;
	/**channel list*/
	mlan_ds_smc_chan_list chanlist[MAX_CHANNEL_LIST];
	/** frame filter set*/
	t_u8 filterCount;
	/**frame filter */
	mlan_ds_smc_frame_filter framefilter[MAX_FRAME_FILTER];
	/** address range set */
	t_u8 addrCount;
	/** multi address */
	mlan_ds_smc_mac_addr addr[MAX_ADDR_FILTER];
	/** ssid length */
	t_u8 ssid_len;
	/** ssid */
	t_u8 ssid[32];
	/** beacon period */
	t_u16 beaconPeriod;
	/** beacon of channel */
	t_u8 beaconOfChan;
	/** probe of channel */
	t_u8 probeOfChan;
	/** customer ie */
	t_u8 customer_ie[MAX_IE_SIZE];
	/** IE len */
	t_u8 ie_len;
} mlan_ds_smc;


typedef struct _mlan_ds_misc_keep_alive {
	t_u8 mkeep_alive_id;
	t_u8 enable;
	t_u8 start;
	t_u16 send_interval;
	t_u16 retry_interval;
	t_u16 retry_count;
	t_u8 dst_mac[MLAN_MAC_ADDR_LENGTH];
	t_u8 src_mac[MLAN_MAC_ADDR_LENGTH];
	t_u16 pkt_len;
	t_u8 packet[MKEEP_ALIVE_IP_PKT_MAX];
} mlan_ds_misc_keep_alive;


/** mlan_ioctl_req data structure */
typedef struct _user_ioctl_req
{
    /** Request id */
    t_u32 req_id;
    /** Action: set or get */
    t_u32 action;
    union
	{
		/** Power saving mode for MLAN_OID_PM_CFG_IEEE_PS */
		t_u8 ps_mode;
		/** ps config mlan_ds_ps_cfg for MLAN_OID_PM_CFG_PS_CFG */
        mlan_ds_ps_cfg ps_cfg;
		/*fast link*/
		t_u8 fast_link_enable;
		/** Host Sleep configuration for MLAN_OID_PM_CFG_HS_CFG */
		mlan_ds_hs_cfg hs_cfg;
		/** fw re-connect cfg param set */
		mlan_ds_auto_assoc auto_reconnect;
		mlan_ds_smc smc;
		mlan_ds_misc_keep_alive keep_alive;
    }param;
} user_ioctl_req, *puser_ioctl_req;


 
/******************************************************
*                    Global Variables         
******************************************************/

/******************************************************
*			 Local Variables
******************************************************/
static char *help_ap[]={
	"wifi ap tools\n",
	"usage:ap start [ch] [ssid] <password>\n"
	"	   ap stop\n"
};
static char *help_sta[]={
	"wifi sta tools\n",
	"usage: sta conn [ssid] <password>\n"
	"		sta disconn\n"
	"		sta status, get connection status\n"
	"		sta config [ssid] [password] [IP] [port]\n"
	"		sta clearcfg, clear server config\n"
	"		sta fastlink\n"
	"		sta hscfg [condigion] [gpio] [gap] [ind_gpio] [level] \n"
	"		sta mefcfg, set mef with default\n"
	"		sta dhcp, get ip by dhcp\n"
	"		sta clearip, clear ip in flash\n"
	"		sta sleep\n"
	"		sta wakeup\n"
	"		sta init\n"
};
static char *help_sdio[]={
	"sdio back test\n",
	"usage: sdio resetflag 0/1\n"
	"		sdio back  \n"
	"		sdio restore \n"
			
};

static int sta_reconn_flag = AK_TRUE;
static ak_pthread_t  g_sta_conn_thread_id = AK_INVALID_TASK;
static struct _conn_param conn_param;

  
/******************************************************
*                    Function prototype                           
******************************************************/
static int sdio_flag_set(uint32_t reset_flag);
static int sdio_flag_get(uint32_t *reset_flag);

/******************************************************
*		        Local Function Declarations
******************************************************/

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
				wifi_power_cfg(0); //wmj for test
				wifistation_netif_init();
				break;
				
			}
		}
		ak_sleep_ms(1000);
	}
	ak_print_normal("sta conn thread exit.\n");
	ak_thread_exit();
}


static int sdio_flag_set(uint32_t reset_flag)
{
	
	void *handle = NULL;
	int ret = -1;
	char *flag_parti = SDIO_RESTORE_FLAG_PARTI;

	ak_print_normal("open %s \n", SDIO_RESTORE_FLAG_PARTI);
	
	handle = ak_partition_open(flag_parti);
	if(handle == NULL)
	{
		ak_print_normal("open %s error\n", SDIO_RESTORE_FLAG_PARTI);
	}
	else
	{
		ret = ak_partition_write(handle, &reset_flag, sizeof(uint32_t));
		if(ret < 0)
		{
			ak_print_normal("read %s error\n", flag_parti);
		}
		ak_partition_close(handle);
	}
	return ret;
}

static int sdio_flag_get(uint32_t *reset_flag)
{
	
	void *handle = NULL;
	int ret = -1;

	handle = ak_partition_open(SDIO_RESTORE_FLAG_PARTI);
	if(handle == NULL)
	{
		ak_print_normal("open %s error\n", SDIO_RESTORE_FLAG_PARTI);
		return ret;
	}
	else
	{
		ret = ak_partition_read(handle, reset_flag, sizeof(uint32_t));
		if(ret < 0)
		{
			ak_print_normal("read %s error\n", SDIO_RESTORE_FLAG_PARTI);
			ak_partition_close(handle);
			return ret;
		}
		else
		{
			ak_partition_close(handle);
			return *reset_flag;
		}
	}
}



/**
 * get configuration for camview (including ssid password ip port and port).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */

int wifi_set_config(struct server_info *t_server_info)
{
	void *handle = NULL;
	int ret = -1;
	char *server_parti = SERVER_IP_PARTI;

	ak_print_normal("open %s \n", server_parti);
	
	handle = ak_partition_open(server_parti);
	if(handle == NULL)
	{
		ak_print_normal("open %s error\n", server_parti);
	}
	else
	{
		ret = ak_partition_write(handle, t_server_info, sizeof(struct server_info));
		if(ret < 0)
		{
			ak_print_normal("write %s error\n", server_parti);
		}
		else
		{
			ak_print_normal("back IP info on %s OK\n", server_parti);
		}
		ak_partition_close(handle);
	}
	return ret;	
}


/**
 * get configuration for camview (including ssid password ip port and port).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */

int wifi_get_config(struct server_info *t_server_info)
{
	void *handle = NULL;
	int ret = -1;
	char *server_parti = SERVER_IP_PARTI;
	
	ak_print_normal("open %s \n", server_parti);
	
	handle = ak_partition_open(server_parti);
	if(handle == NULL)
	{
		ak_print_normal("open %s error\n", server_parti);
	}
	else
	{
		ret = ak_partition_read(handle, t_server_info, sizeof(struct server_info));
		if(ret < 0)
		{
			ak_print_normal("write %s error\n", server_parti);
		}
		else
		{
			ak_print_normal("back IP info on %s OK\n", server_parti);
		}
		ak_partition_close(handle);
	}
	return ret;	
}


static unsigned char magic_mef_conf[] = 
	"mefcfg={\n"\
		"Criteria=3\n"\
		"NumEntries=1\n"\
		"mef_entry_0={\n"\
			"mode=1\n"\
			"action=3\n"\
			"filter_num=3\n"\
			"RPN=Filter_0 && Filter_1 || Filter_2\n"\
			"Filter_0={\n"\
				"type=0x42\n"\
				"pattern=80\n"\
				"offset=44\n"\
				"numbyte=2\n"\
			"}\n"\
			"Filter_1={\n"\
				"type=0x41\n"\
				"repeat=1\n"\
				"byte=c0:a8:01:68\n"\
				"offset=34\n"\
			"}\n"\
			"Filter_2={\n"\
				"type=0x41\n"\
				"repeat=16\n"\
				"byte=00:50:43:21:be:14\n"\
				"offset=56\n"\
			"}\n"\
		"}\n"\
	"}\n";



/**
 *测试WIFI连接,
 *连接到指定名字的路由器，加密模式和频道自动适应
 *密码长度在WPA或WPA2模式下8 <= len <= 64;在WEP模式下必须为5或13
 */
static void cmd_wifi_sta(int argc, char **args)
{
	char *essid = args[1];
	char *password =args[2];
	unsigned long  ipaddr = IPADDR_NONE;
	unsigned short port = 0;

	int fastlink;
	mlan_ds_auto_assoc auto_reconnect;

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
	else if (strcmp(args[0], "status") == 0)
	{
		if(wifi_isconnected())
		{
			ak_print_normal("wifi connected\n");
		}
		else
		{
			ak_print_normal("wifi disconnected\n");
		}
		
	}
	else if(strcmp(args[0], "fastlink") == 0)
	{
		//sdio_flag_set(1);
		//sdio_backup(SDIO_BACK_PARTI);
		if(argc == 1)
		{
			fastlink = moal_wifi_get_fastlink_enable();
			ak_print_normal("fast link status %d\n", fastlink);
		}
		else
		{
			if(strcmp(args[1], "1") == 0)
			{
				moal_wifi_set_fastlink_enable(1);
			}
			else if(strcmp(args[1], "0") == 0)
			{
				moal_wifi_set_fastlink_enable(0);
			}
			else
			{
				ak_print_normal("wrong args\n");
			}
		}
			
		
		
	}
	else if(strcmp(args[0], "hscfg") == 0)
	{
		mlan_ds_hs_cfg hscfg;
		/* hs config*/
		if(argc == 1)/*get*/
		{
			moal_wifi_get_hscfg(&hscfg);
			/* GET operation */
			ak_print_normal("HS Configuration:\n");
			ak_print_normal("  Conditions: %d\n", (int)hscfg.conditions);
			ak_print_normal("  GPIO: %d\n", (int)hscfg.gpio);
			ak_print_normal("  GAP: %d\n", (int)hscfg.gap);
			ak_print_normal("  Indication GPIO: %d\n", (int)hscfg.ind_gpio);
			ak_print_normal("  Level for normal wakeup: %d\n", (int)hscfg.level);
			
		}
		else if(argc == 4 )
		{
			hscfg.is_invoke_hostcmd = MTRUE;
			hscfg.conditions = atoi(args[1]);
			hscfg.gpio = atoi(args[2]);
			hscfg.gap = atoi(args[3]);
			hscfg.hs_wake_interval = 0;
			hscfg.ind_gpio = 0;
			hscfg.level = 0;
			moal_wifi_set_hscfg(&hscfg);
		}
		
		
	}
	else if(strcmp(args[0], "ps") == 0)
	{
		int ps_enable;
		if(argc == 2)
		{
			ps_enable = atoi(args[1]);
			ak_print_normal("set sta ps %d\n", ps_enable);
			wifi_power_cfg(ps_enable);
		}
		else
		{
			ak_print_normal(" sta ps 0/1\n");
		}
	}

	else if(strcmp(args[0], "pscfg") == 0)
	{
		power_save_t pscfg;
		if(argc == 1)/*get*/
		{
			moal_wifi_get_pscfg(&pscfg);
			ak_print_normal("PS Configuration:\n");
			ak_print_normal("  ps_null_interval: %d\n", (int)pscfg.ps_null_interval);
			ak_print_normal("  multiple_dtim_interval: %d\n", (int)pscfg.dtim_interval);
			ak_print_normal("  delay_to_ps: %d\n", (int)pscfg.delay_to_ps);
			ak_print_normal("  ps_mode: %d\n", (int)pscfg.ps_mode);
		}
		else if(argc > 1)
		{
			
			pscfg.ps_null_interval = atoi(args[1]);
			pscfg.dtim_interval = 3;
			pscfg.delay_to_ps = 500;
			pscfg.ps_mode = 1;
			
			if(argc > 2)
			{
				if(!strncmp(args[2], "0x", 2)) 
				{
					pscfg.dtim_interval = strtol(args[2], NULL, 16);
				}
				else
				{
					pscfg.dtim_interval = atoi(args[2]);
				}
			}
			if(argc > 3)
			{
				pscfg.delay_to_ps = atoi(args[3]);
			}
			if(argc > 4)
			{
				pscfg.ps_mode = atoi(args[4]);
			}

			wifi_set_pscfg(&pscfg);
		
		}
	}
	else if(strcmp(args[0], "auto_reconn") == 0)
	{
		if(argc == 1)/*option get*/
		{
			moal_wifi_get_auto_reconnect(&auto_reconnect);
			
		}
		else
		{
			if(strcmp(args[1], "1") == 0)
			{
				auto_reconnect.auto_assoc_enable = 1;
			}
			else if(strcmp(args[1], "0") == 0)
			{
				auto_reconnect.auto_assoc_enable = 0;
			}
			else
			{
				ak_print_normal("wrong args\n");
				return;
			}
			auto_reconnect.auto_assoc_type = 3;
			auto_reconnect.auto_assoc_retry_count = 0xff;
			auto_reconnect.auto_assoc_retry_interval= 10; /* 10 second*/
			auto_reconnect.auto_assoc_flags = 0;
			
			moal_wifi_set_auto_reconnect(&auto_reconnect);
		}
	}
	else if(strcmp(args[0], "mefcfg") == 0)
	{
		moal_wifi_process_mefcfg(magic_mef_conf);
		
	}
	else if(strcmp(args[0], "multicast") == 0)
	{
		//moal_wifi_set_multicast_list();
		
	}
	else if(strcmp(args[0], "keepalive") == 0)
	{
		/* ip  port  payload*/
		
	}
	else if(strcmp(args[0], "smartcfg") == 0)
	{
		/* start smart config*/
		
	}
	else if(strcmp(args[0], "fwdown") == 0)
	{
		moal_wifi_fw_shutdown();
	}
	else if(strcmp(args[0], "txpower") == 0)
	{
		if(argc == 2)
		{
			uint8_t dbm = atoi(args[1]);
			if(dbm > 0 && dbm < 18)
			{
				moal_wifi_set_txpower(dbm);
			}
		}
		else
		{
			ak_print_normal("argument error\n");
		}
	}
	else if(strcmp(args[0], "dhcp") == 0)
	{
		wifistation_netif_init();
	}
	else if(strcmp(args[0], "clearip") == 0)
	{
		struct ip_info t_ip_info;
		memset(&t_ip_info, 0, sizeof(struct ip_info));
		wifi_back_ip_info(&t_ip_info);
	}
	else if(strcmp(args[0], "config") == 0)
	{
		struct server_info t_server_info;
		memset(&t_server_info, 0, sizeof(struct ip_info));

		if(argc == 1)
		{
			wifi_get_config(&t_server_info);
			ak_print_normal("ssid: %s\n", t_server_info.essid);
			ak_print_normal("password: %s\n", t_server_info.essid);
			ak_print_normal("IP: %u.%u.%u.%u\n", 
				t_server_info.server_ip & 0xff, 
				(t_server_info.server_ip >> 8) & 0xff,
				(t_server_info.server_ip >> 16) & 0xff, 
				(t_server_info.server_ip >> 24) & 0xff);
			ak_print_normal("port: %d\n", t_server_info.server_port);
			return;
		}
		if(argc != 5)
		{
			ak_print_normal("%s", help_sta[1]);
			return;
		}
		//SSID
		strcpy(t_server_info.essid, args[1]);
		//password
		strcpy(t_server_info.password, args[2]);
		//ipaddr
		
	    ipaddr = inet_addr((char *)args[3]);
		if (IPADDR_NONE == ipaddr)
		{
		   ak_print_error("set remote_ipaddr wrong.\n");
		   return;
		}
		t_server_info.server_ip = ipaddr;
		
		port = atoi(args[4]);
		if(port > 65535)
		{
			ak_print_error("port should less than 65535\n");
			return;
		}
		t_server_info.server_port = port;
		
		wifi_set_config(&t_server_info);
	}
	else if(strcmp(args[0], "clearcfg") == 0)
	{
		struct server_info t_server_info;
		memset(&t_server_info, 0, sizeof(struct server_info));
		wifi_set_config(&t_server_info);
	}
	else if(strcmp(args[0], "wakeup") == 0)
	{
		wifi_wakeup();
	}
	else if(strcmp(args[0], "sleep") == 0)
	{
		wifi_sleep();
	}
	else if (strcmp(args[0], "init") == 0)
	{
		t_u8 resume;
		
		if(argc == 2)
		{
			resume = 1;
			wifi_init(&resume);
		}
		else
		{
			wifi_init(NULL);
		}
		
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

static void cmd_wifi_sdio(int argc, char **args)
{
	uint32_t reset_flag;

	if (strcmp(args[0], "resetflag") == 0)
	{
		if(argc == 1)
		{
			if(sdio_flag_get(&reset_flag) < 0)
			{
				ak_print_normal("get flag error\n");
			}
			else
			{
				ak_print_normal("resetflag %d\n", reset_flag);
			}
		}
		else if(argc == 2)
		{
			reset_flag = atoi(args[1]);
			sdio_flag_set(reset_flag);
		}
		else
		{
			ak_print_normal("%s",help_sdio[1]);
		}
		
		
	}
	else if (strcmp(args[0], "back") == 0)
	{
		sdio_backup(SDIO_BACK_PARTI);
	
	}
	else if (strcmp(args[0], "restore") == 0)
	{
		sdio_restore(SDIO_BACK_PARTI);
	}
	else
	{
		ak_print_normal("%s",help_sdio[1]);
	}
	
}

/******************************************************
*		        Local Function Declarations
******************************************************/


int wifi_demo_init()
{
	cmd_register("ap", cmd_wifi_ap, help_ap);
	cmd_register("sta", cmd_wifi_sta, help_sta);
	return 0;
}

cmd_module_init(wifi_demo_init)


