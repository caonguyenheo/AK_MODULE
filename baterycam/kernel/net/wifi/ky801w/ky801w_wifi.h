#ifndef _AK8031_WIFI_H_
#define _AK8031_WIFI_H_



char *ky801w_Wifi_GetVersion(void);

int ky801w_wifi_power_cfg(unsigned char power_save_level);


int ky801w_wifi_get_mode();

int ky801w_wifi_set_mode(int type);


#endif

