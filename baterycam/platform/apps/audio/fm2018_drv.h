/**
  ******************************************************************************
  * File Name          : fm2018_drv.h
  * Description        : This file contains all config fm2018 chip
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FM2018_DRV_H__
#define __FM2018_DRV_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include "ak_apps_config.h"

#include "ak_drv_i2c.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

#define FM2018_380_DEVICE_ADDRESS             0xC0
#define FM2018_SYNC_WORD                      0xFCF3

#define FM2018_CMD_WRITE                      0x3B
#define FM2018_CMD_READ_MEM                   0x37
#define FM2018_CMD_READ_REG                   0x60
#define FM2018_DATA_PORT_LOW                  0x25
#define FM2018_DATA_PORT_HIGH                 0x26

#define FM2018_START_ADDRESS                  0x1E3E
#define FM2018_START_DATA                     0x0280    // speaker volume 2.5x gain
#define FM2018_END_ADDRESS                    0x1E3A
#define FM2018_END_DATA                       0x0000 

#define FM2018_MAX_CONFIG                     (44+2)//(43+2)
#define FM2018_SIZE_INSTRUCTION                 (7) // bytes

#define FM_ADDR_01       0x1E30
#define FM_ADDR_02       0x1E34
#define FM_ADDR_03       0x1E3D
#define FM_ADDR_04       0x1E41
#define FM_ADDR_05       0x1E44
#define FM_ADDR_06       0x1E45
#define FM_ADDR_07       0x1E46
#define FM_ADDR_08       0x1E47
#define FM_ADDR_09       0x1E48
#define FM_ADDR_10       0x1E49
#define FM_ADDR_11       0x1E4D
#define FM_ADDR_12       0x1E52
#define FM_ADDR_13       0x1E63
#define FM_ADDR_14       0x1E70
#define FM_ADDR_15       0x1E86
#define FM_ADDR_16       0x1E87
#define FM_ADDR_17       0x1E88
#define FM_ADDR_18       0x1E89
#define FM_ADDR_19       0x1E8B
#define FM_ADDR_20       0x1E8C
#define FM_ADDR_21       0x1E92
#define FM_ADDR_22       0x1E93
#define FM_ADDR_23       0x1EA0
#define FM_ADDR_24       0x1EA1
#define FM_ADDR_25       0x1EBC
#define FM_ADDR_26       0x1EBD
#define FM_ADDR_27       0x1EBF
#define FM_ADDR_28       0x1EC0
#define FM_ADDR_29       0x1EC1
#define FM_ADDR_30       0x1EC5
#define FM_ADDR_31       0x1EC6
#define FM_ADDR_32       0x1EC8
#define FM_ADDR_33       0x1EC9
#define FM_ADDR_34       0x1ECA
#define FM_ADDR_35       0x1ECB
#define FM_ADDR_36       0x1ECC
#define FM_ADDR_37       0x1EF8
#define FM_ADDR_38       0x1EF9
#define FM_ADDR_39       0x1EFF
#define FM_ADDR_40       0x1F00
#define FM_ADDR_41       0x1F0A
#define FM_ADDR_42       0x1F0C
#define FM_ADDR_43       0x1F0D
#define FM_ADDR_44       0x1E36         // Trung add more for line out PGA 2dB
// #define FM_ADDR_45       0x1E57         // mic_sat_th (Microphone Saturate threshold)
// #define FM_ADDR_46       0x1E5C         // Address: 0x1E5C (default value 0x1000)

#define FM_DATA_01       0x0231
#define FM_DATA_02       0x000A //0x000F  // Trung modify for + s26dB
#define FM_DATA_03       0x0100 //0x0280  // 0x0100 2.5x gain
#define FM_DATA_04       0x0101
#define FM_DATA_05       0x0081
#define FM_DATA_06       0x03CF // 0x83CF
#define FM_DATA_07       0x0010
#define FM_DATA_08       0x2800
#define FM_DATA_09       0x1000
#define FM_DATA_10       0x0880
#define FM_DATA_11       0x0100
#define FM_DATA_12       0x0013
#define FM_DATA_13       0x0003
#define FM_DATA_14       0x05C0
#define FM_DATA_15       0x0009
#define FM_DATA_16       0x0003
#define FM_DATA_17       0x4800
#define FM_DATA_18       0x0001
#define FM_DATA_19       0x0180
#define FM_DATA_20       0x0040
#define FM_DATA_21       0x1000
#define FM_DATA_22       0x0400
#define FM_DATA_23       0x0400
#define FM_DATA_24       0x2000
#define FM_DATA_25       0x6800
#define FM_DATA_26       0x0100
#define FM_DATA_27       0x7000
#define FM_DATA_28       0x2680
#define FM_DATA_29       0x1080
#define FM_DATA_30       0x2B06
#define FM_DATA_31       0x0C1F
#define FM_DATA_32       0x2879
#define FM_DATA_33       0x65AB
#define FM_DATA_34       0x4026
#define FM_DATA_35       0x7FFF
#define FM_DATA_36       0x7FFE
#define FM_DATA_37       0x0780 //0x1400 Eric change config
#define FM_DATA_38       0x0400
#define FM_DATA_39       0x5800
#define FM_DATA_40       0x7FFF
#define FM_DATA_41       0x0A00
#define FM_DATA_42       0x0100
#define FM_DATA_43       0x7FFF
#define FM_DATA_44       0x001F         // Trung add more for line out PGA 2dB
// #define FM_DATA_45       0x7E00         // Address: 0x1E57 (default value 0x7E00)
// #define FM_DATA_46       0x2000
/* End of config FM2018-380 */

