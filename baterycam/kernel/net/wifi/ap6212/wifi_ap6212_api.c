
#include <stdint.h> 
#include <stddef.h>
#include "wifi.h"

#include "../wifi_backup_common.h"

#include "drv_api.h"
#include "platform_devices.h"
#include "dev_drv.h"
#include "drv_gpio.h"

#define WIFI_WAKEUP_PATTERN_RECEIVED    0x2
#define WIFI_WAKEUP_BEACON_LOST         0x10
#define WIFI_WAKEUP_KEEPALIVE_TIMEOUT   0x20000

net_wakeup_cfg_t wifi_power_save ={
	.wakeup_data_offset = 42, //42 - UDP paket , 54 - TCP packet
	.wakeup_data        = "0x983B16F8F39C",
	.wakeup_data_len	= 6
};

static uint32_t s_wifi_tcpka = 0;
static uint32_t s_wifi_wakeup_status = 0;

char *wifi_get_version(char *ver)
{
	char version[200] = { 0 };

	if (!mhd_get_wifi_version(version, sizeof(version)))
		memcpy(ver, version, strlen(version) );
		
	return ver;
}

/**
 * @brief  读取mac地址，
 * @param  mac_addr : mac地址 6BYTE
 */
 //TODO:
int wifi_get_mac(unsigned char *mac_addr)
{
	return mhd_sta_get_mac_address(mac_addr);
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
	return mhd_sta_set_powersave(power_save_level, 200);
}

int wifi_ps_cfg(uint8_t ps_mode, uint8_t dtim, uint8_t *wakeup_data)
{
	return mhd_sta_set_bcn_li_dtim(dtim);
}

/**
 * @brief  wifi_get_mode, get wifi mode
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_get_mode()
{
	return 0;
}

/**
 * @brief  wifi_set_mode, set wifi mode.
 * ap_wifi_dev and sta_wifi_dev are two different device, set mode just set the global variable wifi_dev
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_set_mode(int type)
{
    if(WIFI_MODE_STA == type) {
        printk("wifi destroy ap\n");
        wifi_destroy_ap();
    } else if(WIFI_MODE_AP == type ){
        printk("wifi disconnect\n");
        wifi_disconnect();
    } else {
        printk("wifi set mode unsupport\n");
        return -1;
    }
    
	return 0;
}


/**
 * @brief  wifi_isconnected, return wifi connect status
 * @param  : 
 * @retval 1 - connect , 0 - not connected 
 */
//TODO:
int wifi_isconnected()
{
	return mhd_sta_get_connection();
}

/**
 * @brief  wifi_connect, wifi connect to ap 
 * @param  : ssid, key
 * @retval 0 - send connect msg ok , -1 - some error happen 
 */

int wifi_connect(char *essid, char *key)
{
#if 0
	return mhd_sta_up(essid, key); //with ip
#else
	return mhd_sta_setup(essid, key); //without ip
#endif
}


/**
 * @brief  wifi_disconnect, wifi disconnect from ap 
 * @param  : ssid, key
 * @retval 0 - send connect msg ok , -1 - some error happen 
 */

int wifi_disconnect()
{
	return mhd_sta_down();
}

/**
 * @brief  wifi_create_ap, destroy
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_create_ap(struct _apcfg *ap_cfg)
{
	if (ap_cfg->enc_protocol == KEY_WPA2)
		ap_cfg->enc_protocol = SECURITY_WPA2_AES_PSK;

#if 0
	int ip_addr = htonl(inet_addr("192.168.0.1"));
	return mhd_softap_start(ap_cfg->ssid, ap_cfg->key, ap_cfg->enc_protocol, ap_cfg->channel, ip_addr); //with ip
#else
	return mhd_softap_setup(ap_cfg->ssid, ap_cfg->key, ap_cfg->enc_protocol, ap_cfg->channel); //without ip
#endif
}

/**
 * @brief  wifi_destroy_ap, destroy
 * @param  : 
 * @retval AK_WIFI_MODE
 */

int wifi_destroy_ap(void)
{
	return mhd_softap_stop(0);
}

#ifndef MAX_TX_PENDING_CTRL

int wifi_get_tx_pending()
{
	return 0;
}

int wifi_set_max_tx_pending(unsigned char max_tx_pending)
{
	return 0;
}

int wifi_get_max_tx_pending()
{
	return 100;
}
#endif


/**
 * @brief initializing wifi 
 * @author
 * @date 
 * @param [in] pParam a pointer to T_WIFI_INITPARAM type
 * @return int
 * @retval   0  initializing sucessful
 * @retval  -1 initializing fail
 */
