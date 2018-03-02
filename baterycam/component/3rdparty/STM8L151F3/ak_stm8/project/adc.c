//#include"stm8l15x.h"
#include "stm8l15x_conf.h"

#include"adc.h"


void ADC_Block_Init(void)
{

  CLK_PeripheralClockConfig(CLK_Peripheral_ADC1 , ENABLE);    //��ʹ��ADC1ģ���ʱ��
  
  GPIO_Init(GPIOC , GPIO_Pin_4 , GPIO_Mode_In_FL_No_IT);      //����PC4Ϊ��������
  ADC_Init(ADC1 ,                         //����ADC1ģ��
           ADC_ConversionMode_Single ,    //���õ���ת��ģʽ
           ADC_Resolution_12Bit ,         //����Ϊ12λ����
           ADC_Prescaler_2                //ADCʱ��Ϊϵͳ��2��Ƶ������8M
           );
  
  ADC_ChannelCmd(ADC1 ,                   //����ADC1ģ��
                 ADC_Channel_4,         //����Ϊģ��ͨ��18
                 ENABLE                   //ʹ��
                 );
  ADC_Cmd(ADC1 , ENABLE);                 //ʹ��ADC1ģ��
}

void ADC_Block_Release(void)
{
  	 
	ADC_Cmd(ADC1 , DISABLE);                 //ʹ��ADC1ģ��
  
  	ADC_ChannelCmd(ADC1 , 				  //����ADC1ģ��
				  ADC_Channel_4,		 //����Ϊģ��ͨ��18
				  DISABLE					//ʹ��
				);	
  
  	ADC_DeInit(ADC1);
                 	
	CLK_PeripheralClockConfig(CLK_Peripheral_ADC1 , DISABLE);	  //��ʹ��ADC1ģ���ʱ��


}

void Read_ADC_Data(u16 *ADC_Data_)
{
	ADC_SoftwareStartConv(ADC1);    //����ADC1��ʼת��

	while(ADC_GetFlagStatus(ADC1 , ADC_FLAG_EOC) != RESET);  //�ȴ�ת������
	ADC_ClearFlag(ADC1 , ADC_FLAG_EOC);       //���������־λ
	*ADC_Data_ = ADC_GetConversionValue(ADC1);    //��ȡת����ֵ

	//save power

	//CLK_PeripheralClockConfig(CLK_Peripheral_ADC1 , DISABLE);    //��ʹ��ADC1ģ���ʱ��
	//ADC_Cmd(ADC1 , DISABLE);                 //ʹ��ADC1ģ��  
}



