/**
 * @file i2c.c
 * @brief I2C interface driver, define I2C interface APIs.
 *
 * This file provides I2C APIs: I2C initialization, write data to I2C & read data from I2C.
 * Copyright (C) 2004 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author Guanghua Zhang
 * @date 2017-09-30
 * @version 1.0
 * @ref 
 */
 
#include"stm8l15x.h"
#include "stm8l15x_gpio.h"

#include "i2c_software.h"




static unsigned char I2C_SCL = 0xf0;    /* I2C serial interface clock input */
static unsigned char I2C_SDA = 0x0f;    /* I2C serial interface data I/O */


#define I2C_SW_SCL_PORT  GPIOC
#define I2C_SW_SCL_PIN  GPIO_Pin_1


#define I2C_SW_SDA_PORT  GPIOC
#define I2C_SW_SDA_PIN  GPIO_Pin_0


#define init_i2c_pin(scl, sda) \
	do{ \
		GPIO_Init(I2C_SW_SCL_PORT , I2C_SW_SCL_PIN , GPIO_Mode_Out_PP_Low_Fast);  \
		GPIO_Init(I2C_SW_SDA_PORT , I2C_SW_SDA_PIN , GPIO_Mode_Out_PP_Low_Fast); \
		GPIO_SetBits(I2C_SW_SCL_PORT, I2C_SW_SCL_PIN); \
		GPIO_SetBits(I2C_SW_SDA_PORT, I2C_SW_SDA_PIN); \
	}while(0)

#define release_i2c_pin(scl, sda) \
	do{ \
		GPIO_ResetBits(I2C_SW_SCL_PORT, I2C_SW_SCL_PIN); \
		GPIO_ResetBits(I2C_SW_SDA_PORT, I2C_SW_SDA_PIN); \
		GPIO_Init(I2C_SW_SCL_PORT , I2C_SW_SCL_PIN , GPIO_Mode_Out_PP_Low_Fast);  \
		GPIO_Init(I2C_SW_SDA_PORT , I2C_SW_SDA_PIN , GPIO_Mode_Out_PP_Low_Fast); \
	}while(0)

    
#define set_i2c_pin(pin) \
	do{ \
		if(I2C_SCL == pin)\
			GPIO_SetBits(I2C_SW_SCL_PORT, I2C_SW_SCL_PIN);\
		else\
			GPIO_SetBits(I2C_SW_SDA_PORT, I2C_SW_SDA_PIN);\
	}while(0)

#define clr_i2c_pin(pin) \
	do{ \
		if(I2C_SCL == pin)\
			GPIO_ResetBits(I2C_SW_SCL_PORT, I2C_SW_SCL_PIN);\
		else\
			GPIO_ResetBits(I2C_SW_SDA_PORT, I2C_SW_SDA_PIN);\
	}while(0)
	

bool get_i2c_data(void)
{
	u8  gpio_val;
	u8 data = I2C_SW_SDA_PIN;
	
	gpio_val = GPIO_ReadInputData(I2C_SW_SDA_PORT);
	if(0 ==(gpio_val&data))
		return FALSE;
	else
	   return TRUE;
}


#define get_i2c_pin(pin) \
	get_i2c_data()


#define set_i2c_data_in(pin) \
	GPIO_Init(I2C_SW_SDA_PORT , I2C_SW_SDA_PIN , GPIO_Mode_In_PU_No_IT)

#define set_i2c_data_out(pin) \
	GPIO_Init(I2C_SW_SDA_PORT , I2C_SW_SDA_PIN , GPIO_Mode_Out_PP_Low_Fast)
    

static void i2c_delay(unsigned char time);

static void i2c_begin(void);
static void i2c_end(void);
static void i2c_write_ask(unsigned char flag);
static bool i2c_read_ack(void);
static unsigned char   i2c_read_byte(void);
static bool i2c_write_byte(unsigned char data);


bool i2c_write_bytes(unsigned char *addr, unsigned short addrlen, unsigned char *data, unsigned short size)
{
    unsigned short i;     
      
    // start transmite
    i2c_begin();
    
    // write address to I2C device, first is device address, second is the register address
    for (i=0; i<addrlen; i++)
    {
        if (!i2c_write_byte(addr[i]))
        {
            i2c_end();
            return FALSE;
        }
    }   
    
    // transmite data
    for (i=0; i<size; i++)
    {
        if (!i2c_write_byte(data[i]))
        {
            i2c_end();
            return FALSE;
        }
    }
    
    // stop transmited
    i2c_end();
    
    return TRUE;
}

