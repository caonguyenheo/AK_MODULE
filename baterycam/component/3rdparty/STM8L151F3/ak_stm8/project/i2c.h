#ifndef ___I2C_H___
#define ___I2C_H___

//#include"stm8l15x.h"

void IIC_Init(unsigned long OutputClockFrequency, unsigned short OwnAddress);


void IIC_Write(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num);

void IIC_Read(unsigned char subaddr , unsigned char Byte_addr , unsigned char *buffer , unsigned short num);

void IIC_Close();

#endif

