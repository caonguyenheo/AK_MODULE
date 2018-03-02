#include"stm8l15x.h"
#include "stm8l15x_conf.h"
#include "usart1.h"
#include "ocv.h"

#include "ak_stm8_common_it.h"



#define SW16(X) ((((X)&0xff)<<8)|((X>>8)&0xff))
//#define SW32(X) ((((X)&0xff)<<24)|(((X>>8)&0xff)<<16)|(((X>>16)&0xff)<<8)|((X>>24)&0xff))

static  u16 ak_stm_exp_val;
#ifdef ADC_ENABLE
static  u16 ak_stm_adc_val;
#endif
static unsigned char day_night_mode;

/*******************************************************************************
**函数名称：void UART2_Init(u16 baudrate)
**功能描述：初始化USART模块
**入口参数：u16 baudrate  -> 设置串口波特率
**输出：无
*******************************************************************************/
void UART1_Init(unsigned int baudrate)
{
  CLK_PeripheralClockConfig(CLK_Peripheral_USART1 , ENABLE);    //使能串口1的时钟
  
  USART_Init(USART1 , 
             baudrate ,
             USART_WordLength_8b ,
             USART_StopBits_1 , 
             USART_Parity_No , 
             (USART_Mode_Rx | USART_Mode_Tx)
             );
  
  USART_ITConfig(USART1 , USART_IT_RXNE , ENABLE);    //使能串口1接收到数据产生中断
  
  USART_Cmd(USART1 , ENABLE);     //使能串口1
  
}

void UART1_Release(void)
{
  CLK_PeripheralClockConfig(CLK_Peripheral_USART1 , DISABLE);  
  USART_ITConfig(USART1 , USART_IT_RXNE , DISABLE);    //使能串口1接收到数据产生中断
  USART_Cmd(USART1 , DISABLE);     

}


/*******************************************************************************
**函数名称：void Uart2_SendData(unsigned char  data)
**功能描述：向串口发送寄存器写入一个字节数据
**入口参数：unsigned char  data
**输出：无
*******************************************************************************/
 void Uart1_SendData(unsigned char  data)
{
    while(USART_GetFlagStatus(USART1 , USART_FLAG_TXE) == RESET);        //判断发送数据寄存器是否为空   
    USART_SendData8(USART1 , (u8)data);                     //向发送寄存器写入数据 
}


#define CMD_HEAD_LEN (4)


#define	CMD_WAKEUP_SRC_REQ   		0x01
#define	CMD_WAKEUP_SRC_RESP 		0x02

#define	CMD_RING_DINGDONG 		    0x03


/*reset on CC3200 tell H240 to give out a audio prompt*/
#define CMD_SYS_CONFIG				0x04
#define CMD_SLEEP_REQ				0x05

#define CMD_BATTERY_VAL 		    0x06
#define CMD_BATTERY_GET_VAL 		0x07

//#define CMD_RING_CALL_END			0x08
#define CMD_VIDEO_PREVIEW			0x0A
#define CMD_VIDEO_PREVIEW_END		0x0C

#define CMD_VIDEO_PARAM     		0x0E
#define CMD_SYS_RESTART_REQ         0x08    //ak重新上电请求
#define CMD_SLEEP_REQ_PRE			0x09  //睡眠前提示



/*battery define*/
//#define BATTERY_ALARM_THRESHOLD 	5


/*sys alarm event define*/
#define ALARM_BATTERY_TOO_LOW 		0x01
#define ALARM_MOVE_DETECT			0x02


unsigned char ak_stm_cmd_buf[sizeof(CMD_INFO)];

static void send_cmd(CMD_INFO *cmd)
{
	char i;
	CMD_INFO *pcmd = cmd;
	unsigned char* tmp  = (unsigned char*)cmd; 
		
	pcmd->preamble = CMD_PREAMBLE;

	i = sizeof(CMD_INFO)- CMD_HEAD_LEN;
	pcmd->cmd_len = SW16(i);

	for(i = 0; i < sizeof(CMD_INFO); i++)
	{
		Uart1_SendData(tmp[i]);
	}	

}

static volatile unsigned char ak_wifi_backup_flg = 0; 

//extern u16 isp_exp_value[];

static unsigned char event_flag = 0;