bool i2c_read_bytes(unsigned char *addr, unsigned short addrlen, unsigned char *data, unsigned short size)
{
    unsigned short i;     
 
    // start transmite
    i2c_begin();
    
    // write address to I2C device, first is device address, second is the register address
    for (i=0; i<addrlen; i++)
    {
        if (!i2c_write_byte(addr[i]))
        {
            i2c_end();
            return FALSE;
        }
    }

    i2c_end();
    
    // restart transmite
    i2c_begin();
    
    // send message to I2C device to transmite data
    if (!i2c_write_byte((unsigned char)(addr[0] | 1)))
    {
        i2c_end();
        return FALSE;
    }
    
    // transmite datas
    for(i=0; i<size; i++)
    {
        data[i] = i2c_read_byte();
        (i<size-1) ? i2c_write_ask(0) : i2c_write_ask(1);
    }
    
    // stop transmite
    i2c_end();
    
    return TRUE;
}


/**
 * @brief receive one byte from I2C interface function
 * receive one byte data from I2C bus
 * @author 
 * @date 2017-09-30
 * @param void
 * @return unsigned char: received the data
 * @retval
 */
static unsigned char i2c_read_byte(void)
{
    unsigned char i;
    unsigned char ret = 0;

    set_i2c_data_in(I2C_SDA);
    
    for (i=0; i<8; i++)
    {           
        i2c_delay( 2);
        set_i2c_pin(I2C_SCL);   
        i2c_delay( 2);
        ret = ret<<1;
        if (get_i2c_pin(I2C_SDA))
            ret |= 1;
        i2c_delay( 2);
        clr_i2c_pin(I2C_SCL);
        i2c_delay( 1);
        if (i==7)
        {
            set_i2c_data_out(I2C_SDA);
        }
        i2c_delay( 1);
    }       

    return ret;
}

/**
 * @brief write one byte to I2C interface function
 * write one byte data to I2C bus
 * @author 
 * @date 2017-09-30
 * @param unsigned char data: send the data
 * @return bool: return write success or failed
 * @retval FALSE: operate failed
 * @retval TRUE: operate success
 */
static bool i2c_write_byte(unsigned char data)
{
    unsigned char i;

    for (i=0; i<8; i++)
    {
        i2c_delay( 2);
        if (data & 0x80)
            set_i2c_pin(I2C_SDA);
        else
            clr_i2c_pin(I2C_SDA);
        data <<= 1;

        i2c_delay( 2);
        set_i2c_pin(I2C_SCL);
        i2c_delay( 3);
        clr_i2c_pin(I2C_SCL);       
    }   
    
    return i2c_read_ack();
}

/**
 * @brief I2C interface initialize function
 * setup I2C interface
 * @author Guanghua Zhang
 * @date 22017-09-30
 * @param void
 * @return void
 * @retval
 */
void i2c_init(unsigned short pin_scl, unsigned short pin_sda)
{
		
    init_i2c_pin(I2C_SCL, I2C_SDA);
	
}

void i2c_release(unsigned short pin_scl, unsigned short pin_sda)
{
    
    release_i2c_pin(pin_scl, pin_sda);   
}

/**
 * @brief delay function
 * delay the time
 * @author Guanghua Zhang
 * @date 2017-09-30
 * @param unsigned short time: delay time
 * @return void
 * @retval
 */
static void i2c_delay(unsigned char time)
{
  unsigned short x , y;
  for(x = time; x > 0; x--)			/*	通过一定周期循环进行延时*/
	for(y = 2 ; y > 0 ; y--);    //1000  100  10  5

}


/**
 * @brief I2C interface start function
 * start I2C transmit
 * @author
 * @date 2017-09-30
 * @param void
 * @return void
 * @retval
 */
static void i2c_begin(void)
{

    i2c_delay( 2);
    set_i2c_pin(I2C_SDA);   
    i2c_delay( 2);
    set_i2c_pin(I2C_SCL);
    i2c_delay( 3);
    clr_i2c_pin(I2C_SDA);   
    i2c_delay( 3);
    clr_i2c_pin(I2C_SCL);
    i2c_delay( 4);
}

/**
 * @brief I2C interface stop function
 * stop I2C transmit
 * @author 
 * @date 2017-09-30
 * @param void
 * @return void
 * @retval
 */
static void i2c_end(void)
{
    i2c_delay( 2);
    clr_i2c_pin(I2C_SDA);
    i2c_delay( 2);
    set_i2c_pin(I2C_SCL);
    i2c_delay( 3);
    set_i2c_pin(I2C_SDA);
    i2c_delay( 4);
	
}

/**
 * @brief I2C interface send asknowlege function
 * send a asknowlege to I2C bus
 * @author 
 * @date 2017-09-30
 * @param unsigned char
 *   0:send bit 0
 *   not 0:send bit 1
 * @return void
 * @retval
 */
static void i2c_write_ask(unsigned char flag)
{
    if(flag)
        set_i2c_pin(I2C_SDA);
    else
        clr_i2c_pin(I2C_SDA);
    i2c_delay( 2);
    set_i2c_pin(I2C_SCL);
    i2c_delay( 3);
    clr_i2c_pin(I2C_SCL);
    i2c_delay( 2);
    set_i2c_pin(I2C_SDA);
    i2c_delay( 2);
}

