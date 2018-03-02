#ifndef _SV6030_WIFI_H_
#define _SV6030_WIFI_H_



char *sv6030_Wifi_GetVersion(void);

int sv6030_wifi_power_cfg(unsigned char power_save_level);


int sv6030_wifi_get_mode();

int sv6030_wifi_set_mode(int type);


#endif

