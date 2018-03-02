/*******************************************************************
 * @brief  a module to config wifi 
 
 * @Copyright (C), 2016-2017, Anyka Tech. Co., Ltd.
 
 * @Modification  2017-9 create 
*******************************************************************/
#include "wifi_backup_common.h" 
#include "sky_wifi_cfg.h"

#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "dev_info.h"
#include "kernel.h"

#include "drv_gpio.h"


static char *help_wifi_cfg[]={
	"wifi config tools\n",
	"usage: wifista config [IP] [port] [ssid] <password> \n"

};




#define AK_CMD_POWER_ON               "akon"
#define AK_CMD_POWER_OFF              "akoff"
#define AK_CMD_START_VIDEO            "start"
#define AK_CMD_STOP_VIDEO             "stop"
#define AK_CMD_WIFI_TX_TEST			  "wifitx"
#define AK_CMD_CONFIG                 "config"
#define AK_CMD_CONFIG_SHOW            "showcfg"
#define AK_CMD_SET_SSID				  "ssid="
#define AK_CMD_SET_KEY				  "key="
#define AK_CMD_SET_SERVER			  "server="
#define AK_CMD_SET_PORT			      "port="
#define AK_CMD_CLEAR_CFG    	      "clearcfg"


#define WIFI_CFG_AP_SSID        "anyka_sky"
#define WIFI_CFG_AP_KEY         "123456789"
#define WIFI_CFG_AP_PORT        5005


#define WIFI_CFG_CMD_BUF_SIZE       1024

#define SKY_IPV4_BYTE(val,index)                  ( (val >> (index*8)) & 0xFF )


/**
 * get configuration for camview (including ssid password ip port and port).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */

int sky_wifi_set_config(struct net_sys_cfg *t_server_info)
{
	void *handle = NULL;
	int ret = -1;
	char *server_parti = SERVER_IP_PARTI;

	ak_print_normal("open %s \n", server_parti);
	
	handle = ak_partition_open(server_parti);
	if(handle == NULL)
	{
		ak_print_normal("open %s error\n", server_parti);
	}
	else
	{
		ret = ak_partition_write(handle, t_server_info, sizeof(struct net_sys_cfg));
		if(ret < 0)
		{
			ak_print_normal("write %s error\n", server_parti);
		}
		else
		{
			ak_print_normal("back IP info on %s OK\n", server_parti);
		}
		ak_partition_close(handle);
	}
	return ret;	
}


/**
 * get configuration for camview (including ssid password ip port and port).
 *
 * @param ip_info the network interface if infomation including ip gw netmask
 */

int sky_wifi_get_config(struct net_sys_cfg *t_server_info)
{
	void *handle = NULL;
	int ret = -1;
	char *server_parti = SERVER_IP_PARTI;
	
	ak_print_normal("open %s \n", server_parti);
	
	handle = ak_partition_open(server_parti);
	if(handle == NULL)
	{
		ak_print_normal("open %s error\n", server_parti);
	}
	else
	{
		ret = ak_partition_read(handle, t_server_info, sizeof(struct net_sys_cfg));
		if(ret < 0)
		{
			ak_print_normal("write %s error\n", server_parti);
		}
		else
		{
			ak_print_normal("back IP info on %s OK\n", server_parti);
		}
		ak_partition_close(handle);
	}
	return ret;	
}

int  sky_wifi_clear_config(void)
{
    int ret = -1;
    // clear wifi config
    struct net_sys_cfg t_server_info;
    memset(&t_server_info, 0, sizeof(struct net_sys_cfg));
    ret = sky_wifi_set_config(&t_server_info);
    // clear backup ip
    struct ip_info t_ip_info;
    memset(&t_ip_info, 0, sizeof(struct ip_info));
    ret |= wifi_back_ip_info(&t_ip_info);
    return  ret;
}

