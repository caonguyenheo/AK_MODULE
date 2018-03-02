/**
  ******************************************************************************
  * File Name          : doorbell_config.c
  * Description        : This file will read/write config to flash
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/*---------------------------------------------------------------------------*/
/*                           INCLUDED FILES                                  */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ak_apps_config.h"
#include "ringbuffer.h"
#include "spi_transfer.h"
#include "list.h"
#include "spi_cmd.h"

#include "ak_ini.h"
#include "doorbell_config.h"

#include <stdint.h>


/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/



// static SYS_CFG_t   _system_cfg = {NULL, NULL};
static  CFG_DOORBELL_t tableCfg[SYS_MAX_KEY] = 
{
    {SYS_FLICKER, },    // 0
    {SYS_FLIPUD, },     // 1
    {SYS_FLIPTB, },     // 2
    {SYS_BITRATE, },    // 3
    {SYS_FRAMERATE, },  // 4
    {SYS_GOPLEN, },     // 5
    {SYS_VIMINQP, },    // 6
    {SYS_VIMAXQP, },    // 7
    {SYS_RESWIDTH, },   // 8
    {SYS_RESHEIGHT, },  // 9
    {SYS_AESKEY, },      // 10
    {SYS_UDID, },        // 11
    {SYS_TOKEN, },       // 12
    {SYS_URL, },       // 13
    {SYS_RND, },       // 14
    {SYS_SIP, },       // 15
    {SYS_SP, },       // 16
    {SYS_CDS_ON, },      // 17
    {SYS_CDS_OFF, },      // 18
    {SYS_MICD_GAIN, },      // 19
    {SYS_SPKD_GAIN, },      // 20
    {SYS_MICA_GAIN, },      // 21
    {SYS_SPKA_GAIN, },      // 22
    {SYS_ECHO, },      // 23
    {SYS_AGC_NR, },      // 24
    {SYS_MAX_KEY, }      // 25
};

void *g_cfg_handle;
int g_doorbell_load_config = 0;
CFG_DOORBELL_INT_t g_cfg_doorbell_int;


/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/

// static int init_doorbellcfg_param(DOORBELL_CONFIG_t *pDoorbellCfg)
static int init_doorbellcfg_param(void)
{
    char value[CFG_VALUE_BUF_SIZE] = {0};
    int i = 0;
    int iRet = 0;

    printf("dbcfg time S: %u\r\n", get_tick_count());
    for(i = 0; i < SYS_MAX_KEY; i++)
    {
        memset(value, 0x00, CFG_VALUE_BUF_SIZE);
        if(ak_ini_get_item_value(g_cfg_handle, DOORBELL_CFG_NAME, \
                            tableCfg[i].key, tableCfg[i].value) == 0)
        {
            ak_print_normal("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, \
                        tableCfg[i].key, tableCfg[i].value);
        }
        else
        {
            ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, tableCfg[i].key);
            iRet = -1;
            break;
        }
    }
    printf("dbcfg time E: %u\r\n", get_tick_count());
    return iRet;
}

void doorbell_config_init_ini(void)
{
    int ret = 0;
    int cnt = 0;

    g_cfg_handle = ak_ini_init("APPINI");
    if(NULL == g_cfg_handle)
    {
        ak_print_error_ex("open config file failed.\n");
		ret = -1; //exit
    } 
    else
    {
		ak_print_normal_ex("Doorbell config file open successful.\r\n");	
	}

    if(ret == 0)
    {
        ret = init_doorbellcfg_param();
        if(ret != 0)
        {
            ak_print_error_ex("Init sys cfg failed!\n");
        }
        else
        {
            g_doorbell_load_config = 1;
            g_cfg_doorbell_int.flicker = atoi(tableCfg[0].value);
            g_cfg_doorbell_int.flip_updown = atoi(tableCfg[1].value);
            g_cfg_doorbell_int.flip_topbot = atoi(tableCfg[2].value);
            g_cfg_doorbell_int.bitrate = atoi(tableCfg[3].value);
            g_cfg_doorbell_int.framerate = atoi(tableCfg[4].value);
            g_cfg_doorbell_int.gop = atoi(tableCfg[5].value);
            g_cfg_doorbell_int.minqp = atoi(tableCfg[6].value);
            g_cfg_doorbell_int.maxqp = atoi(tableCfg[7].value);
            g_cfg_doorbell_int.width = atoi(tableCfg[8].value);
            g_cfg_doorbell_int.height = atoi(tableCfg[9].value);
            memcpy(g_cfg_doorbell_int.key, tableCfg[10].value, CFG_VALUE_BUF_SIZE); // hardcode 16byte, if read max length ????
            memcpy(g_cfg_doorbell_int.udid, tableCfg[11].value, CFG_VALUE_BUF_SIZE); // hardcode 24byte, if read max length ????
            memcpy(g_cfg_doorbell_int.token, tableCfg[12].value, CFG_VALUE_BUF_SIZE); // hardcode 32byte, if read max length ????
            memcpy(g_cfg_doorbell_int.url, tableCfg[13].value, CFG_VALUE_BUF_SIZE); // hardcode 32byte, if read max length ????
            memcpy(g_cfg_doorbell_int.rnd, tableCfg[14].value, CFG_VALUE_BUF_SIZE); // hardcode 32byte, if read max length ????
            memcpy(g_cfg_doorbell_int.sip, tableCfg[15].value, CFG_VALUE_BUF_SIZE); // hardcode 32byte, if read max length ????
            memcpy(g_cfg_doorbell_int.sp, tableCfg[16].value, CFG_VALUE_BUF_SIZE); // hardcode 32byte, if read max length ????
            g_cfg_doorbell_int.cds_on = atoi(tableCfg[17].value);
            g_cfg_doorbell_int.cds_off = atoi(tableCfg[18].value);

            g_cfg_doorbell_int.micd_gain = atoi(tableCfg[19].value);
            g_cfg_doorbell_int.spkd_gain = atoi(tableCfg[20].value);
            g_cfg_doorbell_int.mica_gain = atoi(tableCfg[21].value);
            g_cfg_doorbell_int.spka_gain = atoi(tableCfg[22].value);

            g_cfg_doorbell_int.echo = atoi(tableCfg[23].value);
            g_cfg_doorbell_int.agc_nr = atoi(tableCfg[24].value);
        }
        ak_ini_destroy(g_cfg_handle);
        g_cfg_handle = NULL;
    }
}

void doorbell_config_release_ini(void)
{
    if(g_cfg_handle)
    {
	    ak_ini_destroy(g_cfg_handle);
    }
}

/*
Set one item to config
*/
int doorbell_set_item(char *pItem, int iValue)
{
    int iRet = 0;
    char str[CFG_VALUE_BUF_SIZE] = {0x00, };

    if(iValue < 0 || pItem == NULL)
    {
        ak_print_error_ex("Parameter is invalid\r\n");
        iRet = -1;
    }
    else
    {
        memset(str, 0x00, CFG_VALUE_BUF_SIZE);
        sprintf(str, "%d", iValue);

        if(g_cfg_handle == NULL)
        {
            g_cfg_handle = ak_ini_init("APPINI");
        }

        if(g_cfg_handle == NULL)
        {
            ak_print_error_ex("open ini fail.\n");
            iRet = -2;
        }
        else
        {
            iRet = ak_ini_set_item_value(g_cfg_handle, \
                DOORBELL_CFG_NAME, pItem, str);
            if(iRet != 0)
            {
                ak_print_error_ex("Set %s->%s: %d failed!\n", \
                                        DOORBELL_CFG_NAME, \
                                        pItem, iValue);
            }
            else
            {
                ak_print_normal_ex("Set %s->%s: %d OK\n", \
                                        DOORBELL_CFG_NAME, \
                                        pItem, iValue);
            }
            ak_ini_destroy(g_cfg_handle);
            g_cfg_handle = NULL;
        }
    }
    return iRet;
}

