
#include"stm8l15x.h"

#include "ak_stm8_common_it.h"

#include "usart1.h"

#include "adc.h"
#include "ocv.h"

#define UART_BAUDRATE 9600


static volatile char ak_power_flg = 0;
static volatile u16  reset_wifi_flg = 0;

static unsigned char ircut_status = 2;   //day:0  night: 1 uninit: 2

u16 adc_sample_value[] ={
 	1352,//1490,  // 1V   3.03V
	2703,//1738, //  2V  3.03V
	2974,//1986, //  2.2V  3.03V
};

u16 isp_exp_value[] ={
#if 0    //25	
	996,
	747,
	498,
	249,
#else   //12.5 
	2012,
	1509,
	1006,
	503,

#endif
};



//#define IR_IS_DAY  isp_exp_value[0]     //具体的数值，需要根据实际确定


/*******************************************************************************
**函数名称：void TIM2_Init()     Name: void TIM2_Init()
**功能描述：初始化定时器2
**入口参数：
**输出：无
*******************************************************************************/
void TIM2_Init(u8 enable)
{
	u8  mask = enable;
	
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2 , ENABLE);              //使能定时器2时钟
	TIM2_TimeBaseInit(TIM2_Prescaler_16 , TIM2_CounterMode_Up , 50000);    //设置定时器2为16分频，向上计数，计数值为50000即为50毫秒的计数值

	if(mask)
	{
		TIM2_ITConfig(TIM2_IT_Update , ENABLE);     //使能向上计数溢出中断
		TIM2_ARRPreloadConfig(ENABLE);  //使能定时器3自动重载功能    

	}
	else
	{
		TIM2_ITConfig(TIM2_IT_Update , DISABLE);     //使能向上计数溢出中断
	}
	
}

/*******************************************************************************
**函数名称：void TIM2_DelayMs(unsigned int ms)
**功能描述：定时器2参进行精确延时，
**入口参数：unsigned int ms     ms :单位50ms
**输出：无
*******************************************************************************/
void TIM2_DelayMs(unsigned int ms)
{ 
	TIM2_Init(0);	
	TIM2_ARRPreloadConfig(ENABLE);  //使能定时器2自动重载功能    
	TIM2_Cmd(ENABLE);               //启动定时器2开始计数
	while(ms--)
	{
		while( TIM2_GetFlagStatus(TIM2_FLAG_Update) == RESET); //等待计数是否达到1毫秒
		TIM2_ClearFlag(TIM2_FLAG_Update);  //计数完成x毫秒，清除相应的标志
	}
	TIM2_Cmd(DISABLE);                   //延时全部结束，关闭定时器2
	TIM2_ARRPreloadConfig(DISABLE);  //使能定时器2自动重载功能  
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2 , DISABLE);              //使能定时器2时钟

}



void pir_wifiwakeup_int_enable(void)
{
	GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_IT); //初始化WIFI，设置PD_0为上拉电阻输入并且使能中断模式
	GPIO_Init(GPIOB, GPIO_Pin_0 , GPIO_Mode_In_FL_IT); //初始化PIR，设置PD_0为上拉电阻输入并且使能中断模式   GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
}

void pir_wifiwakeup_int_disable(void)
{
	GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_No_IT); //初始化WIFI，设置PD_0为上拉电阻输入并且使能中断模式
	GPIO_Init(GPIOB, GPIO_Pin_0 , GPIO_Mode_In_FL_No_IT); //初始化PIR，设置PD_0为上拉电阻输入并且使能中断模式   GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
}
void ircut_option(char on_off)
{
	char tmp  = on_off;
	if(tmp)// on day
	{		
#ifdef IRCUT_DAY_LOW_LEVEL
		GPIO_ResetBits(GPIOB , GPIO_Pin_3);//	
#else
		GPIO_SetBits(GPIOB, GPIO_Pin_3);//close ir 
#endif
		GPIO_ResetBits(GPIOA, GPIO_Pin_3);//close ir led		
		ircut_status = 0;
	}
	else
	{
#ifdef IRCUT_DAY_LOW_LEVEL
		GPIO_SetBits(GPIOB, GPIO_Pin_3);//close ir 
#else
		GPIO_ResetBits(GPIOB , GPIO_Pin_3);//	
#endif	
		GPIO_SetBits(GPIOA , GPIO_Pin_3);// open ir led
		ircut_status = 1;
	}
}


