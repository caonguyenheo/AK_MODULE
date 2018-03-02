/**
  ******************************************************************************
  * File Name          : config_model.h
  * Description        : 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 PREMIELINK PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CONFIG_MODEL_H__
#define __CONFIG_MODEL_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/


/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
#define DOORBELL_TINKLE820                    (1)
extern void *g_camera_handle;
extern void *g_venc_handle_tinkle;
#define BITRATE_TRUE_MAX_KBPS (1100)
#define BITRATE_SET_MAX_KBPS (800)
#define BITRATE_SET_MIN_KBPS (300)
#define BITRATE_CHANGE_STEP_KBPS (100)
extern int g_bitrateadaptive_status;
extern int g_total_frame_count;
/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/

extern unsigned short g_exposure;

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_MODEL_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

