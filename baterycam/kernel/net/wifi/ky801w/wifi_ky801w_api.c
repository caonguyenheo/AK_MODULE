/*
 *
 */
#include "wifi.h"
#include "lwip/tcpip.h"


#include "drv_api.h"
#include "platform_devices.h"
#include "dev_drv.h"
#include "drv_gpio.h"
/**
 * @brief get wifi version
 * @author Jian_Kui
 * @date 2017-10-24
 * @param [in] handle: audio out device handle
 * @param[out] fram_size:read size store address
 * @return int
 * @retval -1 operation failed
 * @retval >=0 frame size
 */
char *wifi_get_version(void)
{
	return ky801w_Wifi_GetVersion();
}

/**
 * @brief  配置省电模式,在保持连接的情况下可达到很可观省电效果
 *		省电模式下的耗电量由数据收发量决定
 *		此外用户可以配置更加细节的电源管理参数,比如DTIM间隔
 *		一般情况下使用此函数已经够用
 *
 * @param  power_save_level : 取值 0,1,2; 
 *	0代表关闭省电模式(CAM)
 *	1采用DTIM省电方式
 *    2采用INACTIVITY省电方式
 * @param  mac : 指向需要获取的station的地址
 */
int wifi_power_cfg(unsigned char power_save_level)
{
    #if 0
    ipinfo info;
    struct cfg_ps_request wowreq;
    netmgr_ipinfo_get(WLAN_IFNAME, &info);
    MEMSET((void*)&wowreq,0,sizeof(wowreq));
    LOG_PRINTF("ipv4=%x\r\n",info.ipv4);
    wowreq.ipv4addr = info.ipv4;
    wowreq.dtim_multiple = power_save_level; //3;
    wowreq.host_ps_st = HOST_PS_SETUP;
    ssv6xxx_wifi_pwr_saving(&wowreq,TRUE);
    #endif

    return ky801w_wifi_power_cfg(power_save_level);
}

/**
 * @brief  wifi_get_mode, get wifi mode
 * @param  : 
 * @retval AK_WIFI_MODE
 */
int wifi_get_mode()
{
    return ky801w_wifi_get_mode();
}

/**
 * @brief  wifi_set_mode, set wifi mode.
 * ap_wifi_dev and sta_wifi_dev are two different device, set mode just set the global variable wifi_dev
 * @param  : 
 * @retval AK_WIFI_MODE
 */
 
int wifi_set_mode(int type)
{
   	printk("wifi_set_mode type %d\r\n", type);
    return ky801w_wifi_set_mode(type);;
}


/**
 * @brief  wifi_isconnected, return wifi connect status
 * @param  : 
 * @retval 1 - connect , 0 - not connected 
 */

int wifi_isconnected()
{
    return ky801w_wifi_isconnected();
}

/**
 * @brief  wifi_connect, wifi connect to ap 
 * @param  : ssid, key
 * @retval 0 - send connect msg ok , -1 - some error happen 
 */

int wifi_connect( char *essid, char *key)
{
	return ky801w_wifi_connect(essid, key);
}

/**
 * @brief  wifi_disconnect, wifi disconnect from ap 
 * @param  : ssid, key
 * @retval 0 - send connect msg ok , -1 - some error happen 
 */

int wifi_disconnect()
{
	return ky801w_wifi_disconnect();
}

int wifi_list_ap(wifi_ap_info_t *ap, int count)
{
	return 0;
	//return sv6030_wifi_list_ap(ap, count);
}


/**
 * @brief  wifi_create_ap, destroy
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_create_ap(struct _apcfg *ap_cfg)
{
	//wifi_ap_cfg(ap_cfg->ssid, ap_cfg->mode, ap_cfg->key, ap_cfg->enc_protocol, ap_cfg->channel, WIFI_MAX_CLIENT_DEFAULT);
	//wifi_power_cfg(0);
	return ky801w_wifi_create_ap(ap_cfg);
}

/**
 * @brief  wifi_destroy_ap, destroy
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_destroy_ap()
{
    //do nothing
    return ky801w_wifi_destroy_ap();
}

int wifi_list_station(struct sta_info_t *sta, int count)
{
	//return ky801w_wifi_sleep(sta, count);
}

int wifi_sleep()
{
	//return ky801w_wifi_sleep();
}

int wifi_wakeup()
{
	//return ky801w_wifi_wakeup();
}
/**
*wifi_ap_cfg, set ap config with 11n param
*@param name: ssid
*@param 11n_mode:  1 - enable 11n , others - disable 11n
*@param key: ciper
*@param key_type: key mode KEY_NONE, KEY_WEP,KEY_WPA,KEY_WPA2
*@param channel: wifi channel
*@param max_client: max client allowed
*/
int wifi_ap_cfg(char *name, int dot11n_enable, char *key, int key_type, int channel, int max_client)
{
	return ky801w_wifi_ap_cfg(name,dot11n_enable,key,key_type,channel,max_client);
}


/**
*wmj+ 
*wifi_get_tx_pending get current tx_pending in driver.
*@return   tx_pending --success, -1 --fail
*/
int wifi_get_tx_pending()
{
    return ky801w_wifi_get_tx_pending();
}
/**
*wmj+ 
*wifi_set_max_tx_pending  set max tx_pending allowed in driver.
*@param tx_pending,  input value 
*@return   0 --success, others --fail

*/

int wifi_set_max_tx_pending(unsigned char max_tx_pending)
{
    return ky801w_wifi_set_max_tx_pending();
}

/**
*wmj+ 
*wifi_get_max_tx_pending  get max tx_pending allowed in driver.
*@return   max_tx_pending --success, -1 --fail

*/

int wifi_get_max_tx_pending()
{
	return ky801w_wifi_get_max_tx_pending();
}

//wmj<<




/**
 * @brief  读取mac地址，mac地址从8782芯片读取
 * @param  mac_addr : mac地址 6BYTE
 */
int wifi_get_mac(unsigned char *mac_addr)
{
	//memcpy(mac_addr, wifi_dev->dev_addr, ETH_ALEN);
	//netdev_getmacaddr(NULL,mac_addr);
	ky801w_wifi_get_mac(mac_addr);
	return 0;
}


int wifi_keepalive_set(keep_alive_t *keep_alive)
{

}

int wifi_check_wakeup_status(void)
{

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
	int ret ;

	int fd;
	T_WIFI_INFO  *wifi =  NULL; //(T_WIFI_INFO *)wifi_dev.dev_data;


    fd = dev_open(DEV_WIFI);
    if(fd < 0)
    {
        printk("open wifi faile\r\n");
        while(1);
    }
    dev_read(fd,  &wifi, sizeof(unsigned int *));
    gpio_share_pin_set( ePIN_AS_SDIO , wifi->sdio_share_pin);
	//printk("=====%d %d %d\r\n",wifi->sdio_share_pin,wifi->bus_mode,wifi->clock);


	
	ret = ky801w_wifi_init();
	if(0 == ret)
		tcpip_init(NULL, NULL);
	return ret;
}



