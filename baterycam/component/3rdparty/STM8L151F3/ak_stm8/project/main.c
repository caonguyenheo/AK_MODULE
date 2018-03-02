
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



//#define IR_IS_DAY  isp_exp_value[0]     //�������ֵ����Ҫ����ʵ��ȷ��


/*******************************************************************************
**�������ƣ�void TIM2_Init()     Name: void TIM2_Init()
**������������ʼ����ʱ��2
**��ڲ�����
**�������
*******************************************************************************/
void TIM2_Init(u8 enable)
{
	u8  mask = enable;
	
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2 , ENABLE);              //ʹ�ܶ�ʱ��2ʱ��
	TIM2_TimeBaseInit(TIM2_Prescaler_16 , TIM2_CounterMode_Up , 50000);    //���ö�ʱ��2Ϊ16��Ƶ�����ϼ���������ֵΪ50000��Ϊ50����ļ���ֵ

	if(mask)
	{
		TIM2_ITConfig(TIM2_IT_Update , ENABLE);     //ʹ�����ϼ�������ж�
		TIM2_ARRPreloadConfig(ENABLE);  //ʹ�ܶ�ʱ��3�Զ����ع���    

	}
	else
	{
		TIM2_ITConfig(TIM2_IT_Update , DISABLE);     //ʹ�����ϼ�������ж�
	}
	
}

/*******************************************************************************
**�������ƣ�void TIM2_DelayMs(unsigned int ms)
**������������ʱ��2�ν��о�ȷ��ʱ��
**��ڲ�����unsigned int ms     ms :��λ50ms
**�������
*******************************************************************************/
void TIM2_DelayMs(unsigned int ms)
{ 
	TIM2_Init(0);	
	TIM2_ARRPreloadConfig(ENABLE);  //ʹ�ܶ�ʱ��2�Զ����ع���    
	TIM2_Cmd(ENABLE);               //������ʱ��2��ʼ����
	while(ms--)
	{
		while( TIM2_GetFlagStatus(TIM2_FLAG_Update) == RESET); //�ȴ������Ƿ�ﵽ1����
		TIM2_ClearFlag(TIM2_FLAG_Update);  //�������x���룬�����Ӧ�ı�־
	}
	TIM2_Cmd(DISABLE);                   //��ʱȫ���������رն�ʱ��2
	TIM2_ARRPreloadConfig(DISABLE);  //ʹ�ܶ�ʱ��2�Զ����ع���  
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2 , DISABLE);              //ʹ�ܶ�ʱ��2ʱ��

}



void pir_wifiwakeup_int_enable(void)
{
	GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_IT); //��ʼ��WIFI������PD_0Ϊ�����������벢��ʹ���ж�ģʽ
	GPIO_Init(GPIOB, GPIO_Pin_0 , GPIO_Mode_In_FL_IT); //��ʼ��PIR������PD_0Ϊ�����������벢��ʹ���ж�ģʽ   GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
}

void pir_wifiwakeup_int_disable(void)
{
	GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_No_IT); //��ʼ��WIFI������PD_0Ϊ�����������벢��ʹ���ж�ģʽ
	GPIO_Init(GPIOB, GPIO_Pin_0 , GPIO_Mode_In_FL_No_IT); //��ʼ��PIR������PD_0Ϊ�����������벢��ʹ���ж�ģʽ   GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
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
	GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_IT);           //��ʼ��WIFI������PD_0Ϊ�����������벢��ʹ���ж�ģʽ  GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
	EXTI_SetPinSensitivity(EXTI_Pin_1 ,  EXTI_Trigger_Falling_Low);//EXTI_Trigger_Falling);  //PortD�˿�Ϊ�½��ش����ж�  EXTI_Trigger_Falling    EXTI_Trigger_Rising

	// PIR WAKEUP
	GPIO_Init(GPIOB, GPIO_Pin_0 , GPIO_Mode_In_FL_IT); //��ʼ��PIR������PD_0Ϊ�����������벢��ʹ���ж�ģʽ   GPIO_Mode_In_FL_IT  GPIO_Mode_In_PU_IT
	EXTI_SetPinSensitivity(EXTI_Pin_0,  EXTI_Trigger_Rising);  //PortD�˿�Ϊ�½��ش����ж�  EXTI_Trigger_Rising  EXTI_Trigger_Falling

	//KEY WAKEUP
	GPIO_Init(GPIOB, GPIO_Pin_4 , GPIO_Mode_In_PU_IT);           //��ʼ��CALL������PD_0Ϊ�����������벢��ʹ���ж�ģʽ
	EXTI_SetPinSensitivity(EXTI_Pin_4 ,  EXTI_Trigger_Falling);  //PortD�˿�Ϊ�½��ش����ж�  EXTI_Trigger_Rising  EXTI_Trigger_Falling
	
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
		if(i>1)//����ǰ����ϵ������ƫ��ϴ�Ĳ���ֵ
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
	GPIO_Init(GPIOC , GPIO_Pin_4 , GPIO_Mode_In_FL_No_IT);		//����PC4Ϊ��������
	
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
		TIM2_Cmd(ENABLE);                //������ʱ��2��ʼ����

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
		TIM2_Cmd(DISABLE);               //������ʱ��2��ʼ����
		set_key_flg();
		
		data = RINGCALL_DINGDONG_EVEN;//	   dingdong
		ak_stm_uart(data);//
	}
	
	if(tmp > 6)//300ms 
	{
		TIM2_Cmd(DISABLE);               //������ʱ��2��ʼ����
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
	disableInterrupts();					//�ر�ϵͳ���ж�
	CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);  //�ڲ�ʱ��Ϊ1��Ƶ = 16Mhz ;	
	port_Init();          //����GPIO��ʼ������
	
	TIM2_DelayMs(3);//  ��ʱ��ֹ pmu �ϵ����������ȥ�� 

    enableInterrupts();   //ʹ��ϵͳ���ж�
    
    pir_wifiwakeup_int_disable();
	set_wakeup_type();
	
	if(wifi_info_reset())
	{
		reset_wifi_flg = WIFI_CONFIG_CLEAR_EVEN;
	}
	
	while(1) //������ѭ�����ȴ��ⲿ�˿�D�жϲ���--KEY1
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
			
			ak_stm_uart(reset_wifi_flg);// ���ݻ�Ӧ�ж��Ƿ��������
			reset_wifi_flg = 0;
			set_uart_flg();//AK ������
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
				halt(); //����͹���ͣ��ģʽCPUֹͣ��������������ֹͣ������ֻ���ⲿ�жϹ������ȴ��ⲿ�жϲ����жϣ��Ὣ��CPU���ѣ���������ִ��
			}
		}
		else
		{
			wfi(); //����͹��ĵȴ�ģʽ��CPUֹͣ�������������軹�������ȴ������жϣ��Ὣ��CPU���ѣ���������ִ��
		}
		

	}

}