/*
Get one item from config
*/
int doorbell_get_item(char *pItem, int *value)
{
    int iRet = 0;
    char strValue[CFG_VALUE_BUF_SIZE] = {0};
    if(value == NULL || pItem == NULL)
    {
        ak_print_error_ex("Parameter is invalid\r\n");
        return -1;
    }

    if(g_cfg_handle == NULL)
    {
        g_cfg_handle = ak_ini_init("APPINI");
    }

    if(g_cfg_handle == NULL)
    {
        ak_print_error_ex("open ini fail.\n");
        iRet = -2;
    }
    else
    {
        memset(strValue, 0x00, CFG_VALUE_BUF_SIZE);
        if(ak_ini_get_item_value(g_cfg_handle, \
                DOORBELL_CFG_NAME, pItem, strValue) == 0)
        {
            *value = atoi(strValue);
        }
        else
        {
            ak_print_error("CFG: read %s config failed!\n", pItem);
            iRet = -3;
        }
        ak_ini_destroy(g_cfg_handle);
        g_cfg_handle = NULL;
    }
    return iRet;
}

/*---------------------------------------------------------------------------*/
// get set item using string
/*
length of pItem and pValue must be greater than 64 byte. 65bytes is the best
*/
int doorbell_set_item_string(char *pItem, char *pValue)
{
    int iRet = 0;
    if(pValue == NULL || pItem == NULL)
    {
        iRet = -1;
    }
    else
    {
        if(g_cfg_handle == NULL)
        {
            g_cfg_handle = ak_ini_init("APPINI");
        }
        if(g_cfg_handle == NULL)
        {
            ak_print_error_ex("open ini fail.\n");
            iRet = -2;
        }
        else
        {
            iRet = ak_ini_set_item_value(g_cfg_handle, \
                DOORBELL_CFG_NAME, pItem, pValue);
            if(iRet != 0)
            {
                ak_print_error_ex("Set %s->%s failed!\n", \
                                DOORBELL_CFG_NAME, pItem);
            }
            ak_ini_destroy(g_cfg_handle);
            g_cfg_handle = NULL;
        }
    }
    return iRet;
}

/*
length of pItem and pValue must be greater than 64 byte. 65bytes is the best
*/
int doorbell_get_item_string(char *pItem, char *pValue)
{
    int iRet = 0;
    char strValue[CFG_VALUE_BUF_SIZE] = {0};

    // ak_print_error_ex("entered item: %s!\n", pItem);
    if(pValue == NULL || pItem == NULL)
    {
        return -1;
    }
    if(g_cfg_handle == NULL)
    {
        g_cfg_handle = ak_ini_init("APPINI");
    }
    if(g_cfg_handle == NULL)
    {
        ak_print_error_ex("open ini fail.\n");
        iRet = -2;
    }
    else
    {
        memset(strValue, 0x00, CFG_VALUE_BUF_SIZE);
        if(ak_ini_get_item_value(g_cfg_handle, \
                DOORBELL_CFG_NAME, pItem, strValue) == 0)
        {
            memcpy(pValue, strValue, strlen(strValue));
        }
        else
        {
            ak_print_error("CFG: read %s config failed!\n", pItem);
            iRet = -3;
        }
        ak_ini_destroy(g_cfg_handle);
        g_cfg_handle = NULL;
    }
    // ak_print_error_ex("exit!\n");
    return iRet;
}
/*---------------------------------------------------------------------------*/

static int get_index_key_table(char *pKey)
{
    int index = -1;
    int i = 0;
    if(pKey == NULL)
    {
        return -1;
    }
    for(i = 0; i < SYS_MAX_KEY; i++)
    {
        if(strcmp(pKey, tableCfg[i].key) == 0)
        {
            index = i;
            break;
        }
    }
    return index;
}

/*
* Common function
*/
static int doorbell_cfg_set_item(char *pcItem, int iValue)
{
    int iRet = 0;
    int idex = -1;
    int iCfgValue = 0;
    if(pcItem == NULL || iValue < 0)
    {
        return -1;
    }
    idex = get_index_key_table(pcItem);
    if(idex < 0)
    {
        iRet = -2;
    }
    else
    {
        if(g_doorbell_load_config == 1)
        {
            iCfgValue = atoi(tableCfg[idex].value);
            // ak_print_normal("%s iCfgValue: %d\n", pcItem, iCfgValue);
            if(iValue != iCfgValue)
            {
                iRet = doorbell_set_item(tableCfg[idex].key, iValue);
                if(iRet == 0)
                {
                    memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                    sprintf(tableCfg[idex].value, "%d", iValue);
                    ak_print_normal_ex("%s Index: %d value: %s\n", \
                        tableCfg[idex].key, idex, tableCfg[idex].value);
                }
                else
                {
                    iRet = -3;
                }
            }
        }
        else
        {
            iRet = doorbell_set_item(tableCfg[idex].key, iValue);
            if(iRet == 0)
            {
                memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                sprintf(tableCfg[idex].value, "%d", iValue);
            }
            else
            {
                iRet = -4;
            }
        }
    }
    return iRet;
}
static int doorbell_cfg_get_item(char *pcItem, int *pValue)
{
    int iRet = 0;
    int idex = -1;
    if(pcItem == NULL || pValue == NULL)
    {
        return -1;
    }
    idex = get_index_key_table(pcItem);
    if(idex < 0)
    {
        iRet = -2;
    }
    else
    {

        iRet = doorbell_get_item(tableCfg[idex].key, pValue);
        if(iRet == 0)
        {
            memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
            sprintf(tableCfg[idex].value, "%d", *pValue);
            ak_print_normal_ex("%s Index: %d value %s\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
        }
        else
        {
            ak_print_error_ex("%s [%d] %s FAILED! (%d)!\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value, iRet);
            iRet = -3;
        }
    }
    return iRet;
}
/*---------------------------------------------------------------------------*/
/*                   PUBLIC API                                              */
/*---------------------------------------------------------------------------*/

/* Flicker */
int doorbell_cfg_set_flicker(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue != SYS_FLICKER_MIN) && (iValue != SYS_FLICKER_MAX))
    {
        ak_print_error("Invalid input flicker!\n");
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_FLICKER, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_flicker(int *pValue)
{
    return doorbell_cfg_get_item(SYS_FLICKER, pValue);
}

/* Flip_ud */
int doorbell_cfg_set_flipupdown(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue != SYS_FLIPUPDOWN_MIN) && (iValue != SYS_FLIPUPDOWN_MAX))
    {
        ak_print_error("Invalid input flipupdown!\n");
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_FLIPUD, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_flipupdown(int *pValue)
{
    return doorbell_cfg_get_item(SYS_FLIPUD, pValue);
}

/* Flip_tb */
int doorbell_cfg_set_fliptopbottom(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue != SYS_FLIPTOPBOTTOM_MIN) && (iValue != SYS_FLIPTOPBOTTOM_MAX))
    {
        ak_print_error("Invalid input fliptopbottom!\n");
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_FLIPTB, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_fliptopbottom(int *pValue)
{
    return doorbell_cfg_get_item(SYS_FLIPTB, pValue);
}

/* Bitrate */
int doorbell_cfg_set_bitrate(int iValue)
{
    int iRet = 0;
    int config_bitrate=0;
    doorbell_cfg_get_item(SYS_BITRATE, &config_bitrate);
    ak_print_normal_ex("Current bitrate %d, input birate: %d\n", config_bitrate, iValue);
    if((iValue < SYS_BITRATE_MIN) || (iValue > SYS_BITRATE_MAX))
    {
        ak_print_error_ex("Invalid input bitrate! Min Max<%d %d>kbps\n", \
                    SYS_BITRATE_MIN, SYS_BITRATE_MAX);
        iRet = -1;
    }
    else
    {
        if (iValue!=config_bitrate){
            iRet = doorbell_cfg_set_item(SYS_BITRATE, iValue);
            ak_print_normal_ex("Set bitrate %d, handler %p\n", iValue, g_venc_handle_tinkle);
            if ((g_venc_handle_tinkle!=NULL) && (ak_venc_set_rc(g_venc_handle_tinkle, iValue)==0))
                ak_print_normal_ex("Set bitrate success: %d\n", iValue);
            else
                ak_print_error_ex("Set bitrate %d, handler %p\n", iValue, g_venc_handle_tinkle);
        } else
            ak_print_normal_ex("No Change to bitrate\n");
    }
    return iRet;
}

int doorbell_cfg_get_bitrate(int *pValue)
{
    return doorbell_cfg_get_item(SYS_BITRATE, pValue);
}

/* Framerate */
int doorbell_cfg_set_framerate(int iValue)
{
    int iRet = 0;
    int config_framerate=0;
    doorbell_cfg_get_item(SYS_FRAMERATE, &config_framerate);
    ak_print_normal_ex("Current framerate %d, input %d\n", config_framerate, iValue);
    if((iValue < SYS_FRAMERATE_MIN) || (iValue > SYS_FRAMERATE_MAX))
    {
        ak_print_error_ex("Invalid input framerate! Min Max<%d %d>fps\n", \
                    SYS_FRAMERATE_MIN, SYS_FRAMERATE_MAX);
        iRet = -1;
    }
    else
    {
        if (config_framerate!=iValue){
            iRet = doorbell_cfg_set_item(SYS_FRAMERATE, iValue);
            ak_print_normal_ex("Set framerate return %d\n", iRet);
        } else
            ak_print_normal_ex("Framerate no change\n");
    }
    return iRet;
}

int doorbell_cfg_get_framerate(int *pValue)
{
    return doorbell_cfg_get_item(SYS_FRAMERATE, pValue);
}

/* Gop_len */
int doorbell_cfg_set_goplen(int iValue)
{
    int iRet = 0;
    int config_gop=0;
    doorbell_cfg_get_item(SYS_GOPLEN, &config_gop);
    ak_print_normal_ex("Current GOP %d, set value: %d\n", config_gop, iValue);
    if((iValue < SYS_GOPLEN_MIN) || (iValue > SYS_GOPLEN_MAX))
    {
        ak_print_error_ex("Invalid input goplen! Min Max<%d %d>\n", \
                    SYS_GOPLEN_MIN, SYS_GOPLEN_MAX);
        iRet = -1;
    }
    else
    {
        if (iValue!=config_gop){
            iRet = doorbell_cfg_set_item(SYS_GOPLEN, iValue);
            ak_print_normal_ex("Set GOP return: %d\n", iRet);
        } else
            ak_print_normal_ex("GOP unchanged\n");
    }
    return iRet;
}

int doorbell_cfg_get_goplen(int *pValue)
{
    return doorbell_cfg_get_item(SYS_GOPLEN, pValue);
}

/* Minqp */
int doorbell_cfg_set_minqp(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue < SYS_QP_MIN) || (iValue > SYS_QP_MAX))
    {
        ak_print_error_ex("Invalid input minqp! Min Max<%d %d>\n", \
                    SYS_QP_MIN, SYS_QP_MAX);
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_VIMINQP, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_minqp(int *pValue)
{
    return doorbell_cfg_get_item(SYS_VIMINQP, pValue);
}

/* Maxqp */
int doorbell_cfg_set_maxqp(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue < SYS_QP_MIN) || (iValue > SYS_QP_MAX))
    {
        ak_print_error_ex("Invalid input maxqp! Min Max<%d %d>\n", \
                    SYS_QP_MIN, SYS_QP_MAX);
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_VIMAXQP, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_maxqp(int *pValue)
{
    return doorbell_cfg_get_item(SYS_VIMAXQP, pValue);
}

/* Resolution Width */
int doorbell_cfg_set_reswidth(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue < SYS_RESWIDTH_MIN) || (iValue > SYS_RESWIDTH_MAX))
    {
        ak_print_error_ex("Invalid input reswidth! Min Max<%d %d>\n", \
                    SYS_RESWIDTH_MIN, SYS_RESWIDTH_MAX);
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_RESWIDTH, iValue);
    }
    return iRet;
}
int doorbell_cfg_get_reswidth(int *pValue)
{
    return doorbell_cfg_get_item(SYS_RESWIDTH, pValue);
}

