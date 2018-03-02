/************************************************************************
 * Copyright (c) 2016, Anyka Co., Ltd. 
 * All rights reserved.	
 *  
 * File Name：netdemo.c
 * Function：This file is demo for network.
 *
 * Author：lx
 * Date：
 * Version：
**************************************************************************/

#define ANYKA_DEBUG 

#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "string.h"

#include "ak_thread.h"
#include "ak_common.h"
#include "kernel.h"

#include "lwip/tcpdump.h"
 

typedef struct _sock_cfg{
  unsigned long  ipaddr;
  unsigned short port;
  int 			 sock_id;
}sock_cfg_t;

struct netrecv_arg
{
	sock_cfg_t       sock;
    unsigned long    runtime;   //second
};

#define NETSEND_STRING_LIMIT  (32) //Byte
struct netsend_arg
{
	sock_cfg_t       sock;
    unsigned long    count; 
	char             string[NETSEND_STRING_LIMIT];
};

typedef struct _speed_test_arg
{
	sock_cfg_t  sock;
	unsigned int pkt_size;
	unsigned int duriation;
	unsigned int protocol;
}speed_test_arg_t;


#define UDPRECV_LOCAL_PORT     (4900)
#define TCPRECV_LOCAL_PORT     (4900)

#define UDPSEND_REMOTE_PORT    (4900)
#define TCPSEND_REMOTE_PORT    (4900)

#define NETRECV_RUNTIME        (60) //second
#define NETSEND_REMOTE_ADDR    "192.168.0.2"
#define NETSEND_COUNT_DEFAULT  (10)
#define NETSEND_STRING_DEFAULT "netsend_hello."

#define PARAM_UDP_PROTOCOL     "udp"
#define PARAM_TCP_PROTOCOL     "tcp"

#define RECVBUF_TEMPSIZE       (10*1024)

#define SPEED_TASK_PRIO 		80


//wmj- for gcc 4.4.1 not support TLS
//#define errno 1
//extern int lwip_errno;


static char *help_recv[]={
	"udp/tcp recvdata demo.",
	"usage: netrecv [udp/tcp] [time_second] <local_port> \n"
};

static char *help_send[]={
	"udp/tcp senddata demo.",
	"usage: netsend [udp/tcp] [string] [send_count] <remote_ipaddr> <remote_port>\n"
};

static char *help_speed_test[]={
	"speedtest demo",
	"usage: speedtest [pkg_size] [time seconds] <remote_ipaddr> <remote_port> <udp/tcp> <test_times>\n"
};

static char *help_tcpdump[]={
	"tcpdump",
	"usage: tcpdump [start/stop] <buff_size KB/file name>\n"
};
static char *help_iperf3[]={
	"iperf3",
	"usage: throughput testing via tcp or udp \n"
};
static char *help_lwipstats[]={
	"lwipstats",
	"usage: display lwip stats\n"
};

static int create_udp_socket(void)
{
   
	int socket_s = -1;
	
	socket_s = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_s < 0)
	{
		ak_print_error("create socket error.\n");
	}
	return socket_s;
}

static int create_tcp_socket(void)
{
   
	int socket_s = -1;
	
	socket_s = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_s < 0)
	{
		ak_print_error("create socket error.\n");
	}
	return socket_s;
}

static int connect_tcp_server(sock_cfg_t * sock_arg)
{
   
	
	struct sockaddr_in net_cfg;
	int socket_s = -1;
	int err = -1;
	
	
	if (NULL == sock_arg)
	{
	    ak_print_error("tcp socket arg error.\n");
		return err;
	}

	net_cfg.sin_family = AF_INET;
	net_cfg.sin_len = sizeof(struct sockaddr_in);
	net_cfg.sin_port = htons(sock_arg->port);
	net_cfg.sin_addr.s_addr = sock_arg->ipaddr;

	socket_s = sock_arg->sock_id;//tcp socket
	if (socket_s < 0)
	{
		ak_print_error("create socket error.\n");
	}
	else
	{
		err = connect(socket_s, (struct sockaddr*)&net_cfg, sizeof(struct sockaddr_in));
		if (err != 0)
		{
			ak_print_error("connect socket error\n");
		}
		
	}
	return err;
}


