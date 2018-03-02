
#ifndef __AK_STM8_COMMON_IT_H
#define __AK_STM8_COMMON_IT_H


#define WIFI_CONFIG_CLEAR_EVEN   0Xffff
#define RINGCALL_DINGDONG_EVEN   0Xff00

#define STM8_AK_VER_MAJOR  1
#define STM8_AK_VER_MINOR  7

void TIM2_DelayMs(unsigned int ms);

void get_wakeup_type(unsigned char *data);

void set_wakeup_type(void);

void get_key_flg(unsigned char *data);

void set_key_flg(void);

void get_time_count(unsigned char *data);

void get_uart_flg(unsigned char *data);

void set_uart_flg(void);


void pir_wifiwakeup_int_enable(void);

void pir_wifiwakeup_int_disable(void);

void power_ak(char on_off);


#ifdef ADC_ENABLE
void  adc_ircut(unsigned short * exp_data,unsigned short * adc_data);
#else
void  io_sample_ircut(unsigned char *level);
#endif

#endif // #ifndef __AK_STM8_COMMON_IT_H

