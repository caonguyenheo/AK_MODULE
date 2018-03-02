/*
* photo.h
*/

#ifndef __TEST_WIFI_CFG_H__
#define __TEST_WIFI_CFG_H__

#include <stdint.h>
#include "wifi.h"
#include "command.h"
#include "ak_common.h"
#include "ak_thread.h"
#include "ak_error.h"
#include "lwip/ip_addr.h"
#include "wifi_backup_common.h" 

typedef struct net_sys_cfg
{
	unsigned char ssid[MAX_SSID_LEN];
	unsigned char key[MAX_KEY_LEN];
	unsigned int  server_ip;
	unsigned short  port_num;
}net_sys_cfg_t;

int test_wifi_init(void);

int test_wifi_get_config(struct net_sys_cfg *t_server_info);
int test_wifi_get_ip(struct ip_info *p_ip_info);

#endif