void port_Init(void)
{	
       // GPIO_Init(GPIOB , GPIO_Pin_6 , GPIO_Mode_Out_PP_Low_Fast); //HS PB6
        
	GPIO_Init(GPIOA , GPIO_Pin_2 , GPIO_Mode_Out_PP_Low_Fast); //power led
	
	GPIO_Init(GPIOA , GPIO_Pin_3 , GPIO_Mode_Out_PP_Low_Fast); //IR led
		
	GPIO_Init(GPIOB , GPIO_Pin_2 , GPIO_Mode_Out_PP_Low_Fast); //wifi led
	
	GPIO_Init(GPIOB , GPIO_Pin_3 , GPIO_Mode_Out_PP_Low_Fast);   //IR CUT
	
	GPIO_Init(GPIOB , GPIO_Pin_5 , GPIO_Mode_Out_PP_Low_Fast);  //power key (PMU)
	
	//WIFI WAKEUP
	GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_IT);           //初始化WIFI，设置PD_0为上拉电阻输入并且使能中断模式  GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
	EXTI_SetPinSensitivity(EXTI_Pin_1 ,  EXTI_Trigger_Falling_Low);//EXTI_Trigger_Falling);  //PortD端口为下降沿触发中断  EXTI_Trigger_Falling    EXTI_Trigger_Rising

	// PIR WAKEUP
	GPIO_Init(GPIOB, GPIO_Pin_0 , GPIO_Mode_In_FL_IT); //初始化PIR，设置PD_0为上拉电阻输入并且使能中断模式   GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
	EXTI_SetPinSensitivity(EXTI_Pin_0,  EXTI_Trigger_Rising);  //PortD端口为下降沿触发中断  EXTI_Trigger_Rising  EXTI_Trigger_Falling

	//KEY WAKEUP
	GPIO_Init(GPIOB, GPIO_Pin_4 , GPIO_Mode_In_PU_IT);           //初始化CALL，设置PD_0为上拉电阻输入并且使能中断模式
	EXTI_SetPinSensitivity(EXTI_Pin_4 ,  EXTI_Trigger_Falling);  //PortD端口为下降沿触发中断  EXTI_Trigger_Rising  EXTI_Trigger_Falling
	
}


// 1:on ;0:off
void power_ak(char on_off)
{
	char tmp  = on_off;
	if(tmp)// on
	{
             //   GPIO_ResetBits(GPIOB , GPIO_Pin_6);//
                
		GPIO_SetBits(GPIOB , GPIO_Pin_5);//
		UART1_Init(UART_BAUDRATE);
		ak_power_flg = 1;
	}
	else
	{
             //   GPIO_SetBits(GPIOB , GPIO_Pin_6);//
		
                UART1_Release();
		
		GPIO_ResetBits(GPIOB, GPIO_Pin_3); //close ir
		GPIO_ResetBits(GPIOA, GPIO_Pin_3);//close ir led	
		ircut_status = 2;

		GPIO_Init(GPIOA, GPIO_Pin_0 , GPIO_Mode_In_FL_No_IT);
		GPIO_Init(GPIOC, GPIO_Pin_5 , GPIO_Mode_In_FL_No_IT);
		GPIO_Init(GPIOC, GPIO_Pin_6 , GPIO_Mode_In_FL_No_IT);
		I2C_dev_open();	
		axp_set_DCDC_voltage();
		axp_power_off();
		I2C_dev_close();
		
		GPIO_ResetBits(GPIOB , GPIO_Pin_5);//off
		
		
		ak_power_flg = 0;
	}

}

static void ak_stm_led(char on_off)
{
	char tmp  = on_off;
	if(tmp)// on
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_2);// POWER LED
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);// POWER LED
	}

}

static u8 wifi_info_reset(void)
{
	u8 tmp = 0;	
	if(0 ==(GPIO_ReadInputData(GPIOB)&GPIO_Pin_4))
	{
		while(0 ==(GPIO_ReadInputData(GPIOB)&GPIO_Pin_4))
		{
			TIM2_DelayMs(20);//50ms
			tmp++;
			if(tmp > 3) // 3s reset wifi flg
			{
				GPIO_SetBits(GPIOB, GPIO_Pin_2);// 
				return 1;
			}			
		}		
	}
  	return 0;
}

#ifdef ADC_ENABLE

void  adc_ircut(unsigned short * exp_data,unsigned short * adc_data)
{
	u16 tmp_value = 0;
	u16 adc_value = 0;
	u16 * tmp_exp = exp_data;
	u16 * tmp_adc = adc_data;
	char i;

	ADC_Block_Init();
	//ADC
#ifdef VDDIO	
	TIM2_DelayMs(1);
#endif
	for(i = 0; i <5; i++)
	{
		Read_ADC_Data(&tmp_value);
		if(i>1)//丢弃前面刚上电的两次偏差较大的采样值
		{
			adc_value += tmp_value;
		}
	}
	
	adc_value = adc_value/3;
	ADC_Block_Release();

	*tmp_adc = adc_value;
#ifdef FOUR_EXP
	for(i = 0; sizeof(adc_sample_value)/sizeof(adc_sample_value[0]);i++)
	{
		if(adc_value <= adc_sample_value[i])
		{
			*tmp_exp = isp_exp_value[i];
			break;
		}
	}
	
	if(i == sizeof(adc_sample_value)/sizeof(adc_sample_value[0]))
	{
		*tmp_exp = isp_exp_value[i];
	}
#else
	if(adc_value < IRCUT_CHARGE_NIGHT_VALUE)
	{
		*tmp_exp = 2012;
	}
	else
	{
		*tmp_exp = 1509;
	}
#endif

	//IRCUT
	if(2 == ircut_status)
	{
		if(adc_value < IRCUT_CHARGE_DAY_VALUE) //
		{
			ircut_option(0);
			set_day_night_mode(NIGHT_MODE);
		}
		else
		{
			ircut_option(1);
			set_day_night_mode(DAY_MODE);
		}
	}
	else if(0 == ircut_status && adc_value < IRCUT_CHARGE_NIGHT_VALUE) //
	{
		ircut_option(0);
		set_day_night_mode(NIGHT_MODE);
	}
	else if(1 == ircut_status && adc_value > IRCUT_CHARGE_DAY_VALUE)
	{
		ircut_option(1);
		set_day_night_mode(DAY_MODE);
	}
	
}
#else

