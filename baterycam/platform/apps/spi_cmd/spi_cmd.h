/**
  ******************************************************************************
  * File Name          : spi_cmd.h
  * Description        : This file contains configs of spi command
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_CMD_H__
#define __SPI_CMD_H__
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

typedef int (*T_pfSPICMD)(char *pCmd, char *pResp);

typedef struct spi_cmd_t
{
    unsigned char cmd_type;
    unsigned char cmd_id;
    // unsigned char isResp;
    // unsigned char reserver;
    T_pfSPICMD pfHandle;
}SpiCmd_t;

typedef struct spi_cmd_factory_test_T
{
    char ak_light;
    char tone_gen;
    char freq[2];
    char bitrate[2];
}SpiCmdFactory_t __attribute__((aligned(8)));

#define SPICMD_OFFSET_ID                  (0)
#define SPICMD_OFFSET_TYPE                (SPICMD_OFFSET_ID+1)
#define SPICMD_OFFSET_DATA                (SPICMD_OFFSET_ID+2)

#define SPIRESP_OFFSET_ID                 (0)
#define SPIRESP_OFFSET_DATA               (1)
#define SPIRESP_OFFSET_LEN_KEY            (SPIRESP_OFFSET_ID + 1)
#define SPIRESP_OFFSET_DATA_KEY            (SPIRESP_OFFSET_ID + 2)

#define SPI_RESOLUTION_720P               (0)
#define SPI_RESOLUTION_960P               (1)
#define SPI_RESOLUTION_1080P              (2)
#define SPI_RESOLUTION_480P               (3)

#define SPICMD_GETSTATUS_FACTORY_RESET      0x00
#define SPICMD_GETSTATUS_NOT_READY          0x01
#define SPICMD_GETSTATUS_READY              0x02
#define SPICMD_GETSTATUS_STREAMING          0x03

#define SPICMD_TYPE_UNKNOWN                 0x00
#define SPICMD_TYPE_GET                     0x01
#define SPICMD_TYPE_SET                     0x02

#define SPICMD_CONTROL_SELECT_FILE          0x01
#define SPICMD_CONTROL_FILE_VALIDATE        0x02
#define SPICMD_CONTROL_TRANSFER_START       0x03
#define SPICMD_CONTROL_TRANSFER_DONE        0x04
#define SPICMD_CONTROL_TRANSFER_STOP        0x05

#define SPICMD_CTRL_1_LEN_OFFSET            0x02
#define SPICMD_CTRL_1_DATA_OFFSET           (SPICMD_CTRL_1_LEN_OFFSET + 2)

#define SPICMD_CTRL_2_FILE_OK               0x01
#define SPICMD_CTRL_2_FILE_NOK              0x02

#define SPICMD_SPK_GAIN_OFFSET              (1)
#define SPICMD_MIC_GAIN_OFFSET              (SPICMD_SPK_GAIN_OFFSET + 1)
#define SPICMD_ECHO_OFFSET                  (SPICMD_MIC_GAIN_OFFSET + 1)
#define SPICMD_AGCNR_OFFSET                 (SPICMD_ECHO_OFFSET + 1)

#define SPICMD_DINGDONG_OPTION_OFFSET       1
#define SPICMD_DINGDONG_OPTION_DINGDONG     0
#define SPICMD_DINGDONG_OPTION_SNAPSHOT     0xA0
#define SPICMD_DINGDONG_OPTA0_LEN_OFFSET    2
#define SPICMD_DINGDONG_OPTA0_DATA_OFFSET   4
/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
int spicmd_process_getversion(char *pCmd, char *pResp);
int spicmd_process_getudid(char *pCmd, char *pResp);
int spicmd_process_getstatus(char *pCmd, char *pResp);
int spicmd_process_getmediapacket0x85(char *pCmd, char *pResp);
int spicmd_process_cmd_unknown(char *pCmd, char *pResp);

int spicmd_process_flicker(char *pCmd, char *pResp);
int spicmd_process_flip(char *pCmd, char *pResp);
int spicmd_process_bitrate(char *pCmd, char *pResp);
int spicmd_process_framerate(char *pCmd, char *pResp);
int spicmd_process_resolution(char *pCmd, char *pResp);
int spicmd_process_aeskey(char *pCmd, char *pResp);

int spicmd_process_getcds(char *pCmd, char *pResp);
int spicmd_process_factory_test(char *pCmd, char *pResp);
int spicmd_process_ntp(char *pCmd, char *pResp);
int spicmd_upgrade_getversion(char *pCmd, char *pResp);
int spicmd_play_dingdong(char *pCmd, char *pResp);

int spicmd_process_udid(char *pCmd, char *pResp);
int spicmd_process_token(char *pCmd, char *pResp);
int spicmd_process_url(char *pCmd, char *pResp);
int spicmd_process_filetransfer(char *pCmd, char *pResp);
int spicmd_process_voice(char *pCmd, char *pResp);

int spicmd_process_volume(char *pCmd, char *pResp);

void *spi_command_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CMD_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

