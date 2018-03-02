/**
  ******************************************************************************
  * File Name          : ftest.h
  * Description        : This file contains all config factory test 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FTEST_H__
#define __FTEST_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 
// #include "ak_apps_config.h"
/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
#define   FTEST_CMD_HELP                                  1
#define   FTEST_CMD_VERSION                               2
#define   FTEST_CMD_AUDIO_LOOPBACK_ON                     3
#define   FTEST_CMD_AUDIO_LOOPBACK_OFF                    4
#define   FTEST_CMD_REBOOT                                5
#define   FTEST_CMD_SPEAKER                               6
#define   FTEST_CMD_FM2018                                7

#define   FTEST_PREFIX_CMD                                "atecmd"
#define   FTEST_LEN_BUF_CONSOLE                           128

typedef enum e_fcmd_id{
    E_FCMD_VERSION = 0,
    E_FCMD_HELP,
    E_FCMD_CPU,
    E_FCMD_PLAY_BEEP,
    E_FCMD_AUDIO_LOOPBACK,
    // E_FCMD_TEST,
    E_FCMD_REBOOT,
    E_FCMD_RESET,
    E_FCMD_NTP,
    E_FCMD_CONFIG,
// #ifdef DOORBELL_TINKLE820
    E_FCMD_FM,
    E_FCMD_FM_SET,
    E_FCMD_FM_GET,
// #endif
    E_FCMD_I2C_WRITE,
    E_FCMD_I2C_READ,
    /* command support set mic and speak gain */
    E_FCMD_MICDIGITAL,
    E_FCMD_MICANALOG,
    E_FCMD_SPKDIGITAL,
    E_FCMD_SPKANALOG,
    E_FCMD_SDCARD_DETECT,
    E_FCMD_LED,
    E_FCMD_TEST_SPEAKER,
    E_FCMD_WRITESN,
    E_FCMD_READSN,
    E_FCMD_DATECODE,
    E_FCMD_UVC,
    E_FCMD_CDSVALUE,
    E_FCMD_AGC,
    E_FCMD_MAX
}E_FCMD_ID_t;

typedef void (*func_handle_t)(char *arg, int length);

typedef struct ftest_cmd{
    char name[32];
    unsigned short cmdid;
    char arg[80];
    func_handle_t   cmd_handle;
    char description[80];
}FTEST_CMD_t;



/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
void* ftest_main_process(void *arg);

char ftest_uvc_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* __FTEST_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