static volatile char wifi_init_flag = 0;
int wifi_init(void *init_param)
{
	int result;
	unsigned int data[2] = {0};
	char *buffer;
	int buff_size;
	unsigned char wifi_backup_flg;
	
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

	
	if(NULL == init_param)
	{
		 wifi_backup_flg = 0;
	}
	else
	{
		 wifi_backup_flg = *(unsigned char*)init_param;
	}
		
	mhd_set_wlregon_pin(wifi->gpio_reset.nb); //14
	mhd_set_oob_pin(wifi->gpio_int.nb);     //15

    if(0 == wifi_backup_flg)
    {
        printk("==init=\r\n");
    	result = mhd_module_init();
		wifi_init_flag = 1;
	}
	else
	{
        printk("==recover: %d=\r\n", get_tick_count());
	    //sdio_restore(SDIO_PARTI);
		if(!sdio_backup_flash_value(0, data, 2))
		{
			printk("Error sdio_backup_flash_value read!\n");
		}
		else
		{
			//sdio_backup(SDIO_PARTI);
			buff_size = data[0]|(data[1]<<8);
			if(0 != buff_size)
			{
				buffer = malloc(buff_size + 2); //mode & back size				
				memcpy(buffer+2, buffer, buff_size);
				sdio_backup_flash_value(0, buffer, buff_size+2);
		
				sdio_backup_set_data(buffer+2,buff_size);
				free(buffer);
			}

		}
		
		sdio_set_clock(wifi->clock, get_asic_freq(), 1); //	SD_POWER_SAVE_ENABLE
		
        wifi_backup_flash_value(0, data, 2);
		
		buff_size = data[0]|(data[1]<<8);

		buffer = malloc(buff_size+2);

		wifi_backup_flash_value(0, buffer, buff_size+2);
				
		mhd_deep_sleep_restore_data(buffer+2);

		free(buffer);

    	result = mhd_module_resume();
		if (result == 0)
		{
			if (mhd_wowl_sta_is_enable())
			{
				s_wifi_wakeup_status = mhd_wowl_sta_check_status();
				mhd_wowl_sta_disable();
				mhd_sta_set_powersave(0, 0); // 0


                printk("==r2: %d, status = 0x%x=\r\n", get_tick_count(), s_wifi_wakeup_status);
                #if 0
		        if (wifi_isconnected())
		        {
		            wifistation_netif_init();
                    printk("==r3: %d=\r\n", get_tick_count());

                    printk("[ka: stop]\n");
                    mhd_tcpka_stop();
		        }
				#endif
				mhd_tcpka_stop();
			}
			else
			{
				if (wifi_isconnected())
		        {
					printk("if device has connected ap, it must enter wowl mode\r\n");
				}
			}
		}
	}
	
    if ( result != 0 )
    {
		printk("Error %d while init wifi!\n", result);
    }
	else
	{
		printk("init wifi OK!\n");
	}
	dev_close(fd);
	return result;
}

int wifi_module_resume(void)
{
	return mhd_module_resume();
}





int wifi_sleep()
{
	char *buffer;
	char *backup_data;
	int backup_size;
	int ret = 0;
	if(1 == wifi_init_flag)
	{
		//del
		mhd_wowl_sta_del(wifi_power_save.wakeup_data, wifi_power_save.wakeup_data_offset);

		//add
		mhd_wowl_sta_add(wifi_power_save.wakeup_data, wifi_power_save.wakeup_data_offset);
	}
	
    if(s_wifi_tcpka > 0)
    {
        printk("set dtim to %d\n", WIFI_DTIM_INTERVAL);
        mhd_sta_set_bcn_li_dtim(WIFI_DTIM_INTERVAL);
    }
    else
    {
        printk("set dtim to 1\n");
        mhd_sta_set_bcn_li_dtim(1);
    }

	//on
	mhd_wowl_sta_enable();

	if(!sdio_backup_get_data(&backup_data,&backup_size))
	{
		printk("Error sdio_backup_get_data!\n");
	}
	else
	{
		//sdio_backup(SDIO_PARTI);

		buffer = malloc(backup_size + 2); //mode & back size
		buffer[0] = backup_size&0xff;
		buffer[1] = (backup_size>>8)&0xff;

		memcpy(buffer+2, backup_data, backup_size);		
		sdio_backup_flash_value(1, buffer, backup_size+2);
		
		free(buffer);
	}



	backup_size = mhd_deep_sleep_backup_size();
	backup_data = mhd_deep_sleep_backup_data();

	buffer = malloc(backup_size + 2); //mode & back size
	buffer[0] = backup_size&0xff;
	buffer[1] = (backup_size>>8)&0xff;
	memcpy(buffer+2, backup_data, backup_size);

	wifi_backup_flash_value(1, buffer, backup_size+2);
	free(buffer);
	
	return ret;
	
}




int wifi_wakeup()
{
	return 0;
}

int wifi_keepalive_set(keep_alive_t *keep_alive)
{
    typedef struct tcpka_timer {
        uint16_t interval;          /* interval timer */
        uint16_t retry_interval;    /* retry_interval timer */
        uint16_t retry_count;       /* retry_count */
    } tcpka_timer_t;

    int ret;

    tcpka_timer_t timer;
    char payload[] = "keepalive\n";

    //keep alive stop
    if(keep_alive->enable == 0)
    {
        printk("[ka: stop]\n");
        mhd_tcpka_stop();
        return 0;
    }

    //check mode,
    if(keep_alive->mode == 0)
    {
        printk("currently not support make another connection mode.\n");
        return -1;
    }

    timer.interval = keep_alive->send_interval;
    timer.retry_interval = keep_alive->retry_interval;
    timer.retry_count = keep_alive->retry_count;
    
    printk("[ka start: %d, %d, %d]\n", timer.interval, timer.retry_interval, timer.retry_count);
    
    ret = mhd_tcpka_start(keep_alive->socket, payload, &timer);
    if(0 == ret)
    {
        s_wifi_tcpka = 1;
        printk("[ka: ok]\n");
        
        return 0;
    }
    else
    {
        s_wifi_tcpka = 0;
        printk("[ka: fail]\n");

        return -1;
    }
}

//0: normal wakeup, -1: wakeup abnormal, 
int wifi_check_wakeup_status(void)
{
    if(s_wifi_wakeup_status == WIFI_WAKEUP_BEACON_LOST || s_wifi_wakeup_status == WIFI_WAKEUP_KEEPALIVE_TIMEOUT)
    {
        return -1;
    }

    return 0;
}

