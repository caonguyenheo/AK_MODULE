/**
 * @file 
 * @brief: 
 *
 * This file describe 
 * Copyright (C) 2010 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author  
 * @date    2017-9-15
 * @version 1.0
 */

#ifndef _WIFI_BACKUP_COMMON_H_
#define _WIFI_BACKUP_COMMON_H_

#include "lwip/ip_addr.h"
#include "wifi.h"

#ifdef __cplusplus
extern "C" {
#endif



#define FLAG_PARTI   			"FLAGPA"
#define SDIO_PARTI   			"SDIOPA"

#define WIFI_IP_PARTI			 "WIFIIP"
#define SERVER_IP_PARTI			 "SERVER"


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

unsigned char wifi_backup_flash_value(char mode, char *data, int len);

unsigned char sdio_backup_flash_value(char mode, char *data, int len);


/**
 * backup IP address configuration for a network interface (including netmask
 * and default gateway).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */
int wifi_back_ip_info(struct ip_info *t_ip_info);
/**
 * resrote IP address configuration for a network interface (including netmask
 * and default gateway).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */
int wifi_restore_ip_info(struct ip_info *t_ip_info);


/*@}*/
#ifdef __cplusplus
}
#endif

#endif //#ifndef _WIFI_BACKUP_COMMON_H_