#define DAY_LEVEL 1
void  io_sample_ircut(unsigned char *level)
{
	unsigned char tmp = 0;
#ifdef VDDIO
	TIM2_DelayMs(1);
#endif	
	GPIO_Init(GPIOC , GPIO_Pin_4 , GPIO_Mode_In_FL_No_IT);		//配置PC4为浮空输入
	
	tmp = (GPIO_ReadInputData(GPIOC)&GPIO_Pin_4)>>4;
	if(DAY_LEVEL == tmp)
	{
		ircut_option(1);
		updata_exp_time(1509);
		set_day_night_mode(DAY_MODE);
	}
	else
	{
		ircut_option(0);
		updata_exp_time(2012);
		set_day_night_mode(NIGHT_MODE);
	}

	*level = tmp;
}

#endif

//0xFF00 ok
static void key_even(void)
{
	static  u8 data_value = 0;

	u16 data = GPIO_Pin_4;
	u8  gpio_val;
	u8 tmp = 0;

	get_key_flg(&tmp);
	
	if(tmp)
	{
		get_time_count(&data_value);
		
		TIM2_Init(1);//50ms 		
		TIM2_Cmd(ENABLE);                //启动定时器2开始计数

		set_key_flg();
	}
	#define KEY_TIME_HOLD 4
	get_time_count(&tmp);

	if(tmp >= data_value)
	{
		tmp = tmp - data_value;
	}
	else
	{
		tmp = 0xff - data_value + tmp;  // 
	}
	
	gpio_val = GPIO_ReadInputData(GPIOB);
	if((0 ==(gpio_val&data))&&(tmp == KEY_TIME_HOLD) )//&& (data_value == stm_time_count)
	{		
		TIM2_Cmd(DISABLE);               //启动定时器2开始计数
		set_key_flg();
		
		data = RINGCALL_DINGDONG_EVEN;//	   dingdong
		ak_stm_uart(data);//
	}
	
	if(tmp > 6)//300ms 
	{
		TIM2_Cmd(DISABLE);               //启动定时器2开始计数
		set_key_flg();
	}
		
}

int main(void)
{
	char ak_stm_led_flg = 0;
	u8 tmp = 0;
	unsigned char statu = 0;
#ifdef ADC_ENABLE
	u16 exp_data = 0, adc_data = 0;
#endif
	disableInterrupts();					//关闭系统总中断
	CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);  //内部时钟为1分频 = 16Mhz ;	
	port_Init();          //调用GPIO初始化函数
	
	TIM2_DelayMs(3);//  延时防止 pmu 上电堵死，不能去掉 

    enableInterrupts();   //使能系统总中断
    
    pir_wifiwakeup_int_disable();
	set_wakeup_type();
	
	if(wifi_info_reset())
	{
		reset_wifi_flg = WIFI_CONFIG_CLEAR_EVEN;
	}
	
	while(1) //进入死循环，等待外部端口D中断产生--KEY1
	{	
		//LED		
		if(0 == ak_stm_led_flg)
		{
			ak_stm_led(1);
			ak_stm_led_flg = 1;
		}
		
		if(0 == ak_power_flg)
		{
			power_ak(1);
#ifdef ADC_ENABLE
			adc_ircut(&exp_data, &adc_data);
			updata_exp_adc(exp_data, adc_data);

#else
			io_sample_ircut(&tmp);
#endif
			
		}
		else
		{
			key_even();			
		}
		
	
		get_uart_flg(&tmp);
		if(tmp) // UART PROCESS
		{
			
			ak_stm_uart(reset_wifi_flg);// 根据回应判断是否清楚配置
			reset_wifi_flg = 0;
			set_uart_flg();//AK 有心跳
		}
		
		if(0 == ak_power_flg)
		{
			ak_stm_led(0);
			GPIO_ResetBits(GPIOB, GPIO_Pin_2);// POWER LED
			ak_stm_led_flg = 0;
			statu = (GPIO_ReadInputData(GPIOB)&GPIO_Pin_1)>>1;
			set_wakeup_type();
			if(0 != statu)
			{
				halt(); //进入低功耗停机模式CPU停止工作，所有外设停止工作，只有外部中断工作；等待外部中断产生中断，会将低CPU唤醒，继续向下执行
			}
		}
		else
		{
			wfi(); //进入低功耗等待模式，CPU停止工作，所有外设还工作，等待产生中断，会将低CPU唤醒，继续向下执行
		}
		

	}

}