/* Resolution Height */
int doorbell_cfg_set_resheight(int iValue)
{
    int iRet = 0;
    int config_resolution=0;
    doorbell_cfg_get_item(SYS_RESHEIGHT, &config_resolution);
    ak_print_normal_ex("Current resolution: %d, set resolution: %d\n", config_resolution, iValue);
    if((iValue < SYS_RESHEIGHT_MIN) || (iValue > SYS_RESHEIGHT_MAX))
    {
        ak_print_error_ex("Invalid input resheight! Min Max<%d %d>\n", \
                    SYS_RESHEIGHT_MIN, SYS_RESHEIGHT_MAX);
        iRet = -1;
    }
    else
    {
        if (iValue!=config_resolution){
            iRet = doorbell_cfg_set_item(SYS_RESHEIGHT, iValue);
            ak_print_normal_ex("Set resolution return: %d\n", iRet);
        } else
            ak_print_normal_ex("Resolution un-changed\n");
    }
    return iRet;
}
int doorbell_cfg_get_resheight(int *pValue)
{
    return doorbell_cfg_get_item(SYS_RESHEIGHT, pValue);
}

/* Aes Key with 16 byte character */
int doorbell_cfg_set_aeskey(char *pKey)
{
    int iRet = 0;
    int iLenKey = 0;
    int idex = -1;
    char acAesKey[17] = {0x00, };
    int i = 0;
    if(pKey == NULL)
    {
        return -1;
    }
    iLenKey = strlen(pKey);
    if(iLenKey < 16)
    {
        ak_print_error_ex("Length of Key is too short (%d). Should be must greater 16 characters\n", iLenKey);
        iRet = -2;
    }
    else
    {
        // copy 16 characters key to array, avoid dummy data after 16 characters^M
        for(i = 0; i < 16; i++)
        {
            acAesKey[i] = *(pKey + i);
        }
        acAesKey[16] = '\0';
        idex = get_index_key_table(SYS_AESKEY);
        if(idex < 0)
        {
            iRet = -2;
        }
        else
        {
            if(g_doorbell_load_config == 1) // System loaded config
            {
                if(strncmp(acAesKey, tableCfg[idex].value, 16) != 0)
                {
                    iRet = doorbell_set_item_string(SYS_AESKEY, acAesKey);
                    if(iRet == 0)
                    {
                        memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                        sprintf(tableCfg[idex].value, "%s", acAesKey);
                        ak_print_normal_ex("%s Index: %d value: %s\n", \
                            tableCfg[idex].key, idex, tableCfg[idex].value);
                    }
                    else
                    {
                        iRet = -3;
                    }

                    // test set AES on flight
                    deinit_aes_engine_key();
                    if(init_aes_engine_key(acAesKey) != 0)
                    {
                        ak_print_error_ex("set AES on flight faied!");
                        // sys_reset();
                    }
                    else
                    {
                        ak_print_error("Have just deinit engine encrypt\r\n");
                    }
                }
            }
            else
            {
                iRet = doorbell_set_item_string(SYS_AESKEY, acAesKey);
                if(iRet == 0)
                {
                    memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                    sprintf(tableCfg[idex].value, "%s", acAesKey);
                    ak_print_normal_ex("%s Index: %d value: %s\n", \
                        tableCfg[idex].key, idex, tableCfg[idex].value);
                }
                else
                {
                    iRet = -3;
                }

                // test set AES on flight
                deinit_aes_engine_key();
                if(init_aes_engine_key(acAesKey) != 0)
                {
                    ak_print_error_ex("set AES on flight faied!");
                    // sys_reset();
                }
                else
                {
                    ak_print_error("Have just deinit engine encrypt\r\n");
                }
            }
        }   
    }
    return iRet;
}

