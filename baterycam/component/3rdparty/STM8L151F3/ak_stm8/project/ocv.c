#include "ocv.h"


#include "stm8l15x_iwdg.h"

#define DEV_ADDR  0x68//0x34

#define I2C_SPEED  100000

#define I2C_SOFTWARE   1

#if I2C_SOFTWARE
#include "i2c_software.h"
#else
#include"i2c.h"
#endif

//
// T =
void IWDG_Start(void)
{
	#if 0
	IWDG_SetReload(0xff);      //重载寄存器写入255
	//先写键值 ，先写0XCC , 后写0X55
	IWDG_Enable();  //CC

	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //55

	IWDG_SetPrescaler(IWDG_Prescaler_64);   //38KHZ  / 16  = 107.79ms
	#endif
}


void IWDG_Stop(void)
{
	#if 0
	//先写键值 ，先写0XCC , 后写0X00
	IWDG_Enable();  //CC
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);  //00
	#endif
}


//typedef unsigned short	uint16_t;
//typedef unsigned char	uint8_t;

#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

#define ABS(x)				((x) >0 ? (x) : -(x) )

 /*初始化开路电压对应百分比表*/

#define OCVREG0				0x00		//3.1328		//0%
#define OCVREG1				0x00		//3.2736		//1%
#define OCVREG2				0x00		//3.4144		//2%
#define OCVREG3				0x00		//3.5552		//5%
#define OCVREG4				0x05		//3.6256		//5%
#define OCVREG5				0x0C		//3.6608		//12%
#define OCVREG6				0x14		//3.6960		//20%
#define OCVREG7				0x22		//3.7312		//34%
#define OCVREG8				0x2C		//3.7664		//44%
#define OCVREG9				0x35		//3.8016		//53%
#define OCVREGA				0x3C		//3.8368		//60%
#define OCVREGB				0x41		//3.8720		//65%
#define OCVREGC				0x4B		//3.9424		//75%
#define OCVREGD				0x52		//4.0128		//82%
#define OCVREGE				0x5C		//4.0832		//92%
#define OCVREGF				0x64		//4.1536		//100%

/* 初始化开路电压*/

#define OCVVOL0				3132
#define OCVVOL1				3273
#define OCVVOL2				3414
#define OCVVOL3				3555
#define OCVVOL4				3625
#define OCVVOL5				3660
#define OCVVOL6				3696
#define OCVVOL7				3731
#define OCVVOL8				3766
#define OCVVOL9				3801
#define OCVVOLA				3836
#define OCVVOLB				3872
#define OCVVOLC				3942
#define OCVVOLD				4012
#define OCVVOLE				4083
#define OCVVOLF				4153


void I2C_dev_open()
{	
	#if I2C_SOFTWARE
	i2c_init(0,0);
	#else
	//unsigned int x , y;
	IIC_Init(I2C_SPEED,DEV_ADDR);

	IWDG_Start();
		#endif

}

void I2C_dev_close()
{
	#if I2C_SOFTWARE
	i2c_release(0,0);
	#else
	IWDG_Stop();
	
	IIC_Close();	
        #endif
}

//写函数
void axp_write(int reg, uint8_t val)
{
	 uint8_t data;
	 data = val;
 	#if I2C_SOFTWARE
	i2c_write_data(DEV_ADDR,reg,&data,1);
	#else
	 IIC_Write(DEV_ADDR,reg,&data,1);
	#endif
}

//读函数
void  axp_read( int reg, uint8_t *val)
{
	 //
	 // Read the specified length of data
	 //
  	#if I2C_SOFTWARE
	i2c_read_data(DEV_ADDR, reg, val, 1);
	#else

	 IIC_Read(DEV_ADDR, reg, val, 1);
	#endif
}
 

 
//连写
#if 0
void axp_writes( int reg, int len, uint8_t *val)
{
	 uint8_t t;

	 for(t=0;t<len;t++)
	 {
		axp_write(reg+t,*(val+t));
	 }	

	 
}

#endif 	

//连读

void axp_reads( int reg, int len, uint8_t *val)
{
 	#if I2C_SOFTWARE
	 uint8_t t;

	 for(t=0;t<len;t++)
	 {
	   	axp_read( reg+t, (val+t));
	 }
	#else		
	 IIC_Read(DEV_ADDR, reg, val, len);
	#endif
 
}

//对位置1
void axp_set_bits( int reg,uint8_t bit_mask)
{

	uint8_t reg_val;

	axp_read(reg, &reg_val);

	if ((reg_val & bit_mask) != bit_mask) 
	{
		reg_val |= bit_mask;
		axp_write( reg, reg_val);
	}

}
 
//对位置0
void axp_clr_bits(int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	axp_read(reg, &reg_val);

	if (reg_val & bit_mask) 
	{
		reg_val &= ~bit_mask;
		axp_write( reg, reg_val);
	}
}

//对多位值置值
void axp_update( int reg, uint8_t val, uint8_t mask)
{
	uint8_t reg_val;

	axp_read(reg, &reg_val);

	if ((reg_val & mask) != val) 
	{
		reg_val = (reg_val & ~mask) | val;
		axp_write( reg, reg_val);
	}
}


 static void axp_vbat_to_mV(int *bat,uint16_t reg)
{
	
	//*bat = (reg) * 1100 / 1000;
	
	*bat = (reg) * 11 / 10;
}

static void axp_ibat_to_mA(int *elect,uint16_t reg)
{
 	*elect = (reg) * 500 / 1000;
}

 static void axp_bat_vol(uint8_t dir,int vol,int cur,int rdc,int *ovc)
{
	if(dir)
		*ovc = (vol-cur*rdc/1000);
	else
		*ovc = (vol+cur*rdc/1000);
}


