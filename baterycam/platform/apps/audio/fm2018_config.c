/**
  ******************************************************************************
  * File Name          : fm2018_config.c
  * Description        : This file contains API control FM2018
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "fm2018_config.h"
#include "ak_apps_config.h"
#include "drv_gpio.h"
#include "ak_ini.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
static FMCFG_t fm2018_cfg[] = 
    {
        {"1E3E", ""},
        {"1E4A", ""},
        {"1E4C", ""},
        {"1E30", ""},
        {"1E34", ""},
        {"1E36", ""},
        {"1E39", ""},
        {"1E4A", ""},
        {"1E4C", ""},
        {"1E3D", ""},
        {"1E40", ""},
        {"1E41", ""},
        {"1E44", ""},
        {"1E45", ""},
        {"1E46", ""},
        {"1E47", ""},
        {"1E48", ""},
        {"1E49", ""},
        {"1E4D", ""},
        {"1E57", ""},
        {"1E5C", ""},
        {"1E4F", ""},
        {"1E51", ""},
        {"1E52", ""},
        {"1E63", ""},
        {"1E70", ""},
        {"1E86", ""},
        {"1E87", ""},
        {"1E88", ""},
        {"1E89", ""},
        {"1E8B", ""},
        {"1E8C", ""},
        {"1E92", ""},
        {"1E93", ""},
        {"1EA0", ""},
        {"1EA1", ""},
        {"1EA7", ""},
        {"1EA8", ""},
        {"1EAB", ""},
        {"1EB3", ""},
        {"1EB4", ""},
        {"1EB5", ""},
        {"1EBB", ""},
        {"1EBC", ""},
        {"1EBD", ""},
        {"1EBF", ""},
        {"1EC0", ""},
        {"1EC1", ""},
        {"1EC5", ""},
        {"1EC6", ""},
        {"1EC8", ""},
        {"1EC9", ""},
        {"1ECA", ""},
        {"1ECB", ""},
        {"1ECC", ""},
        {"1EF8", ""},
        {"1EF9", ""},
        {"1EFF", ""},
        {"1F00", ""},
        {"1F0A", ""},
        {"1F0C", ""},
        {"1F0D", ""},
        {"1F28", ""},
        {"1F29", ""},
        {"1F2A", ""},
        {"1F2B", ""},
        {"1F30", ""},
        {"1E3A", ""},
    };

void *g_fmcfg_handle;

/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/
static char char_to_hex(char c)
{
    char ret_char = 0;
    if(c >= 0x30 && c <= 0x39)
    {
        ret_char = c - 0x30;
    }
    else if(c >= 'a' && c <= 'f')
    {
        ret_char = c - 0x57;
    }
    else if(c >= 'A' && c <= 'F')
    {
        ret_char = c - 0x37;
    }
    else
    {
        ret_char = 0xFF;
    }
    return ret_char;
}

/*
*   Change string to unsigned short
*   length of string is 4 characters
*   ex: 'ABCD' to '0xABCD'
*/
static unsigned short value_str_convert_to_unsignedshort(char *str)
{
    unsigned short ret = 0;
    char c = 0;
    char cTemp = 0;
    int i = 0;
    if(str != NULL)
    {
        for(i = 0; i < sizeof(unsigned short)*2; i++)
        {
            c = *(str + i);
            cTemp = char_to_hex(c);
            ret |= (unsigned short)((cTemp&0x0F) << (16-(i+1)*4));
            // printf("[%d] c = %c cTemp=%x, ret = %X\n", i, c, cTemp, ret);
        }
    }
    return ret;
}
/* 1. show all register and value */
int fmcfg_get_ini_cfg(void)
{
    int iRet = 0;
    int iSizeFmCfg = 0;
    int i = 0;
    unsigned short usValue = 0;
    unsigned short usReg = 0;

    g_fmcfg_handle = ak_ini_init("APPINI");
    if(g_fmcfg_handle == NULL)
    {
        ak_print_error_ex("Open INI failed!\r\n");
        iRet = -1;
    }
    else
    {
        iSizeFmCfg = sizeof(fm2018_cfg) / sizeof(FMCFG_t);
        // ak_print_normal("iSizeFmCfg: %d\r\n", iSizeFmCfg);
        for(i = 0; i < iSizeFmCfg; i++)
        {
            iRet = ak_ini_get_item_value(g_fmcfg_handle, FMCFG_TITLE_NAME, \
                    fm2018_cfg[i].key, fm2018_cfg[i].value);
            if(iRet != 0)
            {
                ak_print_error_ex("[%02d]Get %s failed!\n", i, fm2018_cfg[i].key);
                break;
            }
            else
            {
                usValue = value_str_convert_to_unsignedshort(fm2018_cfg[i].value);
                // ak_print_normal_ex("[%02d]Get %s: %s (0x%04X) OK\n", i, \
                //     fm2018_cfg[i].key, fm2018_cfg[i].value, usValue);
                usReg = value_str_convert_to_unsignedshort(fm2018_cfg[i].key);
                fm2018_cfg[i].fm.usReg = usReg;
                fm2018_cfg[i].fm.usValue = usValue;
            }
        }
        ak_ini_destroy(g_fmcfg_handle);
        g_fmcfg_handle = NULL;
    }
    return iRet;
}