/* Ps Rnd with 12 byte character */
int doorbell_cfg_set_psrnd(char *pRnd)
{
    int iRet = 0;
    int iLenRnd = 0;
    int idex = -1;
    char acRnd[13] = {0x00, };
    int i = 0;
    if(pRnd == NULL)
    {
        return -1;
    }
    iLenRnd = strlen(pRnd);
    if(iLenRnd < 12)
    {
        ak_print_error_ex("Length of Rnd is too short (%d). Should be must greater 12 characters\n", iLenRnd);
        iRet = -2;
    }
    else
    {
        // copy 12 characters key to array, avoid dummy data after 12 characters^M
        for(i = 0; i < 12; i++)
        {
            acRnd[i] = *(pRnd + i);
        }
        acRnd[13] = '\0';
        idex = get_index_key_table(SYS_RND);
        if(idex < 0)
        {
            iRet = -2;
        }
        else
        {
            if(g_doorbell_load_config == 1) // System loaded config
            {
                if(strncmp(acRnd, tableCfg[idex].value, 12) != 0)
                {
                    iRet = doorbell_set_item_string(SYS_RND, acRnd);
                    if(iRet == 0)
                    {
                        memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                        sprintf(tableCfg[idex].value, "%s", acRnd);
                        ak_print_normal_ex("%s Index: %d value: %s\n", \
                            tableCfg[idex].key, idex, tableCfg[idex].value);
                    }
                    else
                    {
                        iRet = -3;
                    }
                }
            }
            else
            {
                iRet = doorbell_set_item_string(SYS_RND, acRnd);
                if(iRet == 0)
                {
                    memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                    sprintf(tableCfg[idex].value, "%s", acRnd);
                    ak_print_normal_ex("%s Index: %d value: %s\n", \
                        tableCfg[idex].key, idex, tableCfg[idex].value);
                }
                else
                {
                    iRet = -3;
                }
            }
        }   
    }
    return iRet;
}
/* Ps Sip with up to 15 byte character */
int doorbell_cfg_set_pssip(char *pSip)
{
    int iRet = 0;
    int iLenSip = 0;
    int idex = -1;
    char acSip[13] = {0x00, };
    int i = 0;
    if(pSip == NULL)
    {
        return -1;
    }
    iLenSip = strlen(pSip);
    if(iLenSip < 7)
    {
        ak_print_error_ex("Length of Sip is too short (%d). Should be must be >= 7 characters\n", iLenSip);
        iRet = -2;
    }
    else
    {
        // copy iLenSip characters key to array, avoid dummy data after iLenSip characters^M
        for(i = 0; i < iLenSip; i++)
        {
            acSip[i] = *(pSip + i);
        }
        acSip[iLenSip] = '\0';
        idex = get_index_key_table(SYS_SIP);
        if(idex < 0)
        {
            iRet = -2;
        }
        else
        {
            if(g_doorbell_load_config == 1) // System loaded config
            {
                if(strncmp(acSip, tableCfg[idex].value, iLenSip) != 0)
                {
                    iRet = doorbell_set_item_string(SYS_SIP, acSip);
                    if(iRet == 0)
                    {
                        memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                        sprintf(tableCfg[idex].value, "%s", acSip);
                        ak_print_normal_ex("%s Index: %d value: %s\n", \
                            tableCfg[idex].key, idex, tableCfg[idex].value);
                    }
                    else
                    {
                        iRet = -3;
                    }
                }
            }
            else
            {
                iRet = doorbell_set_item_string(SYS_SIP, acSip);
                if(iRet == 0)
                {
                    memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                    sprintf(tableCfg[idex].value, "%s", acSip);
                    ak_print_normal_ex("%s Index: %d value: %s\n", \
                        tableCfg[idex].key, idex, tableCfg[idex].value);
                }
                else
                {
                    iRet = -3;
                }
            }
        }   
    }
    return iRet;
}
/* Ps Sp with upto 5 byte character */
int doorbell_cfg_set_pssp(char *pSp)
{
    int iRet = 0;
    int iLenSp = 0;
    int idex = -1;
    char acSp[8] = {0x00, };
    int i = 0;
    if(pSp == NULL)
    {
        return -1;
    }
    iLenSp = strlen(pSp);
    if(iLenSp < 2)
    {
        ak_print_error_ex("Length of Sp is too short (%d). Should be must be >= 2 characters\n", iLenSp);
        iRet = -2;
    }
    else
    {
        // copy iLenSp characters key to array, avoid dummy data after iLenSp characters^M
        for(i = 0; i < iLenSp; i++)
        {
            acSp[i] = *(pSp + i);
        }
        acSp[iLenSp] = '\0';
        idex = get_index_key_table(SYS_SP);
        if(idex < 0)
        {
            iRet = -2;
        }
        else
        {
            if(g_doorbell_load_config == 1) // System loaded config
            {
                if(strncmp(acSp, tableCfg[idex].value, iLenSp) != 0)
                {
                    iRet = doorbell_set_item_string(SYS_SP, acSp);
                    if(iRet == 0)
                    {
                        memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                        sprintf(tableCfg[idex].value, "%s", acSp);
                        ak_print_normal_ex("%s Index: %d value: %s\n", \
                            tableCfg[idex].key, idex, tableCfg[idex].value);
                    }
                    else
                    {
                        iRet = -3;
                    }
                }
            }
            else
            {
                iRet = doorbell_set_item_string(SYS_SP, acSp);
                if(iRet == 0)
                {
                    memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                    sprintf(tableCfg[idex].value, "%s", acSp);
                    ak_print_normal_ex("%s Index: %d value: %s\n", \
                        tableCfg[idex].key, idex, tableCfg[idex].value);
                }
                else
                {
                    iRet = -3;
                }
            }
        }   
    }
    return iRet;
}
int doorbell_cfg_get_aeskey(char *pKey)
{
    int iRet = 0;
    int idex = -1;
    char acAesKey[17] = {0x00, };
    int i = 0;
    if(pKey == NULL)
    {
        return -1;
    }
    idex = get_index_key_table(SYS_AESKEY);
    ak_print_normal_ex("Index of AES KEY: %d, address key: %p\n", idex, pKey);
    if(idex < 0)
    {
        iRet = -2;
    }
    else
    {
        iRet = doorbell_get_item_string(SYS_AESKEY, acAesKey);
        if(iRet == 0)
        {
            acAesKey[16] = '\0';
            memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
            sprintf(tableCfg[idex].value, "%s", acAesKey);
            ak_print_normal_ex("%s Index: %d value %s\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            for(i = 0; i < 16; i++)
            {
                *(pKey + i) = acAesKey[i];
            }
            *(pKey + 16) = '\0';
        }
        else
        {
            ak_print_error_ex("%s [%d] %s FAILED!\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            iRet = -3;
        }
    }
    return iRet;
}

int doorbell_cfg_get_rnd(char *pRnd)
{
    int iRet = 0;
    int idex = -1;
    char acRnd[13] = {0x00, };
    int i = 0;
    if(pRnd == NULL)
    {
        return -1;
    }
    idex = get_index_key_table(SYS_RND);
    ak_print_normal_ex("Index of RND: %d, address key: %p\n", idex, pRnd);
    if(idex < 0)
    {
        iRet = -2;
    }
    else
    {
        iRet = doorbell_get_item_string(SYS_RND, acRnd);
        if(iRet == 0)
        {
            acRnd[12] = '\0';
            memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
            sprintf(tableCfg[idex].value, "%s", acRnd);
            ak_print_normal_ex("%s Index: %d value %s\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            for(i = 0; i < 12; i++)
            {
                *(pRnd + i) = acRnd[i];
            }
            *(pRnd + 12) = '\0';
        }
        else
        {
            ak_print_error_ex("%s [%d] %s FAILED!\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            iRet = -3;
        }
    }
    return iRet;
}

int doorbell_cfg_get_sip(char *pSip)
{
    int iRet = 0;
    int idex = -1;
    char acSip[16] = {0x00, };
    int i = 0;
    int iLenSip;
    if(pSip == NULL)
    {
        return -1;
    }
    idex = get_index_key_table(SYS_SIP);
    ak_print_normal_ex("Index of SIP: %d, address key: %p\n", idex, pSip);
    if(idex < 0)
    {
        iRet = -2;
    }
    else
    {
        iRet = doorbell_get_item_string(SYS_SIP, acSip);
        iLenSip=strlen(acSip);
        if(iLenSip>15)
        	iLenSip=15;
        if(iRet == 0)
        {
            acSip[iLenSip] = '\0';
            memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
            sprintf(tableCfg[idex].value, "%s", acSip);
            ak_print_normal_ex("%s Index: %d value %s\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            for(i = 0; i < iLenSip; i++)
            {
                *(pSip + i) = acSip[i];
            }
            *(pSip + iLenSip) = '\0';
        }
        else
        {
            ak_print_error_ex("%s [%d] %s FAILED!\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            iRet = -3;
        }
    }
    return iRet;
}

int doorbell_cfg_get_sp(char *pSp)
{
    int iRet = 0;
    int idex = -1;
    char acSp[8] = {0x00, };
    int i = 0;
    int iLenSp;
    if(pSp == NULL)
    {
        return -1;
    }
    idex = get_index_key_table(SYS_SP);
    ak_print_normal_ex("Index of SP: %d, address key: %p\n", idex, pSp);
    if(idex < 0)
    {
        iRet = -2;
    }
    else
    {
        iRet = doorbell_get_item_string(SYS_SP, acSp);
        iLenSp=strlen(acSp);
        if(iLenSp>6)
        	iLenSp=6;
        if(iRet == 0)
        {
            acSp[iLenSp] = '\0';
            memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
            sprintf(tableCfg[idex].value, "%s", acSp);
            ak_print_normal_ex("%s Index: %d value %s\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            for(i = 0; i < iLenSp; i++)
            {
                *(pSp + i) = acSp[i];
            }
            *(pSp + iLenSp) = '\0';
        }
        else
        {
            ak_print_error_ex("%s [%d] %s FAILED!\n", \
                    tableCfg[idex].key, idex, tableCfg[idex].value);
            iRet = -3;
        }
    }
    return iRet;
}
/* Porcess UDID and Token */
/* TODO: need to think about security */
/* udid with 24 byte */
int doorbell_cfg_set_string_keygroup(char *pItem, char *pValue)
{
    int iRet = 0;
    int idex = -1;
    int iLenKey = 0;
    char strValue[CFG_VALUE_BUF_SIZE + 1] = {0x00 };

    if(pItem == NULL || pValue == NULL)
    {
        ak_print_error_ex("Parameter is NULL\n");
        iRet = -1;
    }
    else
    {
        idex = get_index_key_table(pItem);
        if(idex < 0)
        {
            ak_print_error_ex("No result for key: %s\r\n", pItem);
            iRet = -2;
        }
        else
        {
            /* copy key to local */
            memset(strValue, 0x00, CFG_VALUE_BUF_SIZE + 1);

            iLenKey = strlen(pValue);
            ak_print_error_ex("pValue: %s iLenKey:%d\r\n", pValue, iLenKey);
            if( iLenKey > CFG_VALUE_BUF_SIZE)
            {
                ak_print_error_ex("Length of key: %d is too long.\r\n", iLenKey);
                iRet = -3;
            }
            else
            {
                memcpy(strValue, pValue, strlen(pValue));
                ak_print_error_ex("strValue: %s strlen(pValue):%d\r\n", strValue, strlen(pValue));
                if(g_doorbell_load_config == 1) // System loaded config
                {
                    /* if the received value is different with table's value, we set 
                    it to config */
                    if(strncmp(strValue, tableCfg[idex].value, CFG_VALUE_BUF_SIZE) != 0)
                    {
                        iRet = doorbell_set_item_string(pItem, strValue);
                        if(iRet == 0)
                        {
                            ak_print_error_ex("value set: %s\r\n", strValue);
                            memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                            sprintf(tableCfg[idex].value, "%s", strValue);
                            ak_print_normal_ex("%s Index: %d value: %s\n", \
                                tableCfg[idex].key, idex, tableCfg[idex].value);
                        }
                        else
                        {
                            ak_print_error_ex("Set %s FAILED!(%d)\r\n", pItem, iRet);
                            iRet = -4;
                        }
                    }
                }
                else
                {
                    iRet = doorbell_set_item_string(pItem, strValue);
                    if(iRet == 0)
                    {
                        ak_print_error_ex("value set: %s\r\n", strValue);
                        memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                        sprintf(tableCfg[idex].value, "%s", strValue);
                        ak_print_normal_ex("%s Index: %d value: %s\n", \
                            tableCfg[idex].key, idex, tableCfg[idex].value);
                    }
                    else
                    {
                        iRet = -5;
                    }
                } // else of if(g_doorbell_load_config == 1)
            } // else of if (iLenKey >...)
        } // else of if(idex < 0)
    }
    return iRet;
}

int doorbell_cfg_get_string_keygroup(char *pItem, char *pValue)
{
    int iRet = 0;
    int idex = -1;
    char strValue[CFG_VALUE_BUF_SIZE + 1] = {0x00 };
    int i = 0;
    int iLenKey = 0;
    if(pItem == NULL || pValue == NULL)
    {
        ak_print_error_ex("Parameter is NULL\n");
        iRet = -1;
    }
    else
    {
        memset(strValue, 0x00, CFG_VALUE_BUF_SIZE + 1);
        idex = get_index_key_table(pItem);
        if(idex < 0)
        {
            ak_print_error_ex("No result for key: %s index: %d\r\n", pItem, idex);
            iRet = -2;
        }
        else
        {
            iRet = doorbell_get_item_string(pItem, strValue);
            if(iRet == 0)
            {
                ak_print_error_ex("value get: %s\r\n", strValue);
                memset(tableCfg[idex].value, 0x00, CFG_VALUE_BUF_SIZE);
                sprintf(tableCfg[idex].value, "%s", strValue);
                ak_print_normal_ex("%s Index: %d value %s\n", \
                        tableCfg[idex].key, idex, tableCfg[idex].value);
                iLenKey = strlen(strValue);
                memcpy(pValue, strValue, iLenKey);
            }
            else
            {
                ak_print_error_ex("%s [%d] %s FAILED!\n", \
                        tableCfg[idex].key, idex, tableCfg[idex].value);
                iRet = -3;
            }
        }
    }// else of if(pItem == NULL || pValue == NULL)
    return iRet;
}

int doorbell_cfg_set_udid(char *pUdid)
{
    int iRet = 0;
    ak_print_error_ex("pUdid: %s\r\n", pUdid);
    iRet = doorbell_cfg_set_string_keygroup(SYS_UDID, pUdid);
    if(iRet < 0)
    {
        ak_print_error_ex("Set UDID failed! (%d) [-1:-5]\r\n", iRet);
    }
    return iRet;
}

int doorbell_cfg_get_udid(char *pUdid)
{
    int iRet = 0;
    if(g_doorbell_load_config == 1)
    {
        memcpy(pUdid, tableCfg[11].value, CFG_VALUE_BUF_SIZE);
    }
    else
    {
        iRet = doorbell_cfg_get_string_keygroup(SYS_UDID, pUdid);
        if(iRet < 0)
        {
            ak_print_error_ex("Get UDID failed! (%d) [-1:-5]\r\n", iRet);
        }
    }
    return iRet;
}

/* token with 32 byte */
int doorbell_cfg_set_token(char *pToken)
{
    int iRet = 0;
    ak_print_error_ex("pToken: %s\r\n", pToken);
    iRet = doorbell_cfg_set_string_keygroup(SYS_TOKEN, pToken);
    if(iRet < 0)
    {
        ak_print_error_ex("Set Token failed! (%d) [-1:-5]\r\n", iRet);
    }
    return iRet;
}

int doorbell_cfg_get_token(char *pToken)
{
    int iRet = 0;
    if(g_doorbell_load_config == 1)
    {
        memcpy(pToken, tableCfg[12].value, CFG_VALUE_BUF_SIZE);
    }
    else
    {
        iRet = doorbell_cfg_get_string_keygroup(SYS_TOKEN, pToken);
        if(iRet < 0)
        {
            ak_print_error_ex("Get Token failed! (%d) [-1:-5]\r\n", iRet);
        }
    }
    return iRet;
}

int doorbell_cfg_set_url(char *pUrl)
{
    int iRet = 0;
    ak_print_error_ex("pUrl: %s\r\n", pUrl);
    iRet = doorbell_cfg_set_string_keygroup(SYS_URL, pUrl);
    if(iRet < 0)
    {
        ak_print_error_ex("Set URL failed! (%d) [-1:-5]\r\n", iRet);
    }
    return iRet;
}

int doorbell_cfg_get_url(char *pUrl)
{
    int iRet = 0;
    if(g_doorbell_load_config == 1)
    {
        memcpy(pUrl, tableCfg[13].value, CFG_VALUE_BUF_SIZE);
    }
    else
    {
        iRet = doorbell_cfg_get_string_keygroup(SYS_URL, pUrl);
        if(iRet < 0)
        {
            ak_print_error_ex("Get URL failed! (%d) [-1:-5]\r\n", iRet);
        }
    }
    return iRet;
}

int doorbell_cfg_get_macaddress(char *pMAC)
{
    int iRet = 0;
    char acUdid[SYS_MAX_LENGTH_KEY + 1] = {0x00 };
    char *p = NULL;
    int len = 12;
    if(pMAC == NULL)
    {
        ak_print_error_ex("Parameter is NULL\r\n");
        iRet = -1;
    }
    else
    {
        memset(acUdid, 0x00, SYS_MAX_LENGTH_KEY + 1);
        iRet = doorbell_cfg_get_udid(acUdid);
        if(iRet != 0)
        {   
            ak_print_error_ex("Get config failed!\r\n");
        }
        else
        {
            p = acUdid + 6;
            memcpy(pMAC, p, 12);
            ak_print_error_ex("MAC: %s\r\n", pMAC);
        }
    }
    return iRet;
}



/* For get/set threshold value to turn on/off led */
/* Resolution Height */
int doorbell_cfg_set_cds_on(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if(iValue <= 0)
    {
        ak_print_error_ex("Invalid input iValue: %d\r\n", iValue);
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_CDS_ON, iValue);
        if (iRet == 0)
        {
            g_cfg_doorbell_int.cds_on = iValue;
        }
    }
    return iRet;
}

int doorbell_cfg_get_cds_on(int *pValue)
{
    if(g_doorbell_load_config == 1)
    {
        *pValue = g_cfg_doorbell_int.cds_on;
        return 0;
    }
    else
    {
        return doorbell_cfg_get_item(SYS_CDS_ON, pValue);
    }
}

int doorbell_cfg_set_cds_off(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if(iValue <= 0)
    {
        ak_print_error_ex("Invalid input iValue: %d\r\n", iValue);
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_CDS_OFF, iValue);
        if (iRet == 0)
        {
            g_cfg_doorbell_int.cds_off = iValue;
        }
    }
    return iRet;
}

int doorbell_cfg_get_cds_off(int *pValue)
{
    if(g_doorbell_load_config == 1)
    {
        *pValue = g_cfg_doorbell_int.cds_off;
        return 0;
    }
    else
    {
        return doorbell_cfg_get_item(SYS_CDS_OFF, pValue);
    }
}
/*++++++++++++++++++++++++++++++++++++++++++++++++*/


/*---------------------------------------------------------------------------*/
/* For Mic and speaker gain */
/* DIGITAL */
/* Mic digital gain */
int doorbell_cfg_set_micd_gain(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue < SYS_MICSPK_DIGITAL_MIN) || (iValue > SYS_MICSPK_DIGITAL_MAX))
    {
        ak_print_error("Invalid input mic digital!\n");
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_MICD_GAIN, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_micd_gain(int *pValue)
{
    return doorbell_cfg_get_item(SYS_MICD_GAIN, pValue);
}
/* Speaker digital gain */
int doorbell_cfg_set_spkd_gain(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue < SYS_MICSPK_DIGITAL_MIN) || (iValue > SYS_MICSPK_DIGITAL_MAX))
    {
        ak_print_error("Invalid input spk digital!\n");
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_SPKD_GAIN, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_spkd_gain(int *pValue)
{
    return doorbell_cfg_get_item(SYS_SPKD_GAIN, pValue);
}


/* ANALOG */
/* Mic analog gain */
int doorbell_cfg_set_mica_gain(int iValue)
{
    int iRet = 0;
    int config_mic_gain=0;
    doorbell_cfg_get_item(SYS_MICA_GAIN, &config_mic_gain);
    ak_print_normal_ex("Current mic gain %d, set value: %d\n", config_mic_gain, iValue);
    // if((iValue < SYS_MICSPK_ANALOG_MIN) || (iValue > SYS_MICSPK_ANALOG_MAX))
    if((iValue < SYS_MICSPK_ANALOG_MIN) || (iValue > SYS_MICSPK_DIGITAL_MAX))
    {
        ak_print_error("Invalid input mic analog!\n");
        iRet = -1;
    }
    else
    {
        if (iValue!=config_mic_gain){
            iRet = doorbell_cfg_set_item(SYS_MICA_GAIN, iValue);
            ak_print_normal_ex("Set mic gain return %d\n", iRet);
        } else
            ak_print_normal_ex("Mic gain unchanged\n");
    }
    return iRet;
}

int doorbell_cfg_get_mica_gain(int *pValue)
{
    return doorbell_cfg_get_item(SYS_MICA_GAIN, pValue);
}
/* Speaker analog gain */
int doorbell_cfg_set_spka_gain(int iValue)
{
    int iRet = 0;
    int config_spk_gain=0;
    doorbell_cfg_get_item(SYS_SPKA_GAIN, &config_spk_gain);
    ak_print_normal_ex("Current speaker gain %d, set speaker gain: %d\n", config_spk_gain, iValue);
    // if((iValue < SYS_MICSPK_ANALOG_MIN) || (iValue > SYS_MICSPK_ANALOG_MAX))
    if((iValue < SYS_MICSPK_ANALOG_MIN) || (iValue > SYS_MICSPK_DIGITAL_MAX))
    {
        ak_print_error("Invalid input spk analog!\n");
        iRet = -1;
    }
    else
    {
        if (iValue!=config_spk_gain){
            iRet = doorbell_cfg_set_item(SYS_SPKA_GAIN, iValue);
            ak_print_normal_ex("Set speaker return %d\n", iRet);
        } else
            ak_print_normal_ex("Speaker gain unchanged\n");
    }
    return iRet;
}

int doorbell_cfg_get_spka_gain(int *pValue)
{
    return doorbell_cfg_get_item(SYS_SPKA_GAIN, pValue);
}

/* Echo cancellation */
int doorbell_cfg_set_echocancellation(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue != 0) && (iValue != 1))
    {
        ak_print_error("Invalid value echo cancellation!\n");
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_ECHO, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_echocancellation(int *pValue)
{
    return doorbell_cfg_get_item(SYS_ECHO, pValue);
}

/* Auto gain control and noise reduction */
int doorbell_cfg_set_agcnr(int iValue)
{
    int iRet = 0;
    ak_print_normal_ex("value: %d\n", iValue);
    if((iValue != 0) && (iValue != 1))
    {
        ak_print_error("Invalid value auto gain control and noise reduction!\n");
        iRet = -1;
    }
    else
    {
        iRet = doorbell_cfg_set_item(SYS_AGC_NR, iValue);
    }
    return iRet;
}

int doorbell_cfg_get_agcnr(int *pValue)
{
    return doorbell_cfg_get_item(SYS_AGC_NR, pValue);
}

/*---------------------------------------------------------------------------*/

int doorbell_load_config(CFG_DOORBELL_INT_t *pCfgDoorbellInt)
{
    int iRet = 0;
    if(pCfgDoorbellInt == NULL)
    {
        return -1;
    }
    if(g_doorbell_load_config != 1)
    {
        return -2;
    }
    // load config from table
    pCfgDoorbellInt->flicker = g_cfg_doorbell_int.flicker;
    pCfgDoorbellInt->flip_updown = g_cfg_doorbell_int.flip_updown;
    pCfgDoorbellInt->flip_topbot = g_cfg_doorbell_int.flip_topbot;
    pCfgDoorbellInt->bitrate = g_cfg_doorbell_int.bitrate;
    pCfgDoorbellInt->framerate = g_cfg_doorbell_int.framerate;
    pCfgDoorbellInt->gop = g_cfg_doorbell_int.gop;
    pCfgDoorbellInt->minqp = g_cfg_doorbell_int.minqp;
    pCfgDoorbellInt->maxqp = g_cfg_doorbell_int.maxqp;
    pCfgDoorbellInt->width = g_cfg_doorbell_int.width;
    pCfgDoorbellInt->height = g_cfg_doorbell_int.height;
    memcpy(pCfgDoorbellInt->key, g_cfg_doorbell_int.key, CFG_VALUE_BUF_SIZE);
    memcpy(pCfgDoorbellInt->udid, g_cfg_doorbell_int.udid, CFG_VALUE_BUF_SIZE);
    memcpy(pCfgDoorbellInt->token, g_cfg_doorbell_int.token, CFG_VALUE_BUF_SIZE);
    memcpy(pCfgDoorbellInt->url, g_cfg_doorbell_int.url, CFG_VALUE_BUF_SIZE);
    memcpy(pCfgDoorbellInt->rnd, g_cfg_doorbell_int.rnd, CFG_VALUE_BUF_SIZE);
    memcpy(pCfgDoorbellInt->sip, g_cfg_doorbell_int.sip, CFG_VALUE_BUF_SIZE);
    memcpy(pCfgDoorbellInt->sp, g_cfg_doorbell_int.sp, CFG_VALUE_BUF_SIZE);

    pCfgDoorbellInt->cds_on = g_cfg_doorbell_int.cds_on;
    pCfgDoorbellInt->cds_off = g_cfg_doorbell_int.cds_off;

    pCfgDoorbellInt->micd_gain = g_cfg_doorbell_int.micd_gain;
    pCfgDoorbellInt->spkd_gain = g_cfg_doorbell_int.spkd_gain;
    pCfgDoorbellInt->mica_gain = g_cfg_doorbell_int.mica_gain;
    pCfgDoorbellInt->spka_gain = g_cfg_doorbell_int.spka_gain;

    pCfgDoorbellInt->echo = g_cfg_doorbell_int.echo;
    pCfgDoorbellInt->agc_nr = g_cfg_doorbell_int.agc_nr;
    
    return iRet;
}

void doorbell_config_test_api1(void)
{
    int iRet = 0, iValue  = 0;
    char test_key[17] = "TienTrungHS12345";
    char get_key[17];
    
    printf("%s enter!\n", __func__);
    iRet = doorbell_cfg_set_flicker(60);
    if(iRet != 0)
    {
        ak_print_error_ex("doorbell_cfg_set_flicker FAILED!\n");
    }
    iRet = doorbell_cfg_get_flicker(&iValue);
    if(iRet != 0)
    {
        ak_print_error_ex("doorbell_cfg_get_flicker FAILED!\n");
    }

    iRet = doorbell_cfg_set_aeskey(test_key);
    if(iRet != 0)
    {
        ak_print_error_ex("doorbell_cfg_set_aeskey FAILED!\n");
    }
    else
    {
        ak_print_normal("Set AES key: %s OK (len %d)\n", test_key, strlen(test_key));
    }

    iRet = doorbell_cfg_get_aeskey(get_key);
    if(iRet != 0)
    {
        ak_print_error_ex("doorbell_cfg_get_aeskey FAILED!\n");
    }
    else
    {
        get_key[16] = '\0';
        ak_print_normal("Get AES key: %s\n", get_key);
    }
}

/*---------------------------------------------------------------------------*/
/* Test write all config one time */
/*---------------------------------------------------------------------------*/
static int test_confirm_write_cfg(void)
{
    char value[CFG_VALUE_BUF_SIZE] = {0};
    int i = 0;
    int iRet = 0;
    void *cfg_handle = NULL;

    cfg_handle = ak_ini_init("APPINI");
    if(cfg_handle == NULL)
    {
        printf("Cannot init APPINI\r\n");
        return -2;
    }
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FLICKER, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_FLICKER, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_FLICKER);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FLIPUD, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_FLIPUD, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_FLIPUD);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FLIPTB, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_FLIPTB, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_FLIPTB);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_BITRATE, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_BITRATE, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_BITRATE);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FRAMERATE, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_FRAMERATE, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_FRAMERATE);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_GOPLEN, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_GOPLEN, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_GOPLEN);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_VIMINQP, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_VIMINQP, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_VIMINQP);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_VIMAXQP, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_VIMAXQP, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_VIMAXQP);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_RESWIDTH, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_RESWIDTH, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_RESWIDTH);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_RESHEIGHT, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_RESHEIGHT, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_RESHEIGHT);
    }

    memset(value, 0x00, sizeof(value));
    if(ak_ini_get_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_AESKEY, value) == 0){
        ak_print_normal_ex("Get %s->%s: %s OK!\n", DOORBELL_CFG_NAME, SYS_AESKEY, value);
    }
    else{
        ak_print_error("get %s->%s failed!\n", DOORBELL_CFG_NAME, SYS_AESKEY);
    }

    ak_ini_destroy(cfg_handle);	
    return iRet;
}

