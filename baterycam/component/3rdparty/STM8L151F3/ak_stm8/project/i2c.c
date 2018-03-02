

/*    ��Ӱ���оƬ��ͷ�ļ�    */
#include"stm8l15x.h"
#include "stm8l15x_conf.h"

#include"i2c.h"






/*******************************************************************************
**��������:void IIC_Init() 
**��������:��ʼ��IIC1�ӿ�
**��ڲ���:
**���:��
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
**��������:void IIC_Write(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
**��������:��IIC����д����
**��ڲ���:
          subaddr :  ��������ַ
          Byte_addr : ȷ������д��ַ����ʼ��ַ
          *buffer   : д���ݵ���ַ��ַ
          num				: Ҫд���ݵĸ���
**���:��
*******************************************************************************/
void IIC_Write(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
{
	
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)); //|I����!?y???D
    //!��!��?��a?e��o?D?o?/*!< Send STRAT condition */
    I2C_GenerateSTART(I2C1, ENABLE);  
    
    /*!< Test on EV5 and clear it *///�ȴ���ʼ�źŲ���
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    
      /*!< Send EEPROM address for write *///���������ص�ַ�������SB��־λ
    I2C_Send7bitAddress(I2C1, (u8)subaddr, I2C_Direction_Transmitter);
    /*!< Test on EV6 and clear it *///�ȴ�������ַ������ɲ��������������ַ��־λ
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    /*!< Send the EEPROM's internal address to write to : LSB of the address */
    I2C_SendData(I2C1, (u8)(Byte_addr));//���������洢�׵�ַ
    /*!< Test on EV8 and clear it *///�ȴ���λ�������������
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
      
    while(num > 0)
    { /*!< Send the byte to be written *///���������洢�׵�ַ
        I2C_SendData(I2C1, *buffer);
        /*!< Test on EV8 and clear it *///�ȴ���λ�������������
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

        buffer++;
        num--;
    }	
    /*!< Send STOP condition *///����ֹͣ�źŽ������ݴ���
    I2C_GenerateSTOP(I2C1 , ENABLE);
}

/*******************************************************************************
**��������:void IIC_Read(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
**��������:��IIC����������
**��ڲ���:
          subaddr :  ��������ַ
          Byte_addr : ȷ������д��ַ����ʼ��ַ
          *buffer   : �����ݵĻ�������ʼ��ַ
          num				: Ҫ�����ݵĸ���
**���:��
*******************************************************************************/
void IIC_Read(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num)
{
  /*!< While the bus is busy */
  while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
  
  /*!< Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C1, ENABLE);//����Ӧ���ź�		
  
  /*!< Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);//������ʼ�ź�
  /*!< Test on EV5 and clear it */
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));//�ȴ���ʼ�źŲ���
  
  /*!< Send EEPROM address for write *///���������ص�ַ�������SB��־λ
  I2C_Send7bitAddress(I2C1, (u8)subaddr, I2C_Direction_Transmitter);
  /*!< Test on EV6 and clear it */
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));//�ȴ�������ַ�������

  /*!< Send the EEPROM's internal address to read from: LSB of the address */
  I2C_SendData(I2C1, (u8)(Byte_addr));//���ʹ洢��ַ
  /*!< Test on EV8 and clear it */
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));//�ȴ���λ�������������

  
   /*!< Send STRAT condition a second time */
  I2C_GenerateSTART(I2C1, ENABLE); //���·�����ʼ�ź�
  /*!< Test on EV5 and clear it *///�ȴ���ʼ�źŲ���
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

   /*!< Send EEPROM address for read *///���������ص�ַ�������SB��־λ
  I2C_Send7bitAddress(I2C1, (u8)subaddr, I2C_Direction_Receiver);

  /*!< Test on EV6 and clear it *///�ȴ�������ַ�������
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));


  while(num)
  {
	if(num == 1)
	{	/*!< Disable Acknowledgement */
	  I2C_AcknowledgeConfig(I2C1, DISABLE);//���һ���ֽڲ�����Ӧ���ź� 		
		/*!< Send STOP Condition */
	  I2C_GenerateSTOP(I2C1, ENABLE);//����ֹͣ�źŽ������ݴ��� 		
	}
	/*!< Test on EV7 and clear it */ //�ȴ����ݽ������
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));

	*buffer = I2C_ReceiveData(I2C1);  //��������
	buffer++;	//�������ݻ����ַ�ݼ�
	num--;		//����������Ŀ��1
  }
}
void IIC_Close()
{	
		asm("sim"); 

	CLK_PeripheralClockConfig(CLK_Peripheral_I2C1 , DISABLE);              //1?��o1?��1IIC1��o!A?��?

  	I2C_Cmd(I2C1, DISABLE);
	I2C_DeInit(I2C1);
		asm("rim"); 

}








