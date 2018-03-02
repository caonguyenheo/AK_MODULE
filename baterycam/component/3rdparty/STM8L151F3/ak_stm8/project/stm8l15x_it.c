/**
  ******************************************************************************
  * @file    Project/STM8L15x_StdPeriph_Template/stm8l15x_it.c
  * @author  MCD Application Team
  * @version V1.6.1
  * @date    30-September-2014
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm8l15x_it.h"

#include "usart1.h"

#include "ak_stm8_common_it.h"

static volatile unsigned char wakeup_type = 0;


void get_wakeup_type(unsigned char *data)
{
	unsigned char *tmp = data;
	*tmp = wakeup_type;
}


void set_wakeup_type(void)
{
	wakeup_type = 0;
}


#if 0
static void TIM3_Start(void)
{

  CLK_PeripheralClockConfig(CLK_Peripheral_TIM3 , ENABLE);              //使能定时器3时钟
  TIM3_TimeBaseInit(TIM3_Prescaler_16 , TIM3_CounterMode_Up , 1000);    //  1 minute
  TIM3_ITConfig(TIM3_IT_Update , ENABLE);     //使能向上计数溢出中断
  TIM3_ARRPreloadConfig(ENABLE);  //使能定时器3自动重载功能    
  TIM3_Cmd(ENABLE);               //启动定时器3开始计数

}

static void TIM3_Stop(void)
{

  CLK_PeripheralClockConfig(CLK_Peripheral_TIM3 , DISABLE);              //使能定时器3时钟
  TIM3_ITConfig(TIM3_IT_Update , DISABLE);     //使能向上计数溢出中断
  TIM3_ARRPreloadConfig(DISABLE);  //使能定时器3自动重载功能    
  TIM3_Cmd(DISABLE);               //启动定时器3开始计数
}
#endif
/** @addtogroup STM8L15x_StdPeriph_Template
  * @{
  */
	
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/

#ifdef _COSMIC_
/**
  * @brief Dummy interrupt routine
  * @par Parameters:
  * None
  * @retval 
  * None
*/
INTERRUPT_HANDLER(NonHandledInterrupt,0)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
#endif

/**
  * @brief TRAP interrupt routine
  * @par Parameters:
  * None
  * @retval 
  * None
*/
INTERRUPT_HANDLER_TRAP(TRAP_IRQHandler)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief FLASH Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(FLASH_IRQHandler,1)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief DMA1 channel0 and channel1 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(DMA1_CHANNEL0_1_IRQHandler,2)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief DMA1 channel2 and channel3 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(DMA1_CHANNEL2_3_IRQHandler,3)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief RTC / CSS_LSE Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(RTC_CSSLSE_IRQHandler,4)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief External IT PORTE/F and PVD Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTIE_F_PVD_IRQHandler,5)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief External IT PORTB / PORTG Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTIB_G_IRQHandler,6)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
	#if 0	
 	if(EXTI_GetITStatus(EXTI_IT_Pin0) !=RESET)
  	{
    	 EXTI_ClearITPendingBit(EXTI_IT_Pin0);//清除标志位
    	 if(0 == wakeup_type)
    	 {
			wakeup_type = EVENT_PIR_WAKEUP; //PIR
		 }
  	}
	
 	if(EXTI_GetITStatus(EXTI_IT_Pin4) !=RESET)
  	{
    	 EXTI_ClearITPendingBit(EXTI_IT_Pin4);//清除标志位
    	 if(0 == wakeup_type)
    	 {
			wakeup_type = EVENT_RING_CALL_WAKEUP; //CALL
		 }
  	}

	//#else		
  	if(EXTI_GetITStatus(EXTI_IT_PortB) !=RESET)
  	{
    	 //GPIO_ToggleBits(GPIOB , GPIO_Pin_2);   //异或取反控制LED1灯
    	 EXTI_ClearITPendingBit(EXTI_IT_PortB);//清除标志位
    	  	 
     	 if(0 == wakeup_type)
    	 {
	    	 if(GPIO_ReadInputData(GPIOB)&0x1)//pb0 PIR HL
	    	 {
				wakeup_type = EVENT_PIR_WAKEUP; //PIR
			 }
			 else
			 {
				wakeup_type = EVENT_RING_CALL_WAKEUP; //CALL

			 }
		 }
  	}
	#endif
}

