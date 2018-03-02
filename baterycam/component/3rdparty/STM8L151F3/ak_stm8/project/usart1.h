#ifndef ___USART1_H___
#define ___USART1_H___


#ifdef UART_USE_STDIO
#if __ICCARM__
#include <yfuns.h>
#endif

#endif
	
#define CMD_PREAMBLE 			0x55

//#define VDDIO  //如果光敏电路的电源使用的pmu输出的VDDIO则需打开该条件
//#define IRCUT_DAY_LOW_LEVEL  //如果白天模式IRCUT需要低电平或者下降沿需打开该条件
#define FOUR_EXP 
#define ADC_ENABLE

#define DAY_MODE 1
#define NIGHT_MODE 0


/*event define*/
#define EVENT_RTC_WAKEUP  		  	0x01
#define EVENT_PIR_WAKEUP 			0x02
#define	EVENT_RING_CALL_WAKEUP 		0x03
#define	EVENT_VIDEO_PREVIEW_WAKEUP	0x04
#define	EVENT_SYS_CONFIG_WAKEUP		0x05
#define	EVENT_SYS_INIT              0x06

typedef struct _cmd_info
{
	unsigned char preamble;
	unsigned char cmd_id;
	unsigned short cmd_len;
	unsigned short cmd_seq;
	unsigned short cmd_result;
	struct
	{
		unsigned char event;
		unsigned char event_1;
		unsigned char event_2;
		unsigned char event_3;

	} param;
	
}CMD_INFO;


#define IRCUT_CHARGE_NIGHT_VALUE 	150 //0.11V    3.03V
#define IRCUT_CHARGE_DAY_VALUE 	200 //0.11V    3.03V




void UART1_Init(unsigned int baudrate);

void UART1_Release(void);

void ak_stm_uart(u16 data);
#ifdef ADC_ENABLE
void updata_exp_adc(unsigned short exp_data,unsigned short adc_data);
#else
void updata_exp_time(unsigned short exp_data);

#endif
unsigned char get_day_night_mode();
void set_day_night_mode(unsigned char mode);


//__interrupt void UART2_RX_RXNE(void);

#endif