void fmcfg_show_config(void)
{
    int i = 0;
    int iSizeFmCfg = 0;
    
    iSizeFmCfg = sizeof(fm2018_cfg) / sizeof(FMCFG_t);
    ak_print_normal("iSizeFmCfg: %d\r\n", iSizeFmCfg);
    for(i = 0; i < iSizeFmCfg; i++)
    {
        ak_print_normal_ex("[%02d]Get %s:%s (0x%04X-0x%04X)\n", i, \
            fm2018_cfg[i].key, fm2018_cfg[i].value, \
            fm2018_cfg[i].fm.usReg, fm2018_cfg[i].fm.usValue);
    }
}

/* 2. get/set value for register */


int fmcfg_send_cfg_to_chip(void)
{
    int ret = 0;
    void *handle = NULL;
    unsigned short u16ReadValue = 0;
    unsigned short u16Addr = 0;
    unsigned short u16Data = 0;
    int i = 0;
    // open device
    ret = fm2018_init(&handle);
    if(NULL == handle){
        ak_print_error_ex("Cannot open I2C (0xC0)!\n");
        return -1;
    }
    // loop, read config and write
    for(i = 0; i < sizeof(fm2018_cfg) / sizeof(FMCFG_t); i++)
    {
        u16Addr = fm2018_cfg[i].fm.usReg;
        u16Data = fm2018_cfg[i].fm.usValue;
        // write config
        ret = fm2018_write_reg(handle, u16Addr, u16Data);
        if(ret != 0)
        {
            ak_print_error("Cannot write FM2018 [%02d] Addr: %04X (ret %d)\n", i, u16Addr, ret);
            break;
        }
        else
        {
            ret = fm2018_read_reg(handle, u16Addr, &u16ReadValue);
            //ak_print_normal("[%02d] Addr: 0x%04X-0x%04X (Read: 0x%04X) (ret %d)\n", \
            //                    i, u16Addr, u16Data, u16ReadValue, ret);
        }
        ak_sleep_ms(10);
    }
    ak_print_normal("Config FM2018 done!\n");
    // close device
    ret = fm2018_deinit(&handle);
    return ret;
}

/* check register in list */
static int fmcfg_check_register_in_config(char *pReg)
{
    int iRet = 0, i = 0;
    int iSizeFmCfg = sizeof(fm2018_cfg) / sizeof(FMCFG_t);
    if(pReg){
        for(i = 0; i < iSizeFmCfg; i++){
            if(strncmp(fm2018_cfg[i].key, pReg, 4) == 0){
                iRet = 1;
                break;
            }
        }
    }
    return iRet;
}

