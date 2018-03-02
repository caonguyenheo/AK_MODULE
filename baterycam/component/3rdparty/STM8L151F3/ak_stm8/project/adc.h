#ifndef ___ADC_H___
#define ___ADC_H___

#include"stm8l15x.h"

void ADC_Block_Init(void);

void ADC_Block_Release(void);

void Read_ADC_Data(u16 *ADC_Data_);

#endif