static void axp_vol2cap(int ocv, unsigned char *value)
{
	unsigned char *valu = value;
    if(ocv >= OCVVOLF)
    {
    	*valu = OCVREGF;
    }
    else if(ocv < OCVVOL0)
    {
    	*valu = OCVREG0;
    }
    else if(ocv < OCVVOL1)
    {
    	*valu = OCVREG0 + (OCVREG1 - OCVREG0) * (ocv - OCVVOL0) / (OCVVOL1 - OCVVOL0);
    }
    else if(ocv < OCVVOL2)
    {
    	*valu = OCVREG1 + (OCVREG2 - OCVREG1) * (ocv - OCVVOL1) / (OCVVOL2 - OCVVOL1);
    }
    else if(ocv < OCVVOL3)
    {
    	*valu = OCVREG2 + (OCVREG3 - OCVREG2) * (ocv - OCVVOL2) / (OCVVOL3 - OCVVOL2);
    }
    else if(ocv < OCVVOL4)
    {
    	*valu = OCVREG3 + (OCVREG4 - OCVREG3) * (ocv - OCVVOL3) / (OCVVOL4 - OCVVOL3);
    }
    else if(ocv < OCVVOL5)
    {
    	*valu = OCVREG4 + (OCVREG5 - OCVREG4) * (ocv - OCVVOL4) / (OCVVOL5 - OCVVOL4);
    }
    else if(ocv < OCVVOL6)                               
    {
    	*valu = OCVREG5 + (OCVREG6 - OCVREG5) * (ocv - OCVVOL5) / (OCVVOL6 - OCVVOL5);
    }
    else if(ocv < OCVVOL7)
    {
    	*valu = OCVREG6 + (OCVREG7 - OCVREG6) * (ocv - OCVVOL6) / (OCVVOL7 - OCVVOL6);
    }
    else if(ocv < OCVVOL8)
    {
    	*valu = OCVREG7 + (OCVREG8 - OCVREG7) * (ocv - OCVVOL7) / (OCVVOL8 - OCVVOL7);
    }
    else if(ocv < OCVVOL9)
    {
    	*valu = OCVREG8 + (OCVREG9 - OCVREG8) * (ocv - OCVVOL8) / (OCVVOL9 - OCVVOL8);
    }
    else if(ocv < OCVVOLA)
    {
    	*valu = OCVREG9 + (OCVREGA - OCVREG9) * (ocv - OCVVOL9) / (OCVVOLA - OCVVOL9);
    }
    else if(ocv < OCVVOLB)
    {
    	*valu = OCVREGA + (OCVREGB - OCVREGA) * (ocv - OCVVOLA) / (OCVVOLB - OCVVOLA);
    }
    else if(ocv < OCVVOLC)
    {
    	*valu = OCVREGB + (OCVREGC - OCVREGB) * (ocv - OCVVOLB) / (OCVVOLC - OCVVOLB);
    }
    else if(ocv < OCVVOLD)
    {
    	*valu = OCVREGC + (OCVREGD - OCVREGC) * (ocv - OCVVOLC) / (OCVVOLD - OCVVOLC);
    }
    else if(ocv < OCVVOLE)
    {
    	*valu = OCVREGD + (OCVREGE - OCVREGD) * (ocv - OCVVOLD) / (OCVVOLE - OCVVOLD);
    }
    else if(ocv < OCVVOLF)
    {
    	*valu = OCVREGE + (OCVREGF - OCVREGE) * (ocv - OCVVOLE) / (OCVVOLF - OCVVOLE);
    }
    else
    {
    	*valu = 0;
    }
}


//电量计量函数，返回值为电量
void axp_ocv_restcap(unsigned char *value)
{
	int ocv, vbat, ibat, elect1, elect2;
	unsigned char val[2] , bat_current_direction; 
	unsigned short tmp[2];
	unsigned char * valu = value;
	axp_read(0x00,&val[0]);   //	读00寄存器
	
	bat_current_direction=	val[0] &&(1<<2) ;//取00寄存器的bit2

	axp_reads(0x78,2,val);	 //	读78,79寄存器 ,计算电池电压
	
	tmp[0]=((uint16_t)val[0] << 4 )| (val[1] & 0x0f);	  //adc->vbat_res
	//vbat=  axp_vbat_to_mV(tmp[0]);
	
	axp_vbat_to_mV(&vbat,tmp[0]);

	axp_reads(0x7A,2,val);//读7A 7B 寄存器计算充电电流
	
	tmp[0]=((uint16_t) val[0] << 5 )| (val[1] & 0x1f);//adc->ichar_res
	
	axp_reads(0x7C,2,val);	//读7C 7D 寄存器计算放电电流
	
	tmp[1]=((uint16_t) val[0] << 5 )| (val[1] & 0x1f);//adc->idischar_res
	axp_ibat_to_mA(&elect1,tmp[0]);
	axp_ibat_to_mA(&elect2,tmp[1]);
	if((elect1) - (elect2))
	{
		ibat = (elect1) - (elect2);
	}
	else
	{
		ibat = (elect2) - (elect1);
	}
	//ibat= ABS(axp_ibat_to_mA(tmp[0])-axp_ibat_to_mA(tmp[1]));

	//ocv = axp_bat_vol(bat_current_direction,vbat,ibat,107);
	
	axp_bat_vol(bat_current_direction,vbat,ibat,107,&ocv);
	
	axp_vol2cap(ocv, valu);
}

//控制pmu关机
void axp_power_off()
{
	axp_write(0x32,0xC6);
}

//将DCDC2的输出电压设置成1.15V
void axp_set_DCDC_voltage()
{
	axp_write(0x23,0x12);
}


//设置启动时间为20ms
void axp_set_startime()
{
	axp_write(0x8F,0x81);
}