/* get value of register */
int fmcfg_get_reg(char *pReg, unsigned short *pValue)
{
    int iRet = 0;
    char strValue[FMCFG_VALUE_BUF_SIZE] = {0};

    if(pReg == NULL || pValue == NULL)
    {
        return -1;
    }

    if(fmcfg_check_register_in_config(pReg) == 0)
    {
        ak_print_normal_ex("Don't find register %s in config!\r\n", pReg);
        return -1;
    }

    g_fmcfg_handle = ak_ini_init("APPINI");
    if(g_fmcfg_handle == NULL)
    {
        ak_print_error_ex("Open INI failed!\r\n");
        iRet = -2;
    }
    else
    {
        if(ak_ini_get_item_value(g_fmcfg_handle, \
            FMCFG_TITLE_NAME, pReg, strValue) == 0){
            printf("reg:%s value:%s\r\n", pReg, strValue);
            *pValue = value_str_convert_to_unsignedshort(strValue);
        }
        else
        {
            ak_print_error("CFG: read %s config failed!\n", pReg);
            iRet = -3;
        }
        ak_ini_destroy(g_fmcfg_handle);
        g_fmcfg_handle = NULL;
    }
    return iRet;
}
/* Set value register */
int fmcfg_set_reg(char *pReg, char *pValue)
{
    int iRet = 0;
    char strValue[FMCFG_VALUE_BUF_SIZE] = {0};

    if(pReg == NULL || pValue == NULL)
    {
        return -1;
    }
    if(fmcfg_check_register_in_config(pReg) == 0)
    {
        ak_print_normal_ex("Don't find register %s in config!\r\n", pReg);
        return -1;
    }
    
    g_fmcfg_handle = ak_ini_init("APPINI");
    if(g_fmcfg_handle == NULL)
    {
        ak_print_error_ex("Open INI failed!\r\n");
        iRet = -2;
    }
    else
    {
        // sprintf(strValue, "%04X", Value);
        printf("Value: %s\r\n", pValue);
        if(ak_ini_set_item_value(g_fmcfg_handle, \
            FMCFG_TITLE_NAME, pReg, pValue) == 0){
            ak_print_normal_ex("Set OK!\n");
        }
        else
        {
            ak_print_error("CFG: read %s config failed!\n", pReg);
            iRet = -3;
        }
        ak_ini_destroy(g_fmcfg_handle);
        g_fmcfg_handle = NULL;
    }
    return iRet;
}



/* 3. power down and power up fm chip */
/* 4. change on flight some parameter for FM */

int fmcfg_cfg_fm2018(void)
{
    int iRet = 0;
    printf("================================\r\n");
    printf("%s entered!\r\n", __func__);
    iRet = fmcfg_get_ini_cfg();
    if(iRet != 0)
    {
        ak_print_error_ex("Read config from flash failed!\r\n");
    }
    else
    {
    	fm2018_dsp_mode();
        fm2018_shi_init_gpio();
        ak_print_normal("SHI init gpio\r\n");
        fm2018_power_down_init_gpio();
        // fm2018_pwd_active_high();
        fm2018_pwd_active(); // use this API will not depend logic (macro logic define in ak_apps_config.h)
        ak_print_error("---------------\r\nSet pin FM PWD active high!\r\n");
        // fmcfg_show_config();
        iRet = fmcfg_send_cfg_to_chip();
        ak_print_normal_ex("send cfg to chip-ret: %d\r\n", iRet);
    }
    return iRet;
}

void fmcfg_atecmd_test(void)
{
    int iRet = 0;
    iRet = fmcfg_get_ini_cfg();
    if(iRet != 0)
    {
        ak_print_error_ex("Read config from flash failed!\r\n");
    }
    else
    {
        fmcfg_show_config();
        iRet = fmcfg_send_cfg_to_chip();
        ak_print_normal_ex("send cfg to chip-ret: %d\r\n", iRet);
    }
}

/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