static void sky_wifi_set_with_shell(int argc, char **args)
{
	unsigned long  ipaddr = IPADDR_NONE;
    unsigned short port = 0;
    
    if(strcmp(args[0], "config") == 0) {
        struct net_sys_cfg t_server_info;
        memset(&t_server_info, 0, sizeof(struct net_sys_cfg));

        if(argc == 1)
        {
            sky_wifi_get_config(&t_server_info);
            ak_print_normal("ssid: %s\n", t_server_info.ssid);
            ak_print_normal("password: %s\n", t_server_info.key);
            ak_print_normal("IP: %u.%u.%u.%u\n", 
                t_server_info.server_ip & 0xff, 
                (t_server_info.server_ip >> 8) & 0xff,
                (t_server_info.server_ip >> 16) & 0xff, 
                (t_server_info.server_ip >> 24) & 0xff);
            ak_print_normal("port: %d\n", t_server_info.port_num);
            return;
        }
        if(argc < 4)
        {
            ak_print_normal("%s", help_wifi_cfg[1]);
            return;
        }
		
        //ipaddr       
        ipaddr = inet_addr((char *)args[1]);
        if (IPADDR_NONE == ipaddr)
        {
           ak_print_error("set remote_ipaddr wrong.\n");
           return;
        }
        t_server_info.server_ip = ipaddr;

		//port
        port = atoi(args[2]);
        if(port > 65535)
        {
            ak_print_error("port should less than 65535\n");
            return;
        }
        t_server_info.port_num = port;

		 //SSID
        strcpy(t_server_info.ssid, args[3]);
        //password
        if(argc > 4)
        {
        	strcpy(t_server_info.key, args[4]);
        }
		
        sky_wifi_set_config(&t_server_info);
    }
    
}