/////////////////////////////udp recv ////////////////////////////////////
static void udprecv(void *argv)
{
    struct netrecv_arg *nr_arg;
	int socket_s = -1;
	int err = 0;
	struct sockaddr_in net_cfg;
	struct sockaddr remote_addr;
	unsigned char rcv_buff[RECVBUF_TEMPSIZE];
	socklen_t retval;
	unsigned long starttick;
	unsigned long endtick;
	unsigned long tick;
	int recvlen;
	unsigned long total_len, total_err;
	
	
	nr_arg = (struct netrecv_arg *)argv;
	if (NULL == nr_arg)
	{
	    ak_print_error("udprecv argv error.\n");
		return;
	}

	memset(&net_cfg, 0, sizeof(struct sockaddr_in));

	net_cfg.sin_family = AF_INET;
	net_cfg.sin_port = htons(nr_arg->sock.port);
	net_cfg.sin_addr.s_addr = INADDR_ANY;
	
	total_len = 0;
	total_err = 0;

	socket_s = nr_arg->sock.sock_id;
	
	if (socket_s < 0)
	{
		ak_print_error("socket id error.\n");
	}
	else
	{
		err = bind(socket_s, (struct sockaddr*) &net_cfg, sizeof(struct sockaddr_in));
		if (err !=0)
		{
			ak_print_error("bind socket error\n");
		}
		else
		{
		    tick = (nr_arg->runtime) * 1000;
		    starttick = endtick = get_tick_count();

			ak_print_normal("start udp server on port %d runtime %d\n",  nr_arg->sock.port, nr_arg->runtime);
			
			while(endtick - starttick < tick)
			{
				recvlen = recvfrom(socket_s, rcv_buff, sizeof(rcv_buff) - 1, MSG_WAITALL, &remote_addr, (socklen_t*) &retval);
				if (recvlen > 0)
				{
					rcv_buff[recvlen]=0;
					total_len += recvlen;
					ak_print_normal("recv len=%d\n",(int)recvlen);		
				}
				else
				{
					ak_print_error("recv error.\n");
					total_err++;
					break;
			    }
				endtick = get_tick_count();
			}
			ak_print_normal("%ld bytes data received, speed %dKB/s, %d times recv error\n", total_len, total_len / (endtick - starttick), total_err);
		}
		if (-1 != socket_s)
		{
			//closesocket(socket_s);
		}
	}
	ak_thread_exit();
}

//////////////////////udp send ///////////////////////////////////////////////////////////////////
static void udpsend(void *argv)
{
    struct netsend_arg *ns_arg;
	struct sockaddr_in net_cfg;
	int socket_s = -1;
	unsigned long  sendcnt=0;
	int send_len = 0;
	unsigned long total_len, total_err;

	ns_arg = (struct netsend_arg *)argv;
	if (NULL == ns_arg)
	{
	    ak_print_error("udpsend argv error.\n");
		return;
	}

	net_cfg.sin_family = AF_INET;
	net_cfg.sin_len = sizeof(struct sockaddr_in);
	net_cfg.sin_port = htons(ns_arg->sock.port);
	net_cfg.sin_addr.s_addr = ns_arg->sock.ipaddr;

	socket_s = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_s < 0)
	{
		ak_print_error("get socket err\n");
		return;
	} 
	
	total_len = 0;
	total_err = 0;
	sendcnt = ns_arg->count;
	
	while (sendcnt-- > 0)
	{
	    ak_print_normal("send data=[%s]\n",ns_arg->string);
		send_len = sendto(socket_s, ns_arg->string, strlen(ns_arg->string), 0, (struct sockaddr*)&net_cfg, sizeof(struct sockaddr_in));
		if (send_len <= 0)
		{
			ak_print_error("send error\n");
			total_err++;
			break;
		}
		total_len += send_len;
		ak_sleep_ms(50);
	}
	ak_print_normal("%ld bytes data sent success, %d times send error\n", total_len, total_err);
	shutdown(socket_s, SHUT_RDWR);
	if (-1 != socket_s)
	{
		closesocket(socket_s);
	}
}

/////////////////////////////tcp recv ////////////////////////////////////
static void tcp_server_recv(void *argv)
{
    struct netrecv_arg *nr_arg;
	int socket_s = -1;
	int new_socket = -1;
	int err = 0;
	int sockaddr_len;
	int len;
	struct sockaddr_in net_cfg;
	struct sockaddr remote_addr;
	unsigned char rcv_buff[RECVBUF_TEMPSIZE];
	socklen_t retval;
	int size;
	unsigned long starttick;
	unsigned long endtick;
	unsigned long tick;
	int opt;
	unsigned long total_len, total_err;

	nr_arg = (struct netrecv_arg *)argv;
	if (NULL == nr_arg)
	{
	    ak_print_error("tcprecv argv error.\n");
		return;
	}

	memset(&net_cfg, 0, sizeof(struct sockaddr_in));

	net_cfg.sin_family = AF_INET;
	net_cfg.sin_port = htons(nr_arg->sock.port);
	net_cfg.sin_addr.s_addr = INADDR_ANY;

	total_len = 0;
	total_err = 0;

	//socket_s = socket(AF_INET, SOCK_STREAM, 0);//tcp socket
	socket_s = nr_arg->sock.sock_id;
	if (socket_s < 0)
	{
		ak_print_error("create socket error.\n");
	}
	else
	{
	    //使server 能立即重用
		opt = SOF_REUSEADDR;
		if (setsockopt(socket_s, SOL_SOCKET, SOF_REUSEADDR, &opt, sizeof(int)) != 0) 
		{
			printf("set opt error\n");
		}
        else
        {
			err = bind(socket_s, (struct sockaddr*) &net_cfg, sizeof(struct sockaddr_in));
			if (err !=0)
			{
				ak_print_error("bind socket error\n");
			}
			else
			{
				err = listen(socket_s, 4);
				if (err !=0 )
				{
					ak_print_error("listen socket error\n");
				}
	            else
	            {
	                sockaddr_len = sizeof(struct sockaddr);
					new_socket = accept((int)socket_s, (struct sockaddr*) &remote_addr, (socklen_t*) &sockaddr_len);
					if (new_socket <0)
					{
						ak_print_error("accept socket error\n");
					}
	                else
	                {
	                    ak_print_debug("remote addr:%s.\n", remote_addr.sa_data);
						
						ak_print_normal("recv data:\n");
					    tick = (nr_arg->runtime) * 1000;
						starttick = endtick = get_tick_count();
						
						while(endtick - starttick < tick)
						{
							len = recv(new_socket, rcv_buff, sizeof(rcv_buff)-1,MSG_WAITALL);
							if (len > 0)
							{
								total_len += len;
								rcv_buff[len]=0;
								ak_print_normal("recv len=%d\n",(int)len);
							}
							else
							{
								ak_print_error("recv error.\n");
								total_err++;
								break;
							}
							endtick = get_tick_count();
						}
						ak_print_normal("%ld bytes data received, speed %dKB/s, %d times recv error\n", total_len, total_len / (endtick - starttick), total_err);
	                }
				}
			}
        }
	}
	
	if (-1 != new_socket)
	{
		closesocket(new_socket);
	}
	if (-1 != socket_s)
	{
		closesocket(socket_s);
	}	
}