//volatile unsigned char gpio_int_test = 0;
/**
  * @brief External IT PORTD /PORTH Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTID_H_IRQHandler,7)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */

	#if 0
	#if 0
 	if(EXTI_GetITStatus(EXTI_IT_Pin0) !=RESET)
  	{
    	 EXTI_ClearITPendingBit(EXTI_IT_Pin0);//清除标志位
    	 GPIO_ToggleBits(GPIOB , GPIO_Pin_2);   //异或取反控制LED1灯
    	 if(0 == wakeup_type)
    	 {
			wakeup_type = EVENT_WIFI_WAKEUP; //wifi
		 }
  	}
	#else
    if(EXTI_GetITStatus(EXTI_IT_PortD) != RESET)
    {
		EXTI_ClearITPendingBit(EXTI_IT_PortD);//清除标志位
		#if 0
		GPIO_ToggleBits(GPIOB, GPIO_Pin_1);   //异或取反控制LED1灯

		#else
		if(0 == wakeup_type)
		{
			wakeup_type = EVENT_VIDEO_PREVIEW_WAKEUP; //wifi
		}
		#endif
    }
	#endif

	#endif
}

/**
  * @brief External IT PIN0 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI0_IRQHandler,8)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
#if 1
    if(EXTI_GetITStatus(EXTI_IT_Pin0) != RESET)
    {
		EXTI_ClearITPendingBit(EXTI_IT_Pin0);//清除标志位

		#if 0
		GPIO_ToggleBits(GPIOA, GPIO_Pin_2);   //异或取反控制LED1灯

		#else
		if(0 == wakeup_type)
		{
			wakeup_type = EVENT_PIR_WAKEUP; //PIR
		}
		#endif
		pir_wifiwakeup_int_disable();
    }

#endif
	
}

/**
  * @brief External IT PIN1 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI1_IRQHandler,9)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */

#if 1
    if(EXTI_GetITStatus(EXTI_IT_Pin1) != RESET)
    {
		EXTI_ClearITPendingBit(EXTI_IT_Pin1);//清除标志位

		#if 0
		if(0 == wakeup_type)
		{
		//1.关闭gpio中断
			GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_No_IT);
		//2.启动定时器10ms之后
			TIM3_Start();
		}
		#else
		if(0 == wakeup_type)
		{
			wakeup_type = EVENT_VIDEO_PREVIEW_WAKEUP; //WIFI
		}
		pir_wifiwakeup_int_disable();
		//GPIO_ToggleBits(GPIOA, GPIO_Pin_2);// POWER LED
		#endif
		
    }

#endif


	
}

/**
  * @brief External IT PIN2 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI2_IRQHandler,10)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief External IT PIN3 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI3_IRQHandler,11)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}


static volatile char ak_stm_key_flg ;

void get_key_flg(unsigned char *data)
{
	unsigned char *tmp = data;
	*tmp = ak_stm_key_flg;
}


void set_key_flg(void)
{
	ak_stm_key_flg = 0;
}



/**
  * @brief External IT PIN4 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI4_IRQHandler,12)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */

#if 1	
    if(EXTI_GetITStatus(EXTI_IT_Pin4) != RESET)
    {
		EXTI_ClearITPendingBit(EXTI_IT_Pin4);//清除标志位

		#if 0
		GPIO_ToggleBits(GPIOB , GPIO_Pin_2);   //异或取反

		#else
		if(0 == wakeup_type)
		{
			wakeup_type = EVENT_RING_CALL_WAKEUP; //call
		}
		#endif
		ak_stm_key_flg = 1;
		
    }


#endif
	
}

/**
  * @brief External IT PIN5 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI5_IRQHandler,13)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief External IT PIN6 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI6_IRQHandler,14)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief External IT PIN7 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI7_IRQHandler,15)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief LCD /AES Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(LCD_AES_IRQHandler,16)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief CLK switch/CSS/TIM1 break Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(SWITCH_CSS_BREAK_DAC_IRQHandler,17)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief ADC1/Comparator Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(ADC1_COMP_IRQHandler,18)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief TIM2 Update/Overflow/Trigger/Break /USART2 TX Interrupt routine.
  * @param  None
  * @retval None
  */

static volatile unsigned char stm_time_count = 0;

void get_time_count(unsigned char *data)
{
	unsigned char *tmp = data;
	*tmp = stm_time_count;
}

