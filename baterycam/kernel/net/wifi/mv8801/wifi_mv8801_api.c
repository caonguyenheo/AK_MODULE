/*
 * wifi API for application
 *
 */

#include <stdint.h>
#include "wifi.h"
#include "lwip/netif.h"

//#include "lwip/tcpip.h"
#include "drv_api.h"
#include "platform_devices.h"
#include "dev_drv.h"
#include "drv_gpio.h"


/* Suppress unused parameter warning */
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER( x ) ( (void)(x) )
#endif


#define SSID_MAX_LEN 32
#define KEY_MAX_LEN 64


char *wifi_get_version()
{
	return(moal_wifi_get_version());
}

/**
 * @brief  wifi_get_mode, get wifi mode
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_get_mode(void)
{
	return moal_wifi_get_mode();	
}

/**
 * @brief  wifi_set_mode, set wifi mode.
 * ap_wifi_dev and sta_wifi_dev are two different device, set mode just set the global variable wifi_dev
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_set_mode(int type)
{
	return moal_wifi_set_mode(type);
}


/**
 * @brief  wifi_isconnected, return wifi connect status
 * @param  : 
 * @retval 1 - connect , 0 - not connected 
 */

int wifi_isconnected(void)
{
	return moal_wifi_isconnected();
}

/**
 * @brief  wifi_connect, wifi connect to ap 
 * @param  : ssid, key
 * @retval 0 - send connect msg ok , -1 - some error happen 
 */

int wifi_connect( char *essid, char *key)
{
	return moal_wifi_connect(essid, key);
}


/**
 * @brief  wifi_disconnect, wifi disconnect from ap 
 * @param  : ssid, key
 * @retval 0 - send connect msg ok , -1 - some error happen 
 */

int wifi_disconnect(void)
{
	return moal_wifi_disconnect();
}


/**
 * @brief  wifi_create_ap, destroy
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_create_ap(struct _apcfg *ap_cfg)
{
	return moal_wifi_create_ap(ap_cfg);
}

/**
 * @brief  wifi_destroy_ap, destroy
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_destroy_ap(void)
{
	return moal_wifi_destroy_ap();
}

int wifi_set_txpower(uint8_t dbm)
{
	moal_wifi_set_txpower(dbm);
}


int wifi_get_pscfg(power_save_t  *pscfg)
{
    return moal_wifi_get_pscfg(pscfg);
}


int wifi_set_pscfg(power_save_t  *pscfg)
{
	return moal_wifi_set_pscfg(pscfg);
}

/**
 * @brief  配置省电模式,在保持连接的情况下可达到很可观省电效果
 *		省电模式下的耗电量由数据收发量决定
 *		此外用户可以配置更加细节的电源管理参数,比如DTIM间隔
 *		一般情况下使用此函数已经够用
 *
 * @param  power_save_level : 取值 0,1,2; 
 *	0 PM0 No power save
 *	1 PM1
 *  2 PM2 with sleep delay
 * @param  mac : 指向需要获取的station的地址
 */
int wifi_power_cfg(uint8_t power_save_level)
{
	/*ps mode is set to 1 as default and this interface may cause some sdio cmd timeout, so comment it*/
	//return moal_wifi_power_cfg(power_save_level);
	return 0;
}


int wifi_ps_status()
{
	return 0;
}

int wifi_keepalive_set(keep_alive_t *keep_alive)
{
	return 0;
}

int wifi_keepalive_get(keep_alive_t *keep_alive)
{
	return 0;
}


int wifi_smc_start(smc_ap_info_t *ret_ap_info)
{
	return 0;
}

int wifi_smc_stop()
{
	return 0;

}