/////////////////////////////tcp send ////////////////////////////////////
static void tcp_client_send(void *argv)
{
    struct netsend_arg *ns_arg;
	struct sockaddr_in net_cfg;
	int socket_s = -1;
	unsigned long  sendcnt=0;
	int send_len = 0;
	int err = -1;
	unsigned long total_len, total_err;

	ns_arg = (struct netsend_arg *)argv;
	if (NULL == ns_arg)
	{
	    ak_print_error("tcpsend argv error.\n");
		return;
	}

	net_cfg.sin_family = AF_INET;
	net_cfg.sin_len = sizeof(struct sockaddr_in);
	net_cfg.sin_port = htons(ns_arg->sock.port);
	net_cfg.sin_addr.s_addr = ns_arg->sock.ipaddr;

	total_len = 0;
	total_err = 0;

	socket_s = socket(AF_INET, SOCK_STREAM, 0);//tcp socket
	if (socket_s <0)
	{
		ak_print_error("create socket error.\n");
	}
	else
	{
		err = connect(socket_s, (struct sockaddr*)&net_cfg, sizeof(struct sockaddr_in));
		if (err !=0)
		{
			ak_print_error("connect socket error\n");
		}
		else
		{
			
			sendcnt = ns_arg->count;
			while (sendcnt-- > 0)
			{
				ak_print_normal("send data=[%s]\n",ns_arg->string);
				send_len = send(socket_s, ns_arg->string, strlen(ns_arg->string), MSG_WAITALL);
				if (send_len < 0)
				{
					ak_print_error("send error\n");
					total_err++;
					break;
				}
				total_len += send_len;
				ak_sleep_ms(50);
			}
			ak_print_normal("%ld bytes data sent success, %d times send error\n", total_len, total_err);
		}
	}
	
	if (-1 != socket_s)
	{
		closesocket(socket_s);
	}	
}


/*******************************************************
 * @brief udp test: udp send data
 * @author
 * @date 
 * @param [in]  ipaddr 
 * @param [in]  port
 * @param [in]  pkt_size
 * @param [in]  duriation
 * @return void
 *******************************************************/
#define SPEED_TEST_REMOTE_PORT  	(4801)
#define SPEED_TEST_REMOTE_ADDR  	"192.168.1.2"
#define WIFI_RESERVE_MEMORY    	(10<<20)  //reserve for wifi working.
#define UDP_MTU  				(1460) //wmj+
#define MAX_SEND_BUF 			(64*1024)
#define PROTOCOL_UDP            17
#define PROTOCOL_TCP            6