static void which_wakeup(u16 data)
{
	u16 tmp = data;
	unsigned char wakeup_src ;
	CMD_INFO cmd ;

	cmd.cmd_id      = CMD_WAKEUP_SRC_RESP;
	
	if(WIFI_CONFIG_CLEAR_EVEN == tmp)
	{
		cmd.cmd_result = WIFI_CONFIG_CLEAR_EVEN;
	}
	else
	{
		if(NIGHT_MODE == get_day_night_mode())
		{
			cmd.param.event_2 = 1;	
		}
		else
		{
			cmd.param.event_2 = 0;	
		}
		
		
		cmd.cmd_result  = SW16(ak_stm_exp_val);
#ifdef ADC_ENABLE
		cmd.cmd_seq     = SW16(ak_stm_adc_val);
#else
		cmd.cmd_seq     = day_night_mode;
#endif


	
	if(1 == ak_wifi_backup_flg)
		cmd.param.event_1 = 1;
	else
		cmd.param.event_1 = 0;
		
		if(event_flag != 0)
		{
			cmd.param.event_3 = event_flag;
			event_flag = 0;
		}
		else
		{
			cmd.param.event_3 = 0;
		}
		
		get_wakeup_type(&wakeup_src);
		
		switch (wakeup_src)
		{
			case EVENT_PIR_WAKEUP:
				cmd.param.event = (EVENT_PIR_WAKEUP);
				
				break;
			case EVENT_RING_CALL_WAKEUP:
				cmd.param.event = (EVENT_RING_CALL_WAKEUP) ;	
				break;

			case EVENT_VIDEO_PREVIEW_WAKEUP :
				cmd.param.event = (EVENT_VIDEO_PREVIEW_WAKEUP) ;
				break;
			case 0 :
				cmd.param.event = (EVENT_SYS_INIT) ;
				break;
				
			default:
				return ;
				break;
		}
	
	}
	
	send_cmd(&cmd);
	ak_wifi_backup_flg = 0;
	set_wakeup_type();
}

//
//
//
static void send_battery_val_to_ak(u16 data)
{
	u16 tmp = data;
	CMD_INFO cmd ;
	
	cmd.cmd_id      = CMD_BATTERY_VAL;	
	cmd.cmd_result  = SW16(tmp);
	
	if(NIGHT_MODE == get_day_night_mode())
	{
		cmd.param.event_2 = 1;	
	}
	else
	{
		cmd.param.event_2 = 0;	
	}
	send_cmd(&cmd);

}



//
//
//
static void send_ring_dingdong_even_to_ak(u16 data)
{
	CMD_INFO cmd ;
	
	cmd.cmd_id = CMD_RING_DINGDONG;	

	cmd.param.event    = STM8_AK_VER_MAJOR; 
	cmd.param.event_1  = STM8_AK_VER_MINOR; 

	send_cmd(&cmd);

}

unsigned char get_day_night_mode()
{
	return day_night_mode;
}

void set_day_night_mode(unsigned char mode)
{
	day_night_mode = mode;
}

#ifdef ADC_ENABLE
void updata_exp_adc(unsigned short exp_data,unsigned short adc_data)
{
	ak_stm_exp_val = exp_data;
	ak_stm_adc_val = adc_data;
}
#else

void updata_exp_time(unsigned short exp_data)
{
	ak_stm_exp_val = exp_data;
}

#endif

static volatile u8 ak_get_power_first = 0;

void ak_stm_uart(u16 data)
{
	CMD_INFO *cmd;
#ifdef ADC_ENABLE
	u16 exp_data = 0, adc_data = 0;
#endif
	u8  power_val = 0;
    //unsigned char wakeup_src ;
	unsigned char statu = 0;
	u16 tmp = data;
	
	if(RINGCALL_DINGDONG_EVEN == tmp)
	{
		send_ring_dingdong_even_to_ak(tmp);
	}
	else
	{
		
		cmd = (CMD_INFO *)ak_stm_cmd_buf;
		
		switch(cmd->cmd_id)
		{
			case CMD_WAKEUP_SRC_REQ:
				which_wakeup(tmp);
				break;	
				
			case CMD_SLEEP_REQ:
                ak_wifi_backup_flg = 1;
                ak_get_power_first = 0;
                //get_wakeup_type(&wakeup_src);//
                statu = (GPIO_ReadInputData(GPIOB)&GPIO_Pin_1)>>1;
				if(0 == statu)
                {
                    power_ak(0);			
                    TIM2_DelayMs(4);              
                    power_ak(1);                             
                }
                else
                {
					//
				  	power_ak(0);    
					
					pir_wifiwakeup_int_enable();
                }  
                                
				break;
				
			case CMD_BATTERY_GET_VAL:
				I2C_dev_open();	
				axp_ocv_restcap(&power_val);//获取电量
				I2C_dev_close();
#ifdef ADC_ENABLE
				adc_ircut(&exp_data, &adc_data);
				updata_exp_adc(exp_data, adc_data);
#else
				io_sample_ircut((unsigned char *)&tmp);
				
#endif
				send_battery_val_to_ak(power_val);
				break;

			case CMD_SYS_RESTART_REQ://异常指令，ak重新上电
				event_flag = cmd->param.event_3;
				power_ak(0);			
				TIM2_DelayMs(4); 
				ak_wifi_backup_flg = 0;
				power_ak(1);
				break;
          	case CMD_SLEEP_REQ_PRE: 
				//set_wakeup_type();
               	///pir_wifiwakeup_int_enable();//WIFI 睡眠后，AK断电前，wifi又被唤醒的处理
               	
               	break;
                           
			
			default:
				break;
			
		}

	}

}