/*

######################### MEF Configuration command ##################
mefcfg={
	#Criteria: bit0-broadcast, bit1-unicast, bit3-multicast
	Criteria=2 		# Unicast frames are received during hostsleepmode
	NumEntries=1		# Number of activated MEF entries
	#mef_entry_0: example filters to match TCP destination port 80 send by 192.168.0.88 pkt or magic pkt.
	mef_entry_0={
		#mode: bit0--hostsleep mode, bit1--non hostsleep mode
		mode=1		# HostSleep mode
		#action: 0--discard and not wake host, 1--discard and wake host 3--allow and wake host
		action=3	# Allow and Wake host
		filter_num=3    # Number of filter
		#RPN only support "&&" and "||" operator,space can not be removed between operator.
		RPN=Filter_0 && Filter_1 || Filter_2
		#Byte comparison filter's type is 0x41,Decimal comparison filter's type is 0x42,
		#Bit comparison filter's type is  0x43
		#Filter_0 is decimal comparison filter, it always with type=0x42
		#Decimal filter always has type, pattern, offset, numbyte 4 field
                #Filter_0 will match rx pkt with TCP destination port 80
		Filter_0={
			type=0x42	# decimal comparison filter
			pattern=80	# 80 is the decimal constant to be compared
			offset=44	# 44 is the byte offset of the field in RX pkt to be compare
			numbyte=2       # 2 is the number of bytes of the field
		}
		#Filter_1 is Byte comparison filter, it always with type=0x41
		#Byte filter always has type, byte, repeat, offset 4 filed
		#Filter_1 will match rx pkt send by IP address 192.168.0.88
		Filter_1={
			type=0x41          	# Byte comparison filter
			repeat=1                # 1 copies of 'c0:a8:00:58'
			byte=c0:a8:00:58	# 'c0:a8:00:58' is the byte sequence constant with each byte
						# in hex format, with ':' as delimiter between two byte.
			offset=34               # 34 is the byte offset of the equal length field of rx'd pkt.
		}
		#Filter_2 is Magic packet, it will looking for 16 contiguous copies of '00:50:43:20:01:02' from
		# the rx pkt's offset 56
		Filter_2={
			type=0x41		# Byte comparison filter
			repeat=16               # 16 copies of '00:50:43:20:01:02'
			byte=00:50:43:20:01:02  # '00:50:43:20:01:02' is the byte sequence constant
			offset=56		# 56 is the byte offset of the equal length field of rx'd pkt.
		}
	}
}

#--------------------------------------------------------------------------------------------------
#	example: Disable MEF filters
#	mefcfg={
#		#Criteria: bit0-broadcast, bit1-unicast, bit3-multicast
#		Criteria=2 		# Unicast frames are received during hostsleepmode
#		NumEntries=0		# Number of activated MEF entries
#	}

*/
static unsigned char mef_conf[WAKEUP_PKT_MAX];

static unsigned char mef_conf_format[] = 
	"mefcfg={\n"\
		"Criteria=3\n"\
		"NumEntries=1\n"\
		"mef_entry_0={\n"\
			"mode=1\n"\
			"action=3\n"\
			"filter_num=2\n"\
			"RPN=Filter_0 || Filter_1\n"\
			"Filter_0={\n"\
				"type=0x41\n"\
				"repeat=1\n"\
				"byte=98:3b:16:f8:f3:9c\n"\
				"offset=50\n"\
			"}\n"\
			"Filter_1={\n"\
				"type=0x41\n"\
				"repeat=1\n"\
				"byte=%02x:%02x:%02x:%02x\n"\
				"offset=46\n"\
			"}\n"\
		"}\n"\
	"}\n";

/*wake up data udp packet*/
/*0x983B16F8F39C*/
/* or ARP packet for device ip
Filter_1 looking for rx pkt with ARP target protocol addr 192.168.0.104
#		Filter_2={
#			type=0x41
#			repeat=1
#			byte=c0:a8:00:68
#			offset=46
#		}
*/
static net_wakeup_cfg_t wakeup_cfg;

extern struct netif if_wifi;


