/**
  ******************************************************************************
  * File Name          : sdcard.h
  * Description        :
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SDCARD_H__
#define __SDCARD_H__
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
typedef enum sdcard_state
{
    E_SDCARD_STATUS_NO_DETECT = 0,
    E_SDCARD_STATUS_DETECTED,
    E_SDCARD_STATUS_MOUNTED,
    E_SDCARD_STATUS_UMOUNT,
    E_SDCARD_STATUS_MAX
}E_SDCARD_STATE_t;

// #define SDCARD_STATUS_NO_DETECT         (0)
// #define SDCARD_STATUS_DETECTED          (1)
// #define SDCARD_STATUS_MOUNTED           (2)
// #define SDCARD_STATUS_UMOUNT            (3)

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
int sdcard_mount(void);
int sdcard_umount(void);

void* sdcard_main_process(void *arg);
E_SDCARD_STATE_t sdcard_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* __SDCARD_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