//static void udp_speed(unsigned long  ipaddr, unsigned short port, unsigned int pkt_size, unsigned int duriation)
static void speed_udp_send(void *param)
{
	int socket_s = -1;
	struct sockaddr_in 	addr;
	unsigned long       	tick_start, tick_end, tick_stat;
	unsigned long      	send_time_count;
	int                	send_len;
	unsigned long      	send_count;;
	unsigned long  		send_count_total;
	unsigned int   		print_count;;
	unsigned char       *sendBuf;
	int 				tx_pending = 0, max_tx_pending;//wmj+
	speed_test_arg_t	    *speed_arg = (speed_test_arg_t*)param;
	
	if(speed_arg->pkt_size > MAX_SEND_BUF)
	{
		ak_print_error("pkt_size larger than the max len %d\n", MAX_SEND_BUF);
		return;
	}
	//wmj+ for queue limit
	max_tx_pending = wifi_get_max_tx_pending();
	ak_print_normal("----max tx pending %d-----\n", max_tx_pending);
	if( max_tx_pending > 0 && speed_arg->pkt_size > max_tx_pending * UDP_MTU)
	{
		ak_print_error("packet size %d is greater than the max queue %d * %d(MTU), some pkts may be droped by wifi driver\n", (int)speed_arg->pkt_size, max_tx_pending, UDP_MTU);
		ak_print_error("exit speedtest\n");
		return;
	}
	//wmj<<
	
	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_port = htons(speed_arg->sock.port);
	addr.sin_addr.s_addr = speed_arg->sock.ipaddr;

	socket_s = speed_arg->sock.sock_id;
	if (socket_s < 0)
	{
		ak_print_error("get socket error:%d");
		return;
	} 
	
	sendBuf = malloc(MAX_SEND_BUF);
	if(sendBuf == NULL)
	{
		ak_print_error("malloc buf for send error\n");
		goto sock_free_exit; 
	}
	/*fill buffer*/	
	for(int i = 0; i < MAX_SEND_BUF; i++)
	{
		sendBuf[i] = i % 26 + 65;  /*A - Z*/
	}
    ak_print_normal("memleft=%d\n", (int)Fwl_GetRemainRamSize());
	ak_print_normal("start send data to %s:%d in udp\n", inet_ntoa(speed_arg->sock.ipaddr), speed_arg->sock.port);
	ak_print_normal("data len=[%d], duriation=%d sec\n\n", speed_arg->pkt_size, speed_arg->duriation);
    
	send_count_total = 0;
	send_count = 0;
	tick_start = tick_end = tick_stat = get_tick_count();
	
	while((tick_end - tick_start) < speed_arg->duriation * 1000)
	{
		//wmj+ for tx pending test, the ramainning pkt queue should cover the sending pkg  
		//incase tx_pending greater than max_tx_pending , convert pkt_size to int to compare
		//reserve several pkts for some protocal packet such as arp to transmit is better. 
		//ie.  ((max_tx_pending - 5 - (tx_pending = wifi_get_tx_pending()))* (UDP_MTU) < (int)pkt_size)
		print_count = 0;
		while ((max_tx_pending > 0 && ((max_tx_pending - (tx_pending = wifi_get_tx_pending()))* (UDP_MTU) < (int)speed_arg->pkt_size))
			    || (Fwl_GetRemainRamSize() < WIFI_RESERVE_MEMORY))
		{
			print_count++;
			if (print_count % 10 == 0)
			{
				ak_print_normal("curr tx_pending %d, max_tx_pending is %d, memleft: %d waiting driver to process tx queue...\n",tx_pending, max_tx_pending, (int)Fwl_GetRemainRamSize());
			}
			if (print_count > 1000)
			{
			    ak_print_normal("long waiting memleft:%d..\n", (int)Fwl_GetRemainRamSize());
				break;
			}
			ak_sleep_ms(1);
		}
		
		//wmj<<
		send_len = sendto(socket_s, sendBuf, speed_arg->pkt_size, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
		if(send_len <= 0)
		{
			if(0)// (lwip_errno == ENOMEM) //ERR_MEM, it should be errno, but AKOS unsupport it
			{
				//ak_print_error("memleft:%d..%d\n", (int)Fwl_GetRemainRamSize(), lwip_errno); //ap6212 print this often, maybe the pbuf_pool_tx
				continue;
			}
			else
			{
				ak_print_error("send error\n");
				break;
			}
		}
		else
		{
			//ak_print_normal("memleft:%d..\n", (int)Fwl_GetRemainRamSize());
			send_count++;
			send_count_total++;
		}
		
		tick_end = get_tick_count();
		
		/*print statistic every 5 seconds*/
		if (tick_end - tick_stat >= 5000)
		{
			ak_print_normal("send_len %u Bytes, send speed = %uKB/s, memleft = %u \n", send_len
				, (send_count * send_len / (tick_end - tick_stat) ), Fwl_GetRemainRamSize() );
			send_count = 0;
			tick_stat = tick_end;
		}
	}
	if (tick_end - tick_start >= 1000) /* greate than 1 second, show statistics*/
	{
		ak_print_normal("speedtest end, duriation = %lu(s), packet count = %lu, total data = %lu(KB),speed=%u(KB/s)\n"
			, (tick_end - tick_start)/1000
			, send_count_total
			, ((speed_arg->pkt_size * send_count_total) / 1000) 
			, ((speed_arg->pkt_size * send_count_total) / (tick_end - tick_start)) );
	}
	else
	{
		ak_print_normal("send end, duriation less than 1 second\n");
	}
	free(sendBuf);
sock_free_exit:
//	shutdown(socket_s, SHUT_RDWR);
//	closesocket(socket_s);
	ak_print_normal("exit udp speed memleft:%u\n", Fwl_GetRemainRamSize());
    ak_thread_exit();
	
}

static void speed_tcp_recv(void *param)
{
	unsigned int       	tick_start, tick_end, tick_stat;
	int                 socket_s;
	unsigned char rcv_buff[RECVBUF_TEMPSIZE];
	unsigned long total_len, total_err;
	int len;
	speed_test_arg_t	    *speed_arg = (speed_test_arg_t*)param;
	

	if(NULL == speed_arg )
	{
		ak_print_error("argument error\n");
		goto task_exit;
	}
	printf("speed tcp socket id %d\n", speed_arg->sock.sock_id);
	socket_s = speed_arg->sock.sock_id;//tcp socket
	if(socket_s < 0)
	{
		ak_print_error("tcp socket id error.\n");
	}
	else
	{
		tick_start = tick_end = tick_stat = get_tick_count();
		total_len = 0;
			
		while((tick_end - tick_start) < speed_arg->duriation * 1000)
		{
			len = recv(socket_s, rcv_buff, RECVBUF_TEMPSIZE, MSG_WAITALL);
			if(len < 0)
			{
				total_err++;
				ak_print_error("tcp recv error.\n");
			}
			else
			{
				total_len += len;
				ak_print_normal("tcp recv %dbytes data.\n", len);
			}
			tick_end = get_tick_count();
			
		}
		ak_print_normal("time out exit tcp recv.\n");
		ak_print_normal("%ld bytes data received, speed %dKB/s, %d times recv error\n", total_len, total_len / (tick_end - tick_start), total_err);
		
	}
task_exit:
	ak_thread_exit();
	
}

/////////////////////////////tcp send ////////////////////////////////////
static void speed_tcp_send(void *param)
{
    int                 socket_s = -1;
	struct sockaddr_in 	addr;
	int 				send_len = 0;
	int					err = -1;
	unsigned int       	tick_start, tick_end, tick_stat;
	unsigned long      	send_count;;
	unsigned long  		send_count_total;
	unsigned int   		print_count;
	unsigned char 		*sendBuf;
	int 				tx_pending = 0, max_tx_pending;//wmj+
	int 				i;
	speed_test_arg_t	    *speed_arg = (speed_test_arg_t*)param;

	if(NULL == speed_arg )
	{
		ak_print_error("argument error\n");
		goto task_exit;
	}
	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_port = htons(speed_arg->sock.port);
	addr.sin_addr.s_addr = speed_arg->sock.ipaddr;

	if(speed_arg->pkt_size > MAX_SEND_BUF)
	{
		ak_print_error("pkt_size larger than the max len %d\n", MAX_SEND_BUF);
		return;
	}
	//wmj+ for queue limit
	max_tx_pending = wifi_get_max_tx_pending();
	ak_print_normal("----max tx pending %d-----\n", max_tx_pending);
	if( max_tx_pending > 0 && speed_arg->pkt_size > max_tx_pending * UDP_MTU)
	{
		ak_print_error("packet size %d is greater than the max queue %d * %d(MTU), some pkts may be droped by wifi driver\n", (int)speed_arg->pkt_size, max_tx_pending, UDP_MTU);
		ak_print_error("exit speedtest\n");
		return;
	}
	
	socket_s = speed_arg->sock.sock_id;//tcp socket
	if (socket_s < 0)
	{
		ak_print_error("tcp_speed socket id error.\n");
	}
	else
	{
		//err = connect(socket_s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
		//if (err !=0)
		//{
		//	ak_print_error("connect socket error\n");
		//}
		//else
		{
			
			sendBuf = malloc(MAX_SEND_BUF);
			if(sendBuf == NULL)
			{
				ak_print_error("malloc buf for send error\n");
				goto sock_free_exit; 
			}
			/*fill buffer*/	
			for(int i = 0; i < MAX_SEND_BUF; i++)
			{
				sendBuf[i] = i % 26 + 65;  /*A - Z*/
			}
			
			ak_print_normal("memleft=%d\n", (int)Fwl_GetRemainRamSize());
			ak_print_normal("start send data to %s:%d in tcp\n", inet_ntoa(speed_arg->sock.ipaddr), speed_arg->sock.port);
			ak_print_normal("data len=[%d], duriation=%d sec\n\n", speed_arg->pkt_size, speed_arg->duriation);
    
			send_count_total = 0;
			send_count = 0;
			tick_start = tick_end = tick_stat = get_tick_count();
			
			while((tick_end - tick_start) < speed_arg->duriation * 1000)
			{
				//wmj+ for tx pending test, the ramainning pkt queue should cover the sending pkg  
				//incase tx_pending greater than max_tx_pending , convert pkt_size to int to compare
				//reserve several pkts for some protocal packet such as arp to transmit is better. 
				//ie.  ((max_tx_pending - 5 - (tx_pending = wifi_get_tx_pending()))* (UDP_MTU) < (int)pkt_size)
				print_count = 0;
				while ((max_tx_pending > 0 && ((max_tx_pending - (tx_pending = wifi_get_tx_pending()))* (UDP_MTU) < (int)speed_arg->pkt_size))
					    || (Fwl_GetRemainRamSize() < WIFI_RESERVE_MEMORY))
				{
					print_count++;
					if (print_count % 10 == 0)
					{
						ak_print_normal("curr tx_pending %d, max_tx_pending is %d, memleft: %d waiting driver to process tx queue...\n",tx_pending, max_tx_pending, (int)Fwl_GetRemainRamSize());
					}

					if (print_count > 1000)
					{
					    ak_print_normal("long waiting memleft:%d..\n", (int)Fwl_GetRemainRamSize());
						break;
					}
					ak_sleep_ms(1);
				}
				send_len = send(socket_s, sendBuf, speed_arg->pkt_size, MSG_WAITALL);
				if (send_len < 0)
				{
					ak_print_error("send error\n");
					break;
				}
				send_count++;
				send_count_total++;
				tick_end = get_tick_count();
		
				/*print statistic every 5 seconds*/
				if (tick_end - tick_stat >= 5000)
				{
					ak_print_normal("send_len %u Bytes, send speed = %uKB/s, memleft = %u \n", send_len
						, (send_count * send_len / (tick_end - tick_stat) ), Fwl_GetRemainRamSize() );
					send_count = 0;
					tick_stat = tick_end;
				}
			}
			
			if (tick_end - tick_start >= 1000) /* greate than 1 second, show statistics*/
			{
				ak_print_normal("speedtest end, duriation = %lu(s), packet count = %lu, total data = %lu(KB),speed=%u(KB/s)\n"
					, (tick_end - tick_start)/1000
					, send_count_total
					, ((speed_arg->pkt_size * send_count_total) / 1000) 
					, ((speed_arg->pkt_size * send_count_total) / (tick_end - tick_start)) );
			}
			else
			{
				ak_print_normal("send end, duriation less than 1 second\n");
			}
			
		}
		free(sendBuf);
sock_free_exit:
		//shutdown(socket_s,SHUT_RDWR);
		//closesocket(socket_s);
		ak_print_normal("exit tcp speed memleft:%d\n", (int)Fwl_GetRemainRamSize());
	}
task_exit:
		ak_thread_exit();

}

void cmd_speed_test (int argc, char **args)
{
	unsigned long  ipaddr = IPADDR_NONE;
    unsigned short port = 0;
	unsigned int pkt_size = 0;
	unsigned int duriation = 0;
	unsigned int protocol;
	unsigned int test_times;
	int i;
	speed_test_arg_t speed_arg;
	struct netrecv_arg nr_arg;
	int socket_s = -1;
	ak_pthread_t thread_id1;
	ak_pthread_t thread_id2;
	int ret;
	
	
	if(argc < 2)
	{
		ak_print_error("%s\n",help_speed_test[1]);
		return;
	}
	
	/*pkt_size and duriation*/
	speed_arg.pkt_size  = atoi(args[0]);
	speed_arg.duriation = atoi(args[1]);    

	/*ip addr*/
	if(argc > 2)
	{
		speed_arg.sock.ipaddr = inet_addr((char *)args[2]);
		if (IPADDR_NONE == speed_arg.sock.ipaddr)
		{
		   ak_print_error("set remote_ipaddr wrong.\n");
		   return;
		}
	}
	else
	{
		speed_arg.sock.ipaddr = inet_addr(SPEED_TEST_REMOTE_ADDR);
	}
	/*port*/
	if (argc > 3)
	{
		speed_arg.sock.port = atoi(args[3]);
		if(speed_arg.sock.port > 65535)
		{
			ak_print_error("port should less than 65535\n");
			return;
		}
	}
	else
	{
		speed_arg.sock.port = SPEED_TEST_REMOTE_PORT;
	}

	/*protocol*/
	if (argc > 4)
	{
		if (strcmp(args[4], PARAM_UDP_PROTOCOL) == 0)
	    {
	       speed_arg.protocol = PROTOCOL_UDP;
		   
		}
		else if (strcmp(args[4], PARAM_TCP_PROTOCOL) == 0)
		{
	       speed_arg.protocol = PROTOCOL_TCP;
		}
		else
		{
			ak_print_error("%s\n",help_speed_test[1]);
			return;
		}
	}
	else
	{
		speed_arg.protocol = PROTOCOL_UDP;
	}

	//test times
	if (argc > 5)
	{
		test_times = atoi(args[5]);
	}
	else
	{
		test_times = 1;
	}

	i = 1;
	while(test_times-- )
	{
		ak_print_normal("\n start %d times test\n\n", i++);
		if(speed_arg.protocol == PROTOCOL_UDP)
		{
			socket_s = create_udp_socket();
			if( socket_s < 0)
			{
				ak_print_error("no socket created exit\n");
			}
			else
			{
				ak_print_normal("socket id %d\n", socket_s);
				speed_arg.sock.sock_id = socket_s;
				memcpy(&nr_arg.sock, &speed_arg.sock , sizeof(nr_arg.sock));
				nr_arg.runtime = speed_arg.duriation;
				
				ak_thread_create(&thread_id1, udprecv, &nr_arg, ANYKA_THREAD_MIN_STACK_SIZE, SPEED_TASK_PRIO);
				ak_thread_create(&thread_id2, speed_udp_send, &speed_arg, ANYKA_THREAD_MIN_STACK_SIZE, SPEED_TASK_PRIO);
				//ak_thread_join(thread_id1);
				ak_thread_join(thread_id2);
				ak_thread_cancel(thread_id1);
				shutdown(socket_s, SHUT_RDWR);
				closesocket(socket_s);
			}
		}
		else if(speed_arg.protocol == PROTOCOL_TCP)
		{
			socket_s = create_tcp_socket();
			if( socket_s < 0)
			{
				ak_print_error("no socket created exit\n");
			}
			else
			{
				sock_cfg_t sock_arg;
				sock_arg.ipaddr = speed_arg.sock.ipaddr;
				sock_arg.port = speed_arg.sock.port;
				sock_arg.sock_id = socket_s;
				
				ret = connect_tcp_server(&sock_arg);
				if(ret < 0)
				{
					ak_print_error("connect tcp server error, exit\n");
					return;
				}
				
				speed_arg.sock.sock_id = socket_s;
				ak_print_normal("tcp socket id %d\n", speed_arg.sock.sock_id);
							
			    ak_thread_create(&thread_id1, speed_tcp_recv, &speed_arg, ANYKA_THREAD_MIN_STACK_SIZE, SPEED_TASK_PRIO);
				ak_thread_create(&thread_id2, speed_tcp_send, &speed_arg, ANYKA_THREAD_MIN_STACK_SIZE, SPEED_TASK_PRIO);
				//ak_thread_join(thread_id1);
				ak_thread_join(thread_id2);
				ak_thread_cancel(thread_id1);
				shutdown(socket_s, SHUT_RDWR);
				closesocket(socket_s);
			}
		}
		else
		{
			ak_print_error("unknown protocol, do nothing.\n");
		}
	}
	ak_print_normal("\n %d times test complete \n\n", i - 1);
	
}

void cmd_netrecv_demo(int argc, char **args)
{
	struct netrecv_arg *nr_arg;
	unsigned short port = 0;
    unsigned long  runtime = NETRECV_RUNTIME;
	bool is_udp;
    bool parse_err = true;
	int socket_s = -1;

    //protocol
	if (argc>0 && (char *)args[0] != NULL)
	{
	    if (strcmp(args[0], PARAM_UDP_PROTOCOL) == 0)
	    {
           is_udp = true;
		   parse_err = false;
		   port = UDPRECV_LOCAL_PORT;
		}
		else if (strcmp(args[0], PARAM_TCP_PROTOCOL) == 0)
		{
           is_udp = false;
		   parse_err = false;
		   port = TCPRECV_LOCAL_PORT;
		}
	}
	
	if (parse_err || argc < 2)
	{
		ak_print_error("%s",help_recv[1]);
		return;
	}

    //udprecv timer
	if (argc>1 && (char *)args[1] != NULL)
	{
	    runtime = atoi(args[1]);
	}

	//socket port
	if (argc>2 && (int *)args[2] != NULL)
	{
		port = atoi(args[2]);
		if(port > 65535)
		{
			ak_print_error("port should less than 65535\n");
			return;
		}
	}
	
	nr_arg = (struct netrecv_arg *)malloc(sizeof(struct netrecv_arg));
    if (NULL == nr_arg)
    {
		ak_print_error("malloc netrecv_arg err.\n");
		return;
	}
	nr_arg->sock.ipaddr = INADDR_ANY;
	nr_arg->sock.port = port;
    nr_arg->runtime = runtime;
	
    ak_print_normal("netrecv protocol=%s,local_port=%d,runtime:%d.\n", 
		           (is_udp?PARAM_UDP_PROTOCOL:PARAM_TCP_PROTOCOL), port, runtime);
	if (is_udp)
	{
		socket_s = create_udp_socket();
		nr_arg->sock.sock_id = socket_s;
		udprecv(nr_arg);
	}
	else
	{
		socket_s = create_tcp_socket();
		nr_arg->sock.sock_id = socket_s;
		tcp_server_recv(nr_arg);
	}

	free(nr_arg);
}

void cmd_netsend_demo(int argc, char **args)
{
	struct netsend_arg *ns_arg;
	unsigned long  ipaddr = IPADDR_NONE;
    unsigned long  count = NETSEND_COUNT_DEFAULT;
	unsigned short port = 0;
	char string[NETSEND_STRING_LIMIT] = NETSEND_STRING_DEFAULT;
	bool is_udp;
    bool parse_err = true;
		
    //protocol
	if (argc>0 && (char *)args[0] != NULL)
	{
	    if (strcmp(args[0], PARAM_UDP_PROTOCOL) == 0)
	    {
		    is_udp = true;
			parse_err = false;
		    port = UDPSEND_REMOTE_PORT;
		}
		else if (strcmp(args[0], PARAM_TCP_PROTOCOL) == 0)
		{
			is_udp = false;
			parse_err = false;
		    port = TCPSEND_REMOTE_PORT;
		}
	}
	
	if (parse_err || argc < 3)
    {
		ak_print_error("%s",help_send[1]);
		return;
	}

	 //string
	if (argc>1 && (char *)args[1] != NULL)
	{
		if(strlen(args[1]) > NETSEND_STRING_LIMIT - 1)
		{
			ak_print_error("string length should less than %d\n",NETSEND_STRING_LIMIT);
			return;
		}
	    strcpy(string, (char *)args[1]);
		
	}

	//count
	if (argc>2 && (int *)args[2] != NULL)
	{
		count = atoi(args[2]);
		
	}
	
	//ipaddr
	if (argc>3 && (char *)args[3] != NULL)
	{
	    ipaddr = inet_addr((char *)args[3]);
		if (IPADDR_NONE == ipaddr)
		{
		   ak_print_error("set remote_ipaddr wrong.\n");
		   return;
		}
	}

	//port
	if (argc>4 && (int *)args[4] != NULL)
	{
		port = atoi(args[4]);
		if(port > 65535)
		{
			ak_print_error("port should less than 65535\n");
			return;
		}
	}
	

	ns_arg = (struct netsend_arg *)malloc(sizeof(struct netsend_arg));
    if (NULL == ns_arg)
    {
		ak_print_error("malloc netsend_arg err.\n");
		return;
	}
	
    ak_print_normal("netsend protocol=%s,remote_ipaddr=%s,remote_port=%d,string=%s,count:%d.\n", 
		           (is_udp?PARAM_UDP_PROTOCOL:PARAM_TCP_PROTOCOL), 
		           ((ipaddr==IPADDR_NONE)?NETSEND_REMOTE_ADDR:(char *)args[3]),
		           port, string, count);
	
	if (ipaddr == IPADDR_NONE)
	{
		ipaddr = inet_addr(NETSEND_REMOTE_ADDR);
	}

	memcpy(ns_arg->string, string, NETSEND_STRING_LIMIT);
	ns_arg->count = count;
    ns_arg->sock.ipaddr = ipaddr;
	ns_arg->sock.port = port;
	
	if (is_udp)
	{
		udpsend(ns_arg);
	}
	else
	{
		tcp_client_send(ns_arg);
	}
	
	free(ns_arg);
}

#define DEFAULT_TCPDUMP_MEM_SIZE 4000 // 4000 KB
#define MAX_TCPDUMP_MEM_SIZE 10000 // 4000 KB
#define DEFAULT_TCPDUMP_FILE "tcpdump_001.cap"
#define FILE_PATH            "a:/"
#define MAX_FILE_NAME_SIZE   256



FILE *g_fp = NULL;


int write_file( const void *buffer, size_t size)
{
	if(g_fp == NULL)
	{
		ak_print_error("file pointer is NULL\n");
		return -1;
	}
	return fwrite(buffer, size, 1, g_fp);
}

void cmd_tcpdump(int argc, char **args)
{
	unsigned int mem_size;
	unsigned char *tmp_file_name = NULL;
	unsigned char file_name[MAX_FILE_NAME_SIZE];
	
	if(argc < 1)
	{
		ak_print_error("%s\n",help_tcpdump[1]);
		return;
	}
	if (strcmp(args[0], "start") == 0)
	{
		if(argc > 1)
		{
			mem_size = atoi(args[1]);
			if(mem_size == 0 || mem_size > MAX_TCPDUMP_MEM_SIZE)
			{
				ak_print_error("mem size invalid should less than %d\n", MAX_TCPDUMP_MEM_SIZE);
				return ;
			}
			mem_size = mem_size * 1024;
		}
		else
		{
			mem_size = DEFAULT_TCPDUMP_MEM_SIZE * 1024;
		}
			
		tcpdump_init(mem_size);
		tcpdump_start();
		ak_print_normal(" tcpdump started \n");
		
			
	}
	else if(strcmp(args[0], "stop") == 0)
	{
		if(argc > 1)
		{
			tmp_file_name = args[1];
			if(strlen(tmp_file_name) > MAX_FILE_NAME_SIZE -1)
			{
				ak_print_error("file name %s too long use default name %s\n", tmp_file_name, DEFAULT_TCPDUMP_FILE);
				tmp_file_name = DEFAULT_TCPDUMP_FILE;
			}
		}
		else
		{
			tmp_file_name = DEFAULT_TCPDUMP_FILE;
		}
		sprintf(file_name, "%s%s", FILE_PATH, tmp_file_name);
		
		if (0 !=ak_mount_fs(DEV_MMCBLOCK, 0, ""))
		{
			ak_print_error("mount sd fail!\n");
			return ;
		}
		
		g_fp = fopen(file_name, "w");
		if(g_fp == NULL)
		{
			ak_print_error("open %s error!\n", file_name);
			return -1;
		}
		tcpdump_stop(write_file);
		tcpdump_free();
		fclose(g_fp);
		ak_print_normal(" tcpdump stopped \n");
	}
	else
	{
		ak_print_error("%s\n",help_tcpdump[1]);
		return;
	}
	
}

extern void net_app_iperf3(int argc, char **argv);

static int g_iperf3_argc = 0;
void net_app_iperf3_thread(char **argv)
{
    net_app_iperf3(g_iperf3_argc,argv);   
    ak_print_normal("==========iperf3 thread exit==========\n");
    ak_thread_exit();
}
void cmd_net_iperf3(int argc, char **argv)
{
    int id;

    g_iperf3_argc = argc;
    ak_thread_create(&id, net_app_iperf3_thread, argv, ANYKA_THREAD_MIN_STACK_SIZE, 80);
}

// lwip stats display
void cmd_lwip_stats(int argc, char **argv)
{
    stats_display();
}

static int cmd_net_demo_reg(void)
{
    cmd_register("netrecv", cmd_netrecv_demo, help_recv);
	cmd_register("netsend", cmd_netsend_demo, help_send);
	cmd_register("speedtest", cmd_speed_test, help_speed_test);
	cmd_register("tcpdump", cmd_tcpdump, help_tcpdump);
    cmd_register("iperf3", cmd_net_iperf3, help_iperf3);
    cmd_register("lwipstats", cmd_lwip_stats, help_lwipstats);
    return 0;
}

cmd_module_init(cmd_net_demo_reg)