/**
 * @brief I2C receive anknowlege
 * receive anknowlege from i2c bus
 * @author 
 * @date 2017-09-30
 * @param void
 * @return bool: return received anknowlege bit
 * @retval FALSE: 0
 * @retval TRUE: 1
 */
static bool i2c_read_ack(void)
{   
    bool ret;
    set_i2c_data_in(I2C_SDA);
    i2c_delay( 3);
    set_i2c_pin(I2C_SCL);
    i2c_delay( 2);
    if (!get_i2c_pin(I2C_SDA))
    {
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }

    i2c_delay( 2);
    clr_i2c_pin(I2C_SCL);
    i2c_delay( 2);

    set_i2c_data_out(I2C_SDA);
    
    i2c_delay( 2); 
    return ret;
}

bool i2c_write_data(unsigned char daddr, unsigned char raddr, unsigned char *data, unsigned short size)
{
    unsigned char addr[2];
    
    addr[0] = daddr;
    addr[1] = raddr;
    
    return i2c_write_bytes(addr, 2, data, size);
}

bool i2c_write_data2(unsigned char daddr, unsigned short raddr, unsigned char *data, unsigned short size)
{
    unsigned char addr[3];

    addr[0] = daddr;
    addr[1] = (unsigned char)(raddr >> 8);   //hight 8bit
    addr[2] = (unsigned char)(raddr);	      //low 8bit

    return i2c_write_bytes(addr, 3, data, size);
}

bool i2c_write_data3(unsigned char daddr, unsigned short raddr, unsigned short *data, unsigned short size)
{
    unsigned short i;
    unsigned char high_8bit,low_8bit;

    high_8bit = (unsigned char)(raddr >> 8);   //hight 8bit
    low_8bit = (unsigned char)(raddr);            //low 8bit

    i2c_begin();
    if (!i2c_write_byte(daddr))
    {
        i2c_end();
        return FALSE;
    }
    if (!i2c_write_byte(high_8bit))
    {
        i2c_end();
        return FALSE;
    }
    if (!i2c_write_byte(low_8bit))
    {
        i2c_end();
        return FALSE;
    }

    for(i=0; i<size; i++)
    {
        low_8bit = (unsigned char)(*data);
        high_8bit = (unsigned char)((*data) >> 8);

        if (!i2c_write_byte(high_8bit))
        {
            i2c_end();
            return FALSE;
        }
        if (!i2c_write_byte(low_8bit ))
        {
            i2c_end();
            return FALSE;
        }		
        data++;
    }
    i2c_end();

    return TRUE;
}

bool i2c_write_data4(unsigned char daddr, unsigned char *data, unsigned short size)
{ 
    return i2c_write_bytes(&daddr, 1, data, size);
}

bool i2c_read_data(unsigned char daddr, unsigned char raddr, unsigned char *data, unsigned short size)
{
    unsigned char addr[2];
    
    addr[0] = daddr;
    addr[1] = raddr;
    
    return i2c_read_bytes(addr, 2, data, size);
}

bool i2c_read_data2(unsigned char daddr, unsigned short raddr, unsigned char *data, unsigned short size)
{
    unsigned char addr[3];

    addr[0] = daddr;
    addr[1] = (unsigned char)(raddr >> 8);   //hight 8bit
    addr[2] = (unsigned char)(raddr);	    //low 8bit

    return i2c_read_bytes(addr, 3, data, size);
}

bool i2c_read_data3(unsigned char daddr, unsigned short raddr, unsigned short *data, unsigned short size)
{
    unsigned short i;
    unsigned char high_8bit,low_8bit;

    high_8bit = (unsigned char)(raddr >> 8);   //hight 8bit
    low_8bit = (unsigned char)(raddr);            //low 8bit

    i2c_begin();
    if (!i2c_write_byte(daddr))
    {
        i2c_end();
        return FALSE;
    }
    if (!i2c_write_byte(high_8bit))
    {
        i2c_end();
        return FALSE;
    }
    if (!i2c_write_byte(low_8bit))
    {
        i2c_end();
        return FALSE;
    }

    i2c_begin();

    if (!i2c_write_byte((unsigned char)(daddr | 1)))
    {
        i2c_end();
        return FALSE;
    }
    for(i=0; i<size; i++)
    {
        high_8bit = i2c_read_byte();
        i2c_write_ask(0);		
        low_8bit = i2c_read_byte();		
        (i<size-1)?i2c_write_ask(0):i2c_write_ask(1);

        *data = (unsigned short)(high_8bit) << 8 | low_8bit;
        data++;
    }

    i2c_end();

    return TRUE;
}

bool i2c_read_data4(unsigned char daddr, unsigned char *data, unsigned short size)
{  
    return i2c_read_bytes(&daddr, 1, data, size);
}


/* end of file */