int wifi_sleep()
{
	char *buffer = NULL;
	int buffer_len;
	char *p_sdio_data;
	int sdio_data_len;
	unsigned char my_ip[4];

	
	//back up SDIO data;
	sdio_backup_get_data(&p_sdio_data, &sdio_data_len);
	printk("sdio back data len %d\n", sdio_data_len);
	buffer_len = sdio_data_len + 4;// sdio data + sdio data size
	buffer = malloc(buffer_len); 
	if(buffer == NULL)
	{
		printk("alloc mem for sdio data error\n");
		return -1;
	}
	memcpy(buffer, &sdio_data_len, 4);//first 4 bytes store sdio_data_len
	memcpy(buffer + 4, p_sdio_data, sdio_data_len); 
	sdio_backup_flash_value(1, buffer, buffer_len);
	free(buffer);

	/*set arp for me as wakeup data*/
   	memcpy(my_ip, &if_wifi.ip_addr, 4);
	sprintf(mef_conf, mef_conf_format, my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
	printk("mef_config: %s\n", mef_conf);
	
	//set wakeup data
	wakeup_cfg.wakeup_data_offset = 50;
	wakeup_cfg.wakeup_data_len = strlen(mef_conf);
	
	if(wakeup_cfg.wakeup_data_len > WAKEUP_PKT_MAX)
	{
		printk("wakeup data len too long, wifi sleep error\n");
		return -1;
	}
	strcpy(wakeup_cfg.wakeup_data, mef_conf);
	
	return moal_wifi_sleep(&wakeup_cfg);
	
}


int wifi_wakeup(void)
{
	return wifistation_netif_init();
}

/**
*wifi_get_tx_pending get current tx_pending in driver.
*@return   tx_pending --success, -1 --fail
*/
int wifi_get_tx_pending(void)
{
	return moal_wifi_get_tx_pending();

}
/**
*wifi_set_max_tx_pending  set max tx_pending allowed in driver.
*@param tx_pending,  input value 
*@return   0 --success, others --fail

*/

int wifi_set_max_tx_pending(t_u8 max_tx_pending)
{
	return moal_wifi_set_max_tx_pending(max_tx_pending);
}

/**
* 
*wifi_get_max_tx_pending  get max tx_pending allowed in driver.
*@return   max_tx_pending --success, -1 --fail

*/

int wifi_get_max_tx_pending(void)
{
	return moal_wifi_get_max_tx_pending();
}


int wifi_list_station(wifi_sta_list_t *sta_list)
{
	return moal_wifi_get_sta_list(sta_list);
}


/**
 * @brief  读取mac地址，mac地址从8782芯片读取
 * @param  mac_addr : mac地址 6BYTE
 */
int wifi_get_mac(unsigned char *mac_addr)
{
	return moal_wifi_get_mac(mac_addr);
}


/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
wifi_if_init(struct netif *netif)
{
	return moal_wifi_if_init(netif);
}


/**
 * @brief initializing wifi 
 * @author
 * @date 
 * @param [in] pParam a pointer to T_WIFI_INITPARAM type
 * @return int
 * @retval   0  initializing sucessful
 * @retval  -1 initializing fail
 */
int wifi_init(void *init_param)
{
	int ret;
	unsigned char wifi_backup_flg;
	char *buffer = NULL;
	int buffer_len;
	char *p_sdio_data;
	int sdio_data_len;
	unsigned char mac[6] = {0,0,0,0,0,0};

	int fd;
	T_WIFI_INFO  *wifi =  NULL; //(T_WIFI_INFO *)wifi_dev.dev_data;
	unsigned long  nb;

    fd = dev_open(DEV_WIFI);
    if(fd < 0)
    {
        printk("open wifi faile\r\n");
        while(1);
    }
    dev_read(fd,  &wifi, sizeof(unsigned int *));
    gpio_share_pin_set( ePIN_AS_SDIO , wifi->sdio_share_pin);
	//printk("=====%d %d %d\r\n",wifi->sdio_share_pin,wifi->bus_mode,wifi->clock);
	
	
	printk("\nWifilib version:%s\n", wifi_get_version());
	
	if(NULL == init_param)
	{
		 wifi_backup_flg = 0;
	}
	else
	{
		 wifi_backup_flg = *(unsigned char*)init_param;
	}
	
	if(wifi_backup_flg)
	{
		printk("wifi_backup_flg 1\n");
		
		//read data size first
		ret = sdio_backup_flash_value(0, &sdio_data_len, 4);
		if(ret == 0 || sdio_data_len == 0) 
		{
			printk("sdio_backup_flash_value read Error!\n");
			printk("init wifi\n");
			ret = moal_wifi_init(init_param);
		}
		else
		{
			buffer = malloc(sdio_data_len + 4); 
			if(buffer == NULL)
			{
				printk("alloc mem for sdio data resume error\n");
				return -1;
			}
			if(!sdio_backup_flash_value(0, buffer, sdio_data_len + 4)) //read data and data size
			{
				printk("Error sdio_backup_flash_value read!\n");
				return -1;
			}
			printk("resume wifi data len %d\n", sdio_data_len);
			sdio_backup_set_data(buffer + 4, sdio_data_len);
			ret = moal_wifi_resume();
			free(buffer);
		}
    }
	else
	{
		printk("init wifi\n");
        
        // reset wifi chip
        //#define nb 14
        nb = wifi->gpio_power.nb;
        gpio_set_pin_as_gpio(nb);
        gpio_set_pin_dir(nb, 1);
        gpio_set_pull_down_r(nb, 0);
        gpio_set_pin_level(nb, 0); 
        ak_sleep_ms(100);
        gpio_set_pin_level(nb, 1); 
        
		ret = moal_wifi_init(init_param);
	}
	
	wifi_get_mac(mac);
	printk("\nmac addr=%x:%x:%x:%x:%x:%x\n\n"
		, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    
	if(0 == ret )
	{
		tcpip_init(NULL, NULL);
	}
	dev_close(fd);
	return ret;
}


//0: normal wakeup, -1: wakeup abnormal, 
int wifi_check_wakeup_status(void)
{
    return 0;
}
