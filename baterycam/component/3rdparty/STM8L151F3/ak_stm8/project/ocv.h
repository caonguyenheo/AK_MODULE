#ifndef __OCV_H
#define __OCV_H	


void axp_ocv_restcap(unsigned char *value);
//unsigned char axp_ocv_restcap(void);


void I2C_dev_open();

void I2C_dev_close();
void axp_power_off();

void axp_set_DCDC_voltage();

void axp_set_startime();





#endif


