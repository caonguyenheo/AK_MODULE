//#include"stm8l15x.h"
#include "stm8l15x_conf.h"

#include"adc.h"


void ADC_Block_Init(void)
{

  CLK_PeripheralClockConfig(CLK_Peripheral_ADC1 , ENABLE);    //先使能ADC1模块的时钟
  
  GPIO_Init(GPIOC , GPIO_Pin_4 , GPIO_Mode_In_FL_No_IT);      //配置PC4为浮空输入
  ADC_Init(ADC1 ,                         //配置ADC1模块
           ADC_ConversionMode_Single ,    //采用单次转换模式
           ADC_Resolution_12Bit ,         //设置为12位精度
           ADC_Prescaler_2                //ADC时钟为系统的2分频，即是8M
           );
  
  ADC_ChannelCmd(ADC1 ,                   //配置ADC1模块
                 ADC_Channel_4,         //配置为模块通道18
                 ENABLE                   //使能
                 );
  ADC_Cmd(ADC1 , ENABLE);                 //使能ADC1模块
}

void ADC_Block_Release(void)
{
  	 
	ADC_Cmd(ADC1 , DISABLE);                 //使能ADC1模块
  
  	ADC_ChannelCmd(ADC1 , 				  //配置ADC1模块
				  ADC_Channel_4,		 //配置为模块通道18
				  DISABLE					//使能
				);	
  
  	ADC_DeInit(ADC1);
                 	
	CLK_PeripheralClockConfig(CLK_Peripheral_ADC1 , DISABLE);	  //先使能ADC1模块的时钟


}

void Read_ADC_Data(u16 *ADC_Data_)
{
	ADC_SoftwareStartConv(ADC1);    //启动ADC1开始转换

	while(ADC_GetFlagStatus(ADC1 , ADC_FLAG_EOC) != RESET);  //等待转换结束
	ADC_ClearFlag(ADC1 , ADC_FLAG_EOC);       //清除结束标志位
	*ADC_Data_ = ADC_GetConversionValue(ADC1);    //读取转换数值

	//save power

	//CLK_PeripheralClockConfig(CLK_Peripheral_ADC1 , DISABLE);    //先使能ADC1模块的时钟
	//ADC_Cmd(ADC1 , DISABLE);                 //使能ADC1模块  
}



