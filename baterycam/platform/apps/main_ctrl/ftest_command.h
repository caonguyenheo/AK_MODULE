/**
  ******************************************************************************
  * File Name          : ftest_command.h
  * Description        : This file contains all config factory test command 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FTEST_COMMAND_H__
#define __FTEST_COMMAND_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

typedef void (*cli_cmd_func) (int, char ** );

typedef struct CLICMD
{
        const char           *cmd;
        cli_cmd_func          handler;
        const char           *cmd_usage;
        
} cli_cmds;

#define M_PI          (3.14)

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
void ftest_audioloop_stop(void);
void ftest_audioloop_start(void);
int ftest_audioloop_get_mode_stop(void);

// int ftest_cmd_audio_loopback(int argc, char **argv);
// void* ftest_audio_loopback_thread(void *arg);
void* ftest_speaker_thread(void *arg);

void* ftest_audio_loopback_thread(void *arg);
void *ftest_test_structure_thread(void *arg);
void* ftest_speaker_tone_generate_thread(void *arg);

void tone_gen_set_on(void);
void tone_gen_set_off(void);
int tone_gen_get_running(void);

void *ftest_test_ntp_thread(void *arg);

int ftest_speaker_set_volume_onflight_with_value(int iValue);
int ftest_mic_set_volume_onflight_with_value(int iValue);

void atecmdled_set_on(void);
void atecmdled_set_off(void);
void* ftest_led_thread(void *arg);

void* ftest_cmd_uvc_test(void *arg);
void videouvc_set_enable(void);
void videouvc_set_disable(void);
int videouvc_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* __FTEST_COMMAND_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

