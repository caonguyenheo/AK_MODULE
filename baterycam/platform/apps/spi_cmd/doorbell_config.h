/**
  ******************************************************************************
  * File Name          : doorbell_config.h
  * Description        : This file contains prototype read/write config doorbell
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DOORBELL_CONFIG_H__
#define __DOORBELL_CONFIG_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 
#include "ak_apps_config.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/


#define FACTORY_CFG_NAME                   "factory"

#define DOORBELL_CFG_NAME                   "doorbell"
// #define CFG_VALUE_BUF_SIZE                  32
#define CFG_VALUE_BUF_SIZE                  SYS_MAX_LENGTH_KEY

#define SYS_FLICKER             "flicker"
#define SYS_FLIPUD              "flip_ud"
#define SYS_FLIPTB              "flip_tb"
#define SYS_BITRATE             "db_bitrate"
#define SYS_FRAMERATE           "framerate"
#define SYS_GOPLEN              "gop_len"
#define SYS_VIMINQP             "vi_minqp"
#define SYS_VIMAXQP             "vi_maxqp"
#define SYS_RESWIDTH            "res_width"
#define SYS_RESHEIGHT           "res_height"
#define SYS_AESKEY              "aes_key"
#define SYS_UDID                "udid"
#define SYS_TOKEN               "token"         // 12
#define SYS_URL                 "url"         // 13
#define SYS_RND                 "rnd"
#define SYS_SIP                 "sip"
#define SYS_SP                  "sp"
#define SYS_CDS_ON              "cds_on"
#define SYS_CDS_OFF             "cds_off"
#define SYS_MICD_GAIN           "micd_gain"
#define SYS_SPKD_GAIN           "spkd_gain"
#define SYS_MICA_GAIN           "mica_gain"
#define SYS_SPKA_GAIN           "spka_gain"
#define SYS_ECHO                "echo"
#define SYS_AGC_NR              "agc_nr"
#define SYS_MAX_KEY              25

#define SYS_FLICKER_MAX             60
#define SYS_FLICKER_MIN             50

#define SYS_FLIPUPDOWN_MAX          1
#define SYS_FLIPUPDOWN_MIN          0
#define SYS_FLIPTOPBOTTOM_MAX       1
#define SYS_FLIPTOPBOTTOM_MIN       0

#define SYS_BITRATE_MAX             2000   // kbps
#define SYS_BITRATE_MIN             125    // kbps

#define SYS_FRAMERATE_MAX           25   // fps
#define SYS_FRAMERATE_MIN           0    // fps

#define SYS_GOPLEN_MAX              50
#define SYS_GOPLEN_MIN              0

#define SYS_QP_MAX                  51
#define SYS_QP_MIN                  20

#define SYS_RESWIDTH_MAX            1920
#define SYS_RESWIDTH_MIN            480
#define SYS_RESHEIGHT_MAX           1080
#define SYS_RESHEIGHT_MIN           360

#define SYS_MICSPK_DIGITAL_MAX          12
#define SYS_MICSPK_DIGITAL_MIN          8
#define SYS_MICSPK_ANALOG_MAX          7
#define SYS_MICSPK_ANALOG_MIN          0

typedef struct cfg_doorbell{
    char key[CFG_VALUE_BUF_SIZE];
    char value[CFG_VALUE_BUF_SIZE];
}CFG_DOORBELL_t;

typedef struct cfg_doorbell_int{
    int flicker;
    int flip_updown;
    int flip_topbot;
    int bitrate;
    int framerate;
    int gop;
    int minqp;
    int maxqp;
    int width;
    int height;
    char key[CFG_VALUE_BUF_SIZE];
    char udid[CFG_VALUE_BUF_SIZE];
    char token[CFG_VALUE_BUF_SIZE];
    char url[CFG_VALUE_BUF_SIZE];
    char rnd[CFG_VALUE_BUF_SIZE];
    char sip[CFG_VALUE_BUF_SIZE];
    char sp[CFG_VALUE_BUF_SIZE];
    int cds_on;
    int cds_off;
    int micd_gain;
    int spkd_gain;
    int mica_gain;
    int spka_gain;
    int echo;
    int agc_nr;
}CFG_DOORBELL_INT_t;




/* config for factory */
#define SYS_FACTORY_CFG_NAME                "factory"
#define FACTORY_ITEM_SERI                   "sn"
#define FACTORY_ITEM_DATECODE               "datecode"