int doorbell_write_all_config(void)
{
    int iRet = 0;
    void *cfg_handle = NULL;
    char acAesKey[17] = "TienTrungHS12345";
    char strValue[16] = {0x00, };
    CFG_DOORBELL_INT_t sCfgDoorbellInt;

    ak_print_normal("ini version:%s\r\n", ak_ini_get_version());

    cfg_handle = ak_ini_init("APPINI");
    if(cfg_handle == NULL)
    {
        printf("Cannot init APPINI\r\n");
        return -2;
    }

    sCfgDoorbellInt.flicker = 60;
    sCfgDoorbellInt.flip_updown = 1;
    sCfgDoorbellInt.flip_topbot = 1;
    sCfgDoorbellInt.bitrate = 500;
    sCfgDoorbellInt.framerate = 15;
    sCfgDoorbellInt.gop = 15;
    sCfgDoorbellInt.minqp = 30;
    sCfgDoorbellInt.maxqp = 51;
    sCfgDoorbellInt.width = 1280;
    sCfgDoorbellInt.height = 720;
    memcpy(sCfgDoorbellInt.key, acAesKey, 16);

    // flicker
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.flicker);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FLICKER, strValue);
	ak_print_error("Set %s->%s ret = %d\n", SYS_FLICKER, strValue, iRet);
    // flip up down
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.flip_updown);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FLIPUD, strValue);
    ak_print_error("Set %s->%s ret = %d\n", SYS_FLIPUD, strValue, iRet);

    // flip top bottom
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.flip_topbot);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FLIPTB, strValue);
	ak_print_error("Set %s->%s ret = %d\n", SYS_FLIPTB, strValue, iRet);
    // bitrate
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.bitrate);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_BITRATE, strValue);
    ak_print_error("Set %s->%s ret = %d\n", SYS_BITRATE, strValue, iRet);

    // framerate
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.framerate);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_FRAMERATE, strValue);
	ak_print_error("Set %s->%s ret = %d\n", SYS_FRAMERATE, strValue, iRet);
    // gop
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.gop);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_GOPLEN, strValue);
    ak_print_error("Set %s->%s ret = %d\n", SYS_GOPLEN, strValue, iRet);

    // minqp
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.minqp);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_VIMINQP, strValue);
	ak_print_error("Set %s->%s ret = %d\n", SYS_VIMINQP, strValue, iRet);
    // maxqp
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.maxqp);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_VIMAXQP, strValue);
    ak_print_error("Set %s->%s ret = %d\n", SYS_VIMAXQP, strValue, iRet);

    // width
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.width);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_RESWIDTH, strValue);
	ak_print_error("Set %s->%s ret = %d\n", SYS_RESWIDTH, strValue, iRet);
    // height
    memset(strValue, 0x00, sizeof(strValue));
    sprintf(strValue, "%s", sCfgDoorbellInt.height);
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_RESHEIGHT, sCfgDoorbellInt.height);
    ak_print_error("Set %s->%s ret = %d\n", SYS_RESHEIGHT, sCfgDoorbellInt.height, iRet);

    // aes key
    iRet = ak_ini_set_item_value(cfg_handle, DOORBELL_CFG_NAME, SYS_AESKEY, sCfgDoorbellInt.key);
    ak_print_error("Set %s->%s ret = %d\n", SYS_AESKEY, sCfgDoorbellInt.key, iRet);

    ak_ini_destroy(cfg_handle);	

    printf("-----------------------------------------------------\n");
    printf("Load config to confirm\n");
    printf("-----------------------------------------------------\r\n");
    test_confirm_write_cfg();

    return iRet;
}

