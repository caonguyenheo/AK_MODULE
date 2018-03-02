/**
  ******************************************************************************
  * File Name          : fm2018_config.h
  * Description        : This file contains all config fm2018 chip
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FM2018_CONFIG_H__
#define __FM2018_CONFIG_H__
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
#define FMCFG_VALUE_BUF_SIZE               (32)
#define FMCFG_TITLE_NAME                "fm2018"

typedef struct fmhex{
    unsigned short  usReg;
    unsigned short  usValue;
}FMHEX_t;


typedef struct fmcfg{
    char key[FMCFG_VALUE_BUF_SIZE];
    char value[FMCFG_VALUE_BUF_SIZE];
    FMHEX_t fm;
}FMCFG_t;


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
int fmcfg_get_ini_cfg(void);
void fmcfg_show_config(void);
int fmcfg_send_cfg_to_chip(void);
int fmcfg_cfg_fm2018(void);
void fmcfg_atecmd_test(void);

int fmcfg_get_reg(char *pReg, unsigned short *pValue);
int fmcfg_set_reg(char *pReg, char *pValue);

#ifdef __cplusplus
}
#endif

#endif /* __FM2018_CONFIG_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

