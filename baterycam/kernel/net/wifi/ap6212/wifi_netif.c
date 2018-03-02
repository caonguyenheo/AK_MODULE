/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "netif/etharp.h"
#include "lwip/tcpip.h"
#include "libc_mem.h"

#ifdef ANYKA_DEBUG
#define akp_info(...) do{printf(__VA_ARGS__) ;}while(0)
#define akp_err(...) do{printf("err:");printf(__VA_ARGS__);}while(0)
#define akp_debug(...) do{printf(__VA_ARGS__); }while(0)
#define akp_debug_enter do{printf("enter %s\n", __func__ ); printf("\r\n");}while(0)
#define akp_debug_exit do{printf("exit %s\n", __func__ ); printf("\r\n");}while(0)

#else
#define akp_info(...) do{printf(__VA_ARGS__) ;}while(0)
#define akp_err(...) do{printf("err:");printf(__VA_ARGS__);}while(0)

#define akp_debug(...)
#define akp_debug_exit
#define akp_debug_enter
#endif





int wifi_netif_init()
{
	struct ip_addr ipaddr, netmask, gw;

	const char * IP_ADDR = 	"192.168.0.1";
	const char * GW_ADDR = 	"192.168.0.1";
	const char * MASK_ADDR = 	"255.255.255.0";
	
	gw.addr = inet_addr(GW_ADDR);
	ipaddr.addr = inet_addr(IP_ADDR);
	netmask.addr = inet_addr(MASK_ADDR);

	return mhd_softap_ipup(htonl(ipaddr.addr), htonl(netmask.addr), htonl(gw.addr));
}


int wifistation_netif_init()
{
	struct ip_addr ipaddr, netmask, gw;
	unsigned int tick;
	
	gw.addr =  0;
	ipaddr.addr = 0;
	netmask.addr = 0;

	return mhd_sta_ipup(htonl(ipaddr.addr), htonl(netmask.addr), htonl(gw.addr));
}

//TODO: set dhcp

//int dhcp_setup()