INTERRUPT_HANDLER(TIM2_UPD_OVF_TRG_BRK_USART2_TX_IRQHandler,19)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
    #if 0
    if(TIM2_GetITStatus(TIM2_IT_Update) != RESET)
    {
       TIM2_ClearITPendingBit(TIM2_IT_Update); //清除中断标志
    }
	#endif
	if(TIM2_GetFlagStatus(TIM2_FLAG_Update) != RESET)
	{
		TIM2_ClearITPendingBit(TIM2_IT_Update); //清除中断标志
		stm_time_count++;
	}
}

/**
  * @brief Timer2 Capture/Compare / USART2 RX Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM2_CC_USART2_RX_IRQHandler,20)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}




/**
  * @brief Timer3 Update/Overflow/Trigger/Break Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM3_UPD_OVF_TRG_BRK_USART3_TX_IRQHandler,21)
{
#if 0	
	unsigned char tmp = 0;
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
	if(TIM3_GetFlagStatus(TIM3_FLAG_Update) != RESET)
	{
		TIM3_ClearITPendingBit(TIM3_IT_Update); //清除中断标志
		//1.读取PB1的电平，若为低则更改唤醒源状态
		tmp = (GPIO_ReadInputData(GPIOB)&GPIO_Pin_1)>>1;
		if(0 == tmp)
		{
			wakeup_type = EVENT_VIDEO_PREVIEW_WAKEUP; //WIFI
			pir_wifiwakeup_int_disable();
		}
		else
		{
		//2.打开PB1的中断
			GPIO_Init(GPIOB, GPIO_Pin_1 , GPIO_Mode_In_FL_IT);
		}
		//3.关闭time3中断
		TIM3_Stop();

		
	}

#endif	
}
/**
  * @brief Timer3 Capture/Compare /USART3 RX Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM3_CC_USART3_RX_IRQHandler,22)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief TIM1 Update/Overflow/Trigger/Commutation Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM1_UPD_OVF_TRG_COM_IRQHandler,23)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief TIM1 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM1_CC_IRQHandler,24)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief TIM4 Update/Overflow/Trigger Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM4_UPD_OVF_TRG_IRQHandler,25)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @brief SPI1 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(SPI1_IRQHandler,26)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */		
}

/**
  * @brief USART1 TX / TIM5 Update/Overflow/Trigger/Break Interrupt  routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(USART1_TX_TIM5_UPD_OVF_TRG_BRK_IRQHandler,27)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}

/**
  * @brief USART1 RX / Timer5 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */

static volatile unsigned char rx_uart_data = 0;
static  volatile char ak_stm_uart_flg = 0;;
extern unsigned char ak_stm_cmd_buf[sizeof(CMD_INFO)];


void get_uart_flg(unsigned char *data)
{
	unsigned char *tmp = data;
	*tmp = ak_stm_uart_flg;
}


void set_uart_flg(void)
{
	ak_stm_uart_flg = 0;
}


INTERRUPT_HANDLER(USART1_RX_TIM5_CC_IRQHandler,28)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
	unsigned char tmp;

	
  if(USART_GetITStatus(USART1 , USART_IT_RXNE) != RESET)
  {
    USART_ClearITPendingBit(USART1 , USART_IT_RXNE);   //接收到数据，清除中断标志
    #if 1
    tmp = USART_ReceiveData8(USART1);
	
	//GPIO_ToggleBits(GPIOA, GPIO_Pin_2);   //异或取反
	if(0 == ak_stm_uart_flg)
	{

		if(CMD_PREAMBLE == tmp)
		{
			ak_stm_cmd_buf[rx_uart_data] = tmp;
			rx_uart_data++;
			return;
		}
		
		if(rx_uart_data)
		{
			ak_stm_cmd_buf[rx_uart_data] = tmp;
			rx_uart_data++;
			
			if(sizeof(CMD_INFO) == rx_uart_data)//接收完成一条命令
			{
				rx_uart_data = 0;
				ak_stm_uart_flg = 1;
			}
		}
	}
	
	#else
    USART_SendData8(USART1 , USART_ReceiveData8(USART1));   //把接收到的数据，返回去
	#endif
  }
}

/**
  * @brief I2C1 / SPI2 Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(I2C1_SPI2_IRQHandler,29)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
}
/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
