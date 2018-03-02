#ifndef _WIFI_H_
#define _WIFI_H_

/** Character, 1 byte */
typedef signed char t_s8;
/** Unsigned character, 1 byte */
typedef unsigned char t_u8;

/** Short integer */
typedef signed short t_s16;
/** Unsigned short integer */
typedef unsigned short t_u16;

/** Integer */
typedef signed int t_s32;
/** Unsigned integer */
typedef unsigned int t_u32;

/** Long long integer */
typedef signed long long t_s64;
/** Unsigned long long integer */
typedef unsigned long long t_u64;

/** Void pointer (4-bytes) */
typedef void t_void;


//set wifi module
typedef enum
{
	WIFI_MODE_STA = 0, 
	WIFI_MODE_AP,
	WIFI_MODE_ADHOC,
	WIFI_MODE_UNKNOWN
	
}AK_WIFI_MODE;

#define MAC_ADDR_LEN  	6
#define MAX_SSID_LEN 	32
#define MIN_KEY_LEN  	8
#define MAX_KEY_LEN  	64
#define MAX_AP_COUNT 	16
#define MAX_STA_COUNT 	16

enum wifi_security 
{
    SECURITY_OPEN               = 0,
    SECURITY_WPA_AES_PSK        = 1, // WPA-PSK AES
    SECURITY_WPA2_AES_PSK       = 2, // WPA2-PSK AES
    SECURITY_WEP_PSK            = 3, // WEP+OPEN
    SECURITY_WEP_SHARED         = 4, // WEP+SHARE
    SECURITY_WPA_TKIP_PSK       = 5, // WPA-PSK TKIP
    SECURITY_WPA2_TKIP_PSK      = 6, // WPA2-PSK TKIP
    SECURITY_WPA2_MIXED_PSK     = 7  // WPA2-PSK AES & TKIP MIXED
} wifi_security_e;

#define SECURITY_STR(val) 				(( val == SECURITY_OPEN ) ? "Open" : \
								         ( val == SECURITY_WEP_PSK ) ? "WEP" : \
								         ( val == SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" : \
								         ( val == SECURITY_WPA_AES_PSK ) ? "WPA AES" : \
								         ( val == SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" : \
								         ( val == SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" : \
								         ( val == SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" : \
                						 "Unknown")



struct _apcfg
{
	unsigned char mac_addr[MAC_ADDR_LEN];
	unsigned char ssid[MAX_SSID_LEN];
	unsigned int  ssid_len;
	unsigned char mode;    // bg, bgn, an,...
	unsigned char channel;
	unsigned char txpower; //0 means max power
	unsigned int  enc_protocol; // encryption protocol  eg.wep, wpa, wpa2 
	unsigned char key[MAX_KEY_LEN];
	unsigned int  key_len;
	
};

typedef struct wifi_ap_info {
    t_u8   ssid[MAX_SSID_LEN];
    t_u8   bssid[MAC_ADDR_LEN];
    t_u32  channel;
    t_u32  security;
    t_u32  rssi;
} wifi_ap_info_t;

typedef struct wifi_ap_list
{
	t_u16  ap_count;
	wifi_ap_info_t ap_info[MAX_AP_COUNT];
}wifi_ap_list_t;

typedef struct wifi_sta_info 
{
    t_u8  mac_address[MAC_ADDR_LEN];
	t_u8  rssi;
	t_u8  rate;
} wifi_sta_info_t;

typedef struct wifi_sta_list
{
	t_u16  sta_count;
	wifi_sta_info_t sta_info[MAX_STA_COUNT];
} wifi_sta_list_t;


#define WAKEUP_PKT_MAX  256
#define KEEP_ALIVE_PKT_MAX 256

typedef struct _net_wakeup_cfg{	
	unsigned int   wakeup_data_offset; /*offset  of  the head of  ethernet II*/
	unsigned int   wakeup_data_len;
	unsigned char  wakeup_data[WAKEUP_PKT_MAX];
}net_wakeup_cfg_t;


typedef struct _power_save_t {
	unsigned int ps_mode; /*0:disable, 1: Auto mode, 2: PS-Poll mode, 3: PS Null mode */
	unsigned int delay_to_ps;
	unsigned int dtim_interval;
	unsigned int ps_null_interval;
} power_save_t;

typedef struct _keep_alive_t {
	unsigned char enable;
	unsigned char mode;             //0: make another socket connection, 1: use existing socket
	int socket;
	unsigned short send_interval;
	unsigned short retry_interval;
	unsigned short retry_count;
	unsigned long  dst_ip;
	unsigned short dst_port;
	unsigned short payload_len;
	unsigned char payload[KEEP_ALIVE_PKT_MAX];
} keep_alive_t;


typedef struct _smc_ap_info_t
{
	unsigned char  ssid[33]; /*max ssid 32 + \0*/
	unsigned char  bssid[6];
	unsigned char  channel;
	unsigned char  security;
	unsigned char  security_key_length;
	unsigned char  security_key[65]; /*max key 64 + \0*/
} smc_ap_info_t;


//user interface to control wifi
char *wifi_get_version();
int wifi_get_mode();  		
int wifi_set_mode(int type);  		
int wifi_connect(char *ssid, char *password);  
int wifi_disconnect(); 
int wifi_isconnected();   	
int wifi_create_ap(struct _apcfg *ap_cfg);
int wifi_destroy_ap();

//kernel interface for lwip interface
int wifi_get_mac(unsigned char *mac_addr);
int wifi_init(void *init_param);

int wifi_sleep();
int wifi_wakeup();

int wifi_keepalive_set(keep_alive_t *keep_alive);
int wifi_check_wakeup_status(void);

typedef enum
{
	KEY_NONE = 0, KEY_WEP, KEY_WPA, KEY_WPA2, KEY_MAX_VALUE
} SECURITY_TYPE;

typedef struct _WIFI_INITPARAM{
	unsigned int size;
	unsigned char *cache;
}T_WIFI_INITPARAM, *T_pWIFI_INITPARAM;

extern struct net_device *wifi_dev;

#define DOT11N_ENABLE 1
#define DOT11N_DISABLE 0
#define WIFI_MAX_CLIENT_DEFAULT 4
#define DEFAULT_MAX_TX_PENDING 100

#define UDP_MTU  (1460) 


#ifdef MAX_TX_PENDING_CTRL
int wifi_get_tx_pending();
int wifi_set_max_tx_pending(u8 max_tx_pending);
int wifi_get_max_tx_pending();
#endif

#endif