/*---------------------------------------------------------------------------*/
/*                           FACTORY CONFIG                                  */
/*---------------------------------------------------------------------------*/
typedef struct factory_cfg{
    char key[CFG_VALUE_BUF_SIZE];
    char value[CFG_VALUE_BUF_SIZE];
}FACTORY_CFG_t;

static FACTORY_CFG_t g_fac_cfg_table[] =
{
    {FACTORY_ITEM_SERI, },
    {FACTORY_ITEM_DATECODE, }
};

// static void *g_factory_cfg_handle = NULL;
// /*
// * Function Name  : dbcfg_init_factory_config
// * Description    : Open factory config partition
// * Input          : [uint32_t]size 
// * Output         : None
// * Return         : 0 successful
// *                : -1 failure
// */
// int dbcfg_init_factory_config(void)
// {
//     int iRet = 0;
//     g_factory_cfg_handle = ak_ini_init("APPINI");
//     if(NULL == g_factory_cfg_handle)
//     {
//         ak_print_error_ex("open config file failed.\n");
// 		iRet = -1; //exit
//     } 
//     else
//     {
// 		ak_print_normal("Factory config open successful.\r\n");	
// 	}
//     return iRet;
// }

// /*
// * Function Name  : dbcfg_deinit_factory_config
// * Description    : Close factory config partition
// * Input          : [uint32_t]size 
// * Output         : None
// * Return         : 0 successful
// *                : -1 failure
// */
// int dbcfg_deinit_factory_config(void)
// {
//     int iRet = 0;
//     if(g_factory_cfg_handle)
//     {
// 	    iRet = ak_ini_destroy(g_factory_cfg_handle);
//     }
//     return iRet;
// }