typedef struct fm_param_cfg{
    unsigned short addr;     // 2 bytes address
    unsigned short data;        // 2 bytes data write
}FM_PARAM_CFG_t;

/* Parameter config FM2018-380 */
typedef struct fm_instruction{
    unsigned char sync[2];   // 2 byte 0xFCF3
    unsigned char cmd_entry;   // 1byte
    unsigned char addr[2];     // 2 bytes address
    unsigned char data[2];        // 2 bytes data write
}FM_INSTRUCTION_t __attribute__((aligned(8)));

typedef struct fm_transaction_write{
    union{
        unsigned char byte_wr[7]; // sync[0xFCF3] cmd[0x3B] addr[2byte] data[2byte]
        FM_INSTRUCTION_t inst;
    }cmd;
}FM_TRANSACTION_WRITE_t __attribute__((aligned(8)));

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
// structure for process data, don't use it to send I2C
typedef struct inst{
    unsigned short sync;
    unsigned char cmd;
    unsigned short addr;
    unsigned short data;
}PROC_INST_t;
// Structure for send data I2C
// This structure contain 7 bytes
typedef struct fm_dt_inst{
    unsigned char sync[2];
    unsigned char cmd;
    unsigned char addr[2];
    unsigned char data[2];
}TRAN_INST_t;
 
typedef struct fm_rd{
    unsigned char cmd;
    unsigned char ind[2]; // indicator is address register or data port
}TRAN_READ_t;
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/

// int fm2018_init(void **ppHandle);
// int fm2018_deinit(void **ppHandle);
// int fm2018_config_data(void);
/*
* Config FM2018 for Cinatic
*/
int fm2018_config_param(void);
int fm2018_ftest_config_param(void);

int fm2018_analog_communication_mode(void);
int fm2018_dsp_mode(void);
void fm2018_drv_set_pin_irq_analog(int logic);


int fm2018_power_down_init_gpio(void);
int fm2018_pwd_active_high(void);
int fm2018_pwd_active_low(void);

int fm2018_pwd_active(void);
int fm2018_pwd_deactive(void);


#ifdef __cplusplus
}
#endif

#endif /* __FM2018_DRV_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

