
/**
 * @file 
 * @brief:
 *
 * This file provides 
 * Copyright (C) 2017 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author 
 * @date 2017-2-09
 * @version 1.0
 */

#include "arch_init.h"
 
#include "platform_devices.h"
#include "dev_drv.h"

#ifdef __cplusplus
extern "C"{
#endif

#define INVALID_GPIO                    0xfe

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif
static  T_CAMERA_PLATFORM_INFO camera_platform_info = {
	//sensor information
	.sensor_info = {
		.gpio_reset = {
				.nb        = 5,	
				},
				
		.gpio_avdd = {
					.nb        = 0,	
					},
					
		.gpio_scl = {
					.nb        = 27,	
					},
					
		.gpio_sda = {
					.nb        = 28,	
					},	
	},
	
	//ircut information
	.ircut_info  = {
		.gpio_ctrl = {
				.nb        = INVALID_GPIO,	
				},
				
		.gpio_dectect = {
				.nb        = 0,	
				},

	},

	//irfeed gpio information
	.irfeed_gpio_info  = {
		.logic_level_day = 1,// 0 or 1
		.gpio = {
				.nb			= INVALID_GPIO,//INVALID_PIN,//47,
				.in			= 1,
				},
	},
};


T_PLATORM_DEV_INFO camera_platform_dev = {
	.dev_name = DEV_CAMERA,
	.devcie   =&camera_platform_info,	
};


//speaker information
static  T_SPEAKER_INFO speaker_platform_info = {
	.gpio_attribute = {
				.nb        = 31,
				.high      = 1,
				.out	   = 1,
				},
};

T_PLATORM_DEV_INFO speaker_platform_dev = {
	.dev_name = DEV_SPEAKER,
	.devcie   =&speaker_platform_info,	
};

//i2c information
static T_I2C_INFO i2c_platform_info = {

	.i2c_mode = 0,
		
	.gpio_sda = {
				.nb        = 28,	
				},
	.gpio_scl = {
				.nb        = 27,	

	},
};
T_PLATORM_DEV_INFO i2c_platform_dev ={
	.dev_name = DEV_I2C,
	.devcie   =&i2c_platform_info,
};

//UART information
static T_UART_INFO uart_platform_info = {
	.baudrate = 115200,
	.uart_id       = 1,	//´®¿Ú1
};

//uart for commucation
T_PLATORM_DEV_INFO uart_platform_dev = {
	.dev_name = DEV_UART_1,
	.devcie   =&uart_platform_info,	
};

//wifi information
static T_WIFI_INFO wifi_platform_info =  {
	.sdio_share_pin =  1,
		
	.gpio_power     =  {
						.nb        = 14,	
					},
					
	.gpio_int       =  {
						.nb        = 23,	
					},
	.interface      =  1,
	.bus_mode       =  1,
};

T_PLATORM_DEV_INFO wifi_platform_dev = {
	.dev_name = DEV_WIFI,
	.devcie   =&wifi_platform_info,	
};


//
static T_PLATORM_DEV_INFO *ak39_evt_platform_devices[] = {
	&uart_platform_dev,
	&speaker_platform_dev,
	&camera_platform_dev,
	&wifi_platform_dev,
	&i2c_platform_dev,
};




static int set_devices_info(void)
{

	/* register platform devices */
	platform_add_devices_info(ak39_evt_platform_devices, ARRAY_SIZE(ak39_evt_platform_devices));
    return 0;
}


module_init(set_devices_info)


#ifdef __cplusplus
}
#endif