// /*
// * Function Name  : dbcfg_write_serial_number
// * Description    : Set a string serial number to config from factory
// * Input          : [char *] string serial number
// * Input          : [unsigned char] length of string
// * Output         : None
// * Return         : 0 successful
// *                : -1 failure
// */
// int dbcfg_write_serial_number(char *pSn, unsigned char ucLen)
// {
//     int iRet = 0;
//     void *fac_handle = NULL;
//     if(pSn == NULL || ucLen <= 0)
//     {
//         ak_print_error_ex("Parameter is invalid\n");
// 		iRet = -1; //exit
//     }

//     // open INI
//     fac_handle = ak_ini_init("APPINI");
//     if(NULL == g_factory_cfg_handle)
//     {
//         ak_print_error_ex("open config file failed.\n");
// 		iRet = -1; //exit
//     } 
//     else
//     {
//         // ak_print_normal("Factory config open successful.\r\n");	    
//         // write to config
//         iRet = ak_ini_set_item_value(fac_handle, \
//             SYS_FACTORY_CFG_NAME, FACTORY_ITEM_SERI, pSn);
//         // will comment this code for ftest
//         if(iRet != 0)
//         {
//             ak_print_error_ex("Set %s->%s: %d failed!\n", \
//                 SYS_FACTORY_CFG_NAME, FACTORY_ITEM_SERI, pSn);
//         }
//         else
//         {
//             ak_print_normal("Set %s->%s: %d OK\n", \
//                 SYS_FACTORY_CFG_NAME, FACTORY_ITEM_SERI, pSn);
//         }