/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
void doorbell_config_init_ini(void);
/* Flicker */
int doorbell_cfg_set_flicker(int iValue);
int doorbell_cfg_get_flicker(int *pValue);
/* Flip_ud */
int doorbell_cfg_set_flipupdown(int iValue);
int doorbell_cfg_get_flipupdown(int *pValue);
/* Flip_tb */
int doorbell_cfg_set_fliptopbottom(int iValue);
int doorbell_cfg_get_fliptopbottom(int *pValue);
/* Bitrate */
int doorbell_cfg_set_bitrate(int iValue);
int doorbell_cfg_get_bitrate(int *pValue);
/* Framerate */
int doorbell_cfg_set_framerate(int iValue);
int doorbell_cfg_get_framerate(int *pValue);
/* Gop_len */
int doorbell_cfg_set_goplen(int iValue);
int doorbell_cfg_get_goplen(int *pValue);
/* Minqp */
int doorbell_cfg_set_minqp(int iValue);
int doorbell_cfg_get_minqp(int *pValue);
/* Maxqp */
int doorbell_cfg_set_maxqp(int iValue);
int doorbell_cfg_get_maxqp(int *pValue);
/* Resolution Width */
int doorbell_cfg_set_reswidth(int iValue);
int doorbell_cfg_get_reswidth(int *pValue);
/* Resolution Height */
int doorbell_cfg_set_resheight(int iValue);
int doorbell_cfg_get_resheight(int *pValue);
/* Aes Key with 16 byte */
int doorbell_cfg_set_aeskey(char *pKey);
int doorbell_cfg_set_psrnd(char *pRnd);
int doorbell_cfg_set_pssip(char *pSip);
int doorbell_cfg_set_pssp(char *pSp);
int doorbell_cfg_get_aeskey(char *pKey);
int doorbell_cfg_get_psrnd(char *pRnd);
int doorbell_cfg_get_pssip(char *pSip);
int doorbell_cfg_get_pssp(char *pSp);

/* udid with 24 byte */
int doorbell_cfg_set_udid(char *pUdid);
int doorbell_cfg_get_udid(char *pUdid);

/* token with 32 byte */
int doorbell_cfg_set_token(char *pToken);
int doorbell_cfg_get_token(char *pToken);

int doorbell_cfg_set_url(char *pUrl);
int doorbell_cfg_get_url(char *pUrl);

int doorbell_cfg_get_macaddress(char *pMAC);

int doorbell_cfg_set_cds_on(int iValue);
int doorbell_cfg_get_cds_on(int *pValue);
int doorbell_cfg_set_cds_off(int iValue);
int doorbell_cfg_get_cds_off(int *pValue);

int doorbell_cfg_set_micd_gain(int iValue);
int doorbell_cfg_get_micd_gain(int *pValue);
int doorbell_cfg_set_spkd_gain(int iValue);
int doorbell_cfg_get_spkd_gain(int *pValue);
int doorbell_cfg_set_mica_gain(int iValue);
int doorbell_cfg_get_mica_gain(int *pValue);
int doorbell_cfg_set_spka_gain(int iValue);
int doorbell_cfg_get_spka_gain(int *pValue);

int doorbell_cfg_set_echocancellation(int iValue);
int doorbell_cfg_get_echocancellation(int *pValue);

int doorbell_cfg_set_agcnr(int iValue);
int doorbell_cfg_get_agcnr(int *pValue);

int doorbell_load_config(CFG_DOORBELL_INT_t *pCfgDoorbellInt);
void doorbell_config_test_api1(void);

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
int dbcfg_write_string_to_factory_config(char *pItem, char *pSn, unsigned char ucLen);
/*
* Function Name  : dbcfg_read_string_to_factory_config
* Description    : Get a string serial number to config from factory
* Input          : [char *] string datecode
* Input          : [unsigned char] length of string
* Output         : None
* Return         : 0 successful
*                : -1 failure
*/
int dbcfg_read_string_to_factory_config(char *pItem, char *pSn, unsigned char *pucLen);

#ifdef __cplusplus
}
#endif

#endif /* __DOORBELL_CONFIG_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

