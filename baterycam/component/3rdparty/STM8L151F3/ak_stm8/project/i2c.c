

/*    添加包含芯片的头文件    */
#include"stm8l15x.h"
#include "stm8l15x_conf.h"

#include"i2c.h"






/*******************************************************************************
**函数名称:void IIC_Init() 
**功能描述:初始化IIC1接口
**入口参数:
**输出:无
*******************************************************************************/
void IIC_Init(unsigned long OutputClockFrequency, unsigned short OwnAddress)
{					
				
	asm("sim"); 					//???????
  	CLK_PeripheralClockConfig(CLK_Peripheral_I2C1 , ENABLE);              //??IIC1??

  	I2C_Cmd(I2C1, ENABLE);
  
  	/* sEE_I2C configuration after enabling it */
  	I2C_Init(I2C1, OutputClockFrequency, OwnAddress, I2C_Mode_I2C, I2C_DutyCycle_2,
           I2C_Ack_Enable, I2C_AcknowledgedAddress_7bit);
	
	asm("rim"); 					//???????
}

/*******************************************************************************
**函数名称:void IIC_Write(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
**功能描述:向IIC器件写数据
**入口参数:
          subaddr :  从器件地址
          Byte_addr : 确定器件写地址的起始地址
          *buffer   : 写数据的起址地址
          num				: 要写数据的个数
**输出:无
*******************************************************************************/
void IIC_Write(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
{
	
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)); //|I¨¨!?y???D
    //!¤!é?¨a?e¨o?D?o?/*!< Send STRAT condition */
    I2C_GenerateSTART(I2C1, ENABLE);  
    
    /*!< Test on EV5 and clear it *///等待起始信号产生
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    
      /*!< Send EEPROM address for write *///发送器件地地址，并清除SB标志位
    I2C_Send7bitAddress(I2C1, (u8)subaddr, I2C_Direction_Transmitter);
    /*!< Test on EV6 and clear it *///等待器件地址发送完成并清除发送器件地址标志位
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    /*!< Send the EEPROM's internal address to write to : LSB of the address */
    I2C_SendData(I2C1, (u8)(Byte_addr));//发送器件存储首地址
    /*!< Test on EV8 and clear it *///等待移位发送器发送完成
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
      
    while(num > 0)
    { /*!< Send the byte to be written *///发送器件存储首地址
        I2C_SendData(I2C1, *buffer);
        /*!< Test on EV8 and clear it *///等待移位发送器发送完成
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

        buffer++;
        num--;
    }	
    /*!< Send STOP condition *///发送停止信号结束数据传输
    I2C_GenerateSTOP(I2C1 , ENABLE);
}

/*******************************************************************************
**函数名称:void IIC_Read(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
**功能描述:向IIC器件读数据
**入口参数:
          subaddr :  从器件地址
          Byte_addr : 确定器件写地址的起始地址
          *buffer   : 读数据的缓冲区起始地址
          num				: 要读数据的个数
**输出:无
*******************************************************************************/
void IIC_Read(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
{
  /*!< While the bus is busy */
  while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
  
  /*!< Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C1, ENABLE);//产生应答信号		
  
  /*!< Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);//发送起始信号
  /*!< Test on EV5 and clear it */
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));//等待起始信号产生
  
  /*!< Send EEPROM address for write *///发送器件地地址，并清除SB标志位
  I2C_Send7bitAddress(I2C1, (u8)subaddr, I2C_Direction_Transmitter);
  /*!< Test on EV6 and clear it */
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));//等待器件地址发送完成

  /*!< Send the EEPROM's internal address to read from: LSB of the address */
  I2C_SendData(I2C1, (u8)(Byte_addr));//发送存储地址
  /*!< Test on EV8 and clear it */
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));//等待移位发送器发送完成

  
   /*!< Send STRAT condition a second time */
  I2C_GenerateSTART(I2C1, ENABLE); //重新发送起始信号
  /*!< Test on EV5 and clear it *///等待起始信号产生
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

   /*!< Send EEPROM address for read *///发送器件地地址，并清除SB标志位
  I2C_Send7bitAddress(I2C1, (u8)subaddr, I2C_Direction_Receiver);

  /*!< Test on EV6 and clear it *///等待器件地址发送完成
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));


  while(num)
  {
	if(num == 1)
	{	/*!< Disable Acknowledgement */
	  I2C_AcknowledgeConfig(I2C1, DISABLE);//最后一个字节不产生应答信号 		
		/*!< Send STOP Condition */
	  I2C_GenerateSTOP(I2C1, ENABLE);//发送停止信号结束数据传输 		
	}
	/*!< Test on EV7 and clear it */ //等待数据接收完成
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));

	*buffer = I2C_ReceiveData(I2C1);  //读出数据
	buffer++;	//读出数据缓存地址递加
	num--;		//接收数据数目减1
  }
}
void IIC_Close()
{	
		asm("sim"); 

	CLK_PeripheralClockConfig(CLK_Peripheral_I2C1 , DISABLE);              //1?¨o1?¨1IIC1¨o!A?¨?

  	I2C_Cmd(I2C1, DISABLE);
	I2C_DeInit(I2C1);
		asm("rim"); 

}








