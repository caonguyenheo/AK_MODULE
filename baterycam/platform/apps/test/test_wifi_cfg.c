/*******************************************************************
 * @brief  a module to config wifi 
 
 * @Copyright (C), 2016-2017, Anyka Tech. Co., Ltd.
 
 * @Modification  2017-9 create 
*******************************************************************/
#include "wifi_backup_common.h" 
#include "test_wifi_cfg.h"

#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "dev_info.h"
#include "kernel.h"

#include "drv_gpio.h"


/**
 * get configuration for camview (including ssid password ip port and port).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */
int test_wifi_get_config(struct net_sys_cfg *t_server_info)
{
	void *handle = NULL;
	int ret = AK_SUCCESS;
	
	
	if((handle = ak_ini_init("APPINI")) == NULL)
	{
		// ak_print_error_ex("open %s error\n", server_parti);
		return AK_FAILED;
	}

	ret = ak_ini_get_item_value(handle, "test", "test_wifi_ssid", t_server_info->ssid);
	ret |= ak_ini_get_item_value(handle, "test", "test_wifi_pwd", t_server_info->key);
	if(ret < 0)
		ret = AK_FAILED;


ERR:
	ak_ini_destroy(handle);
	return ret;	
}

int test_wifi_init(void)
{
    int init = 0;

    if(wifi_init(&init) == 0)  {
        ak_print_normal("init wifi ok!!!\n");
        return AK_SUCCESS;
    } 

	ak_print_error("init wifi error!!!\n");
    return AK_FAILED;
}

int test_wifi_get_ip(struct ip_info *p_ip_info)
{

	void *handle = NULL;
	int ret = AK_FAILED;
	char *flag_parti = WIFI_IP_PARTI;

	handle = ak_partition_open(flag_parti);
	if(handle == NULL)
	{
		ak_print_error("open %s error\n", flag_parti);
	}
	else
	{
		ret = ak_partition_read(handle, p_ip_info, sizeof(struct ip_info));
		if(ret < 0)
		{
			printk("read %s error\n", flag_parti);
		}
		else
		{
			printk("read %s OK\n", flag_parti);
		}
		kernel_partition_close(handle);
	}
	return ret;	
}