//         // close INI
//         ak_ini_destroy(fac_handle);
        
//     }
//     return iRet;
// }

// /*
// * Function Name  : dbcfg_read_serial_number
// * Description    : Get a string serial number to config from factory
// * Input          : [char *] string serial number
// * Input          : [unsigned char] length of string
// * Output         : None
// * Return         : 0 successful
// *                : -1 failure
// */
// int dbcfg_read_serial_number(char *pSn, unsigned char *pucLen)
// {
//     int iRet = 0;
//     void *fac_handle = NULL;
//     if(pSn == NULL || pucLen == NULL)
//     {
//         ak_print_error_ex("Parameter is invalid\n");
// 		iRet = -1; //exit
//     }

//     // open INI
//     fac_handle = ak_ini_init("APPINI");
//     if(NULL == g_factory_cfg_handle)
//     {
//         ak_print_error_ex("open config file failed.\n");
// 		iRet = -1; //exit
//     } 
//     else
//     {
//         // ak_print_normal("Factory config open successful.\r\n");	    
//         // write to config
//         iRet = ak_ini_get_item_value(fac_handle, \
//             SYS_FACTORY_CFG_NAME, FACTORY_ITEM_SERI, pSn);
//         // will comment this code for ftest
//         if(iRet != 0)
//         {
//             ak_print_error_ex("Get %s->%s (%d) failed!\n", \
//                 SYS_FACTORY_CFG_NAME, FACTORY_ITEM_SERI, iRet);
//         }
//         else
//         {
//             ak_print_normal("Get %s->%s: %s OK\n", \
//                 SYS_FACTORY_CFG_NAME, FACTORY_ITEM_SERI, pSn);
//         }

//         // close INI
//         ak_ini_destroy(fac_handle);
        
//     }
//     return iRet;
// }

// /*
// * Function Name  : dbcfg_write_datecode
// * Description    : Set a datecode to config from factory
// * Input          : [char *] string datecode YYYYMMDD
// * Output         : None
// * Return         : 0 successful
// *                : -1 failure
// */
// int dbcfg_write_datecode(char *pDatecode)
// {
//     int iRet = 0;
    
//     return iRet;
// }

// /*
// * Function Name  : dbcfg_read_datecode
// * Description    : Get a datecode to config from factory
// * Input          : [char *] string serial number
// * Output         : None
// * Return         : 0 successful
// *                : -1 failure
// */
// int dbcfg_read_datecode(char *pDatecode)
// {
//     int iRet = 0;
    
//     return iRet;
// }

/*
* Function Name  : dbcfg_write_string_to_factory_config
* Description    : Set a string to config from factory
* Input          : [char *] key item
* Input          : [char *] string serial number
* Input          : [unsigned char] length of string
* Output         : None
* Return         : 0 successful
*                : -1 failure
*/
int dbcfg_write_string_to_factory_config(char *pItem, char *pSn, unsigned char ucLen)
{
    int iRet = 0;
    void *fac_handle = NULL;
    if(pItem == NULL || pSn == NULL || ucLen <= 0)
    {
        ak_print_error_ex("Parameter is invalid\n");
		iRet = -1; //exit
    }

    // open INI
    fac_handle = ak_ini_init("APPINI");
    if(NULL == fac_handle)
    {
        ak_print_error_ex("open config file failed.\n");
		iRet = -1; //exit
    } 
    else
    {
        // ak_print_normal("Factory config open successful.\r\n");	    
        // write to config
        iRet = ak_ini_set_item_value(fac_handle, \
            SYS_FACTORY_CFG_NAME, pItem, pSn);
        // will comment this code for ftest
        if(iRet != 0)
        {
            ak_print_error_ex("Set %s->%s: %d failed!\n", \
                SYS_FACTORY_CFG_NAME, pItem, pSn);
        }
        else
        {
            ak_print_normal("Set %s->%s: %d OK\n", \
                SYS_FACTORY_CFG_NAME, pItem, pSn);
        }

        // close INI
        ak_ini_destroy(fac_handle);
        
    }
    return iRet;
}

/*
* Function Name  : dbcfg_read_string_to_factory_config
* Description    : Get a string serial number to config from factory
* Input          : [char *] string serial number
* Input          : [unsigned char] length of string
* Output         : None
* Return         : 0 successful
*                : -1 failure
*/
int dbcfg_read_string_to_factory_config(char *pItem, char *pSn, unsigned char *pucLen)
{
    int iRet = 0;
    void *fac_handle = NULL;
    if(pItem == NULL || pSn == NULL || pucLen == NULL)
    {
        ak_print_error_ex("Parameter is invalid\n");
		iRet = -1; //exit
    }

    // open INI
    fac_handle = ak_ini_init("APPINI");
    if(NULL == fac_handle)
    {
        ak_print_error_ex("open config file failed.\n");
		iRet = -1; //exit
    } 
    else
    {
        // ak_print_normal("Factory config open successful.\r\n");	    
        // write to config
        iRet = ak_ini_get_item_value(fac_handle, \
            SYS_FACTORY_CFG_NAME, pItem, pSn);
        // will comment this code for ftest
        if(iRet != 0)
        {
            ak_print_error_ex("Get %s->%s (%d) failed!\n", \
                SYS_FACTORY_CFG_NAME, pItem, iRet);
        }
        else
        {
            ak_print_normal("Get %s->%s: %s OK\n", \
                SYS_FACTORY_CFG_NAME, pItem, pSn);
            *pucLen = strlen(pSn);
        }
        // close INI
        ak_ini_destroy(fac_handle);
    }
    return iRet;
}

/*---------------------------------------------------------------------------*/
/*                           AGC CONFIG                                      */
/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/
