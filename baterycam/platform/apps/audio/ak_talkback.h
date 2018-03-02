/**
  ******************************************************************************
  * File Name          : ak_talkback.h
  * Description        : This file contains all prototype ak_talkback
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AK_TALKBACK_H__
#define __AK_TALKBACK_H__
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

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
void *ak_talkback_task(void *arg);


int talkback_init(void **pp_ao_handler);

void *ak_talkback_fast_play_task(void *arg);

int talkback_init_engine(void);
int talkback_deinit_engine(void);
int talkback_process_data_spi(char *pcData, int length);
void prepare_data_play_dingdong(void);

int talkback_ao_set_volume(void *pAoHandle);

int talkback_set_volume_onflight(void);
int talkback_set_volume_onflight_with_value(int iValue);

int aktb_backup_data_audio(char *pData, int len);


#ifdef __cplusplus
}
#endif

#endif /* __AK_TALKBACK_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

