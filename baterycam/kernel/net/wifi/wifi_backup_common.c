
/**
 * @file 
 * @brief 
 *
 * This file provides 
 * Copyright (C) 2016 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author 
 * @date 2017-9-15
 * @version 1.0
 */


#include "anyka_types.h"
#include "partition_port.h"
#include "wifi_backup_common.h"


unsigned char wifi_backup_flash_value(char mode, char *data, int len)
{
	//char *flag_parti = "FLAGPA";
	void *handle = NULL;
	int ret;

	if (data == NULL)
		return;

	handle = kernel_partition_open(FLAG_PARTI);
	if(handle == NULL)
	{
		printk("open %s error\n", FLAG_PARTI);
        //memset(data, 0, len);
        ret = false;
	}
	else
	{
		if (mode == 1)
		{
			ret = kernel_partition_write(handle, data, len);
			if(ret < 0)
			{
				printk("write %s error\n", FLAG_PARTI);
                ret = false;
			}
			else
			{
				printk("write %s ok\n", FLAG_PARTI);
                ret = true;
			}
		}
		else if (mode == 0)
		{
			ret = kernel_partition_read(handle, data, len);
			if(ret < 0)
			{
				printk("read %s error\n", FLAG_PARTI);
                ret = false;
			}
			else
			{
				printk("read %s ok\n", FLAG_PARTI);
                ret = true;
			}
		}

		kernel_partition_close(handle);
	}

	return ret;
}


unsigned char sdio_backup_flash_value(char mode, char *data, int len)
{
	void *handle = NULL;
	int ret;

	if (data == NULL)
		return false;

	handle = kernel_partition_open(SDIO_PARTI);
	if(handle == NULL)
	{
		printk("open %s error\n", SDIO_PARTI);
        //memset(data, 0, len);
        ret = false;
	}
	else
	{
		if (mode == 1)
		{
			ret = kernel_partition_write(handle, data, len);
			if(ret < 0)
			{
				printk("write %s error\n", SDIO_PARTI);
                ret = false;
			}
			else
			{
				printk("write %s ok\n", SDIO_PARTI);
                ret = true;
			}
		}
		else if (mode == 0)
		{
			ret = kernel_partition_read(handle, data, len);
			if(ret < 0)
			{
				printk("read %s error\n", SDIO_PARTI);
                ret = false;
			}
			else
			{
				printk("read %s ok\n", SDIO_PARTI);
                ret = true;
			}
		}

		kernel_partition_close(handle);
	}

	return ret;
}



/**
 * backup IP address configuration for a network interface (including netmask
 * and default gateway).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */

int wifi_back_ip_info(struct ip_info *t_ip_info)
{
	void *handle = NULL;
	int ret = -1;
	char *flag_parti = WIFI_IP_PARTI;

	handle = kernel_partition_open(flag_parti);
	if(handle == NULL)
	{
		printk("open %s error\n", flag_parti);
	}
	else
	{
		ret = kernel_partition_write(handle, t_ip_info, sizeof(struct ip_info));
		if(ret < 0)
		{
			printk("write %s error\n", flag_parti);
		}
		else
		{
			printk("back IP info on %s OK\n", flag_parti);
		}
		kernel_partition_close(handle);
	}
	return ret;	
}



/**
 * resrote IP address configuration for a network interface (including netmask
 * and default gateway).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */
int wifi_restore_ip_info(struct ip_info *t_ip_info)
{
	void *handle = NULL;
	int ret = -1;
	char *flag_parti = WIFI_IP_PARTI;

	handle = kernel_partition_open(flag_parti);
	if(handle == NULL)
	{
		printk("open %s error\n", flag_parti);
	}
	else
	{
		ret = kernel_partition_read(handle, t_ip_info, sizeof(struct ip_info));
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