static int sky_net_cmd_parse(char *cmd_buf,void *args)
{
    int ret; 
	char *optPtr;
    char char_buf[64];
    int cfd = *(int*)args;
    struct net_sys_cfg t_server_info;
    memset(&t_server_info, 0, sizeof(struct net_sys_cfg));
    
    if(optPtr = strstr(cmd_buf, AK_CMD_SET_SSID))
    {
        if(strlen(optPtr) > strlen(AK_CMD_SET_SSID) && strlen(optPtr) <= strlen(AK_CMD_SET_SSID) + MAX_SSID_LEN)
        {
            sky_wifi_get_config(&t_server_info);
            strcpy(t_server_info.ssid, strstr(cmd_buf, AK_CMD_SET_SSID) + strlen(AK_CMD_SET_SSID));
            ret = sky_wifi_set_config(&t_server_info);
            if(ret > 0)
            {
                char *result = "Set ssid OK\n";
                /*send back set result*/
                ret = send(cfd, result, strlen(result), 0);
            }
        }
    }
     else if(optPtr = strstr(cmd_buf, AK_CMD_SET_KEY))
    {
        if(strlen(optPtr) > strlen(AK_CMD_SET_KEY) && strlen(optPtr) <= strlen(AK_CMD_SET_KEY) + MAX_KEY_LEN)
        {
            sky_wifi_get_config(&t_server_info);
            strcpy(t_server_info.key, strstr(cmd_buf, AK_CMD_SET_KEY) + strlen(AK_CMD_SET_KEY));
            ret = sky_wifi_set_config(&t_server_info);
            if(ret > 0)
            {
                char *result = "Set key OK\n";
                /*send back set result*/
                ret = send(cfd, result, strlen(result), 0);
                
            }
        }
    }
    else if(optPtr = strstr(cmd_buf, AK_CMD_SET_SERVER))
    {
        strcpy(char_buf, strstr(cmd_buf, AK_CMD_SET_SERVER) + strlen(AK_CMD_SET_SERVER));
        unsigned int server_ip = inet_addr(char_buf);
        if(0 != server_ip)
        {
            sky_wifi_get_config(&t_server_info);
            t_server_info.server_ip = server_ip;
            ret = sky_wifi_set_config(&t_server_info);
            if(ret > 0)
            {
                char *result = "Set server OK\n";
                ret = send(cfd, result, strlen(result), 0);
                
            }
        }
    }
    else if(optPtr = strstr(cmd_buf, AK_CMD_SET_PORT))
    {
        strcpy(char_buf, strstr(cmd_buf, AK_CMD_SET_PORT) + strlen(AK_CMD_SET_PORT));
        int port = atoi(char_buf);
        if(port > 0 && port < 65536)
        {
            sky_wifi_get_config(&t_server_info);
            t_server_info.port_num = (unsigned short )port;
            ret = sky_wifi_set_config(&t_server_info);
            if(ret > 0)
            {
                ak_print_normal("port= %d",t_server_info.port_num);
                char *result = "Set port OK\n";
                ret = send(cfd, result, strlen(result), 0);
            }
        }
    }
    else if(optPtr = strstr(cmd_buf, AK_CMD_CONFIG_SHOW))
    {
        char  *result= malloc(WIFI_CFG_CMD_BUF_SIZE);
        memset((char*)&t_server_info, 0, sizeof(struct net_sys_cfg));
        
        sky_wifi_get_config(&t_server_info);
        sprintf(result, "SSID: %s \n", t_server_info.ssid);
        sprintf(result + strlen(result), "KEY:  %s \n", t_server_info.key);
        sprintf(result + strlen(result), "Server IP: %d.%d.%d.%d \n", SKY_IPV4_BYTE(t_server_info.server_ip,0),
                        SKY_IPV4_BYTE(t_server_info.server_ip,1),
                        SKY_IPV4_BYTE(t_server_info.server_ip,2),
                        SKY_IPV4_BYTE(t_server_info.server_ip,3));
        sprintf(result + strlen(result), "PORT: %d\n", t_server_info.port_num);
        
        ret = send(cfd, result, strlen(result), 0);
        free(result);
    } else if(strstr(cmd_buf, AK_CMD_CLEAR_CFG))
    {
        ret = sky_wifi_clear_config();
        if(ret >= 0) {
            char *result = "clear wifi config ok\n";
            ret = send(cfd, result, strlen(result), 0);
        }
    } else {
        char *result = "no desired cmd\n";
        ret = send(cfd, result, strlen(result), 0);
    }

}
static int sky_wifi_set_with_net_task(void *args)
{
    int cfd = -1;
    int sfd;
	char cmd_buf[WIFI_CFG_CMD_BUF_SIZE] = {0};
        
   
    /** create socket **/
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
		//exit_err("sfdet");
		ak_print_error_ex("sfdet\n");
	}
    
	/** initialize and fill in the message to the sfdet addr infor struct **/
	struct sockaddr_in server_addr;	/** server socket addr information struct **/
	memset(&server_addr, 0, sizeof(struct sockaddr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(WIFI_CFG_AP_PORT);
	server_addr.sin_addr.s_addr =INADDR_ANY;

	/** set socket attribute **/
	int sinsize = SOF_REUSEADDR;//SOF_REUSEADDR;//1;
	int ret = setsockopt(sfd, SOL_SOCKET, SOF_REUSEADDR, &sinsize, sizeof(int));
    if(ret != 0){
        printf("set opt error\n");
    }

	/** bind sfdet with this program **/
	ret = bind(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
	if (ret != 0) {
		close(sfd);
		//exit_err("bind");
		ak_print_error_ex("bind\n");
	}


	/** listen, create wait_queue **/
	ret = listen(sfd, 4);	/* only one client can connect to this server */
	if (ret < 0) {
		close(sfd);
		//exit_err("listen");
		ak_print_error_ex("listen error\n");
		return -1;
	}

	struct sockaddr_in peer_addr;		/** server socket addr information struct **/
	memset(&peer_addr, 0, sizeof(struct sockaddr));
	//accept
	while (1) {
        ak_print_notice("wifi start as ap\n");
		ak_print_notice("waiting for client connect ...\n");

		sinsize = sizeof(struct sockaddr);
		/** accept the client to connect **/
		cfd = accept(sfd, (struct sockaddr *)&peer_addr, (socklen_t *)&sinsize);
		if (cfd < 0) {
			close(cfd);
			//exit_err("accept");
			ak_print_error_ex("accept error\n");
			return -1;
		}
		printf("Client connected, socket cfd = %d\n", cfd);

        while(1) {
            memset(cmd_buf, 0, WIFI_CFG_CMD_BUF_SIZE);
            ret = recv(cfd, cmd_buf, WIFI_CFG_CMD_BUF_SIZE, 0);
            ak_print_normal("recv %s\n",cmd_buf);
    	    if( ret <= 0 ) {
                ak_print_error_ex("wifi config recv error\n");
                break;
    	    }
            sky_net_cmd_parse(cmd_buf,&cfd);

        }
        close(cfd);
    }

}

static int sky_wifi_init()
{
    int fd; 
    int tmp;
    int init = 0;

    fd = dev_open(DEV_WIFI);
    if(fd < 0)
    {
        printk("open wifi faile\r\n");
        while(1);
    }
    dev_read(fd,  &tmp, 1);
    gpio_share_pin_set( ePIN_AS_SDIO , tmp);

    if(wifi_init(&init) == 0)  {
        printk("init wifi ok!!!\n");
        unsigned char mac[6] = {0,0,0,0,0,0};
        wifi_get_mac(mac);
        printk("\nmac addr=%x:%x:%x:%x:%x:%x\n\n"
            , mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        return 1;
    } 
    return 0;
}
void sky_wifi_set_with_net(void)
{
    int ret;
    ak_pthread_t id;


    if(sky_wifi_init()) {
        struct _apcfg  ap_cfg;
        memset(&ap_cfg, 0, sizeof(struct _apcfg));
        // set ap mode
        ap_cfg.mode = 0; //0:bg mode  1:bgn mode
        ap_cfg.channel = 1;
        strcpy(ap_cfg.ssid, WIFI_CFG_AP_SSID);
        if(strlen(ap_cfg.ssid) > MAX_SSID_LEN)
        {
            ak_print_normal("AP SSID should be less than 32 characters \n");
            return 0;
        }
        if(strlen(WIFI_CFG_AP_KEY) < MIN_KEY_LEN || strlen(WIFI_CFG_AP_KEY) > MAX_KEY_LEN)
        {
            ak_print_normal("AP key should be %d ~ %d characters \n", MIN_KEY_LEN, MAX_KEY_LEN);
            return 0;
        }
        strcpy(ap_cfg.key, WIFI_CFG_AP_KEY);
        ap_cfg.enc_protocol = KEY_WPA2;
        
        ak_print_normal("Create AP SSID:%s, 11n: %s, key mode:%d, key:%s, channel:%d\n", 
            ap_cfg.ssid, ap_cfg.mode?"enable":"disable", 
            ap_cfg.enc_protocol, ap_cfg.enc_protocol?(char*)ap_cfg.key:"",
            ap_cfg.channel);
        
        wifi_set_mode(WIFI_MODE_AP);
        wifi_create_ap(&ap_cfg);
        wifi_netif_init();
        
        // creat tcp server
         ret = ak_thread_create(&id, sky_wifi_set_with_net_task, NULL, 8*1024, 50);
        if(ret < 0)
        {
            ak_print_error("create sky_wifi_set_with_net_task fail!\n");
            return ;
        }       
    }
    return ;
}

int sky_wifi_demo()
{
    cmd_register("wifista", sky_wifi_set_with_shell, help_wifi_cfg);
    //cmd_register("wifiinit", sky_wifi_init, help_wifi_cfg);
    return 0;
}

cmd_module_init(sky_wifi_demo)




