/**
  ******************************************************************************
  * File Name          : spi_cmd.c
  * Description        : This file will process command which AK received from 
  *						CC3200 over SPI
  ******************************************************************************
  *
  * COPYRIGHT(c) 201 NXCOMM PTE LTD
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
#include "doorbell_config.h"

#include "led_cds.h"

#include "main_ctrl.h"
#include "ak_common.h"
#include <stdint.h>
#include <stdlib.h>

#include "sdcard.h"

#include "ak_talkback.h"
#include "ak_audio.h"
#include "config_model.h"
#include "hal_camera.h"

#include "ftest.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/

int g_play_dingdong = 0;
int g_SpiCmdSnapshot = 0;

extern int g_iDoneSnapshot;
extern int g_isDoneUpload;
extern int g_sd_update;
//ak_sem_t  sem_SpiCmdSnapShot;

// static unsigned long ulTickLastCmdDing = 0;

ak_sem_t  sem_SpiCmdSnapShot;

/* semaphore trigger file transfer */
ak_sem_t  sem_SpiCmdStartTransfer;
ak_sem_t  sem_SpiCmdStopTransfer;
int g_iP2pSdStreamStart = 0;
void *g_pvFile = NULL;

extern char g_acDirName[256];
extern char g_acDirName_Clip[256];

char g_SpiCmdFileStream[256];

extern char fw_version[];

int g_ntp_updated = 0;

static bool g_bValid = false;

// int g_speaker_volume_updated = 0;
// int g_mic_volume_updated = 0;
// char g_cSpeakerVolume = 0;
// char g_cMicVolume = 0;

const SpiCmd_t SPI_CMD_TABLE[] =
{
	{CMD_VERSION,				0x00, spicmd_process_getversion},
	{CMD_UDID,					0x00, spicmd_process_getudid},
	{CMD_STATUS,				0x00, spicmd_process_getstatus},
	{CMD_GET_PACKET,			0x00, spicmd_process_getmediapacket0x85},
	{CMD_GET_RESP,				0x00, spicmd_process_cmd_unknown},
	{CMD_REQUEST,				0x00, NULL},	// need to implement
	{CMD_RESPONSE,				0x00, NULL},	// need to implement
	{SPI_CMD_FLICKER,			0x00, spicmd_process_flicker},
	{SPI_CMD_FLIP,				0x00, spicmd_process_flip},
	{SPI_CMD_VIDEO_BITRATE,		0x00, spicmd_process_bitrate},
	{SPI_CMD_VIDEO_FRAMERATE,	0x00, spicmd_process_framerate},
	{SPI_CMD_VIDEO_RESOLUTION,	0x00, spicmd_process_resolution},
	{SPI_CMD_AES_KEY,			0x00, spicmd_process_aeskey},
	{SPI_CMD_GET_CDS,			0x00, spicmd_process_getcds},
	{SPI_CMD_MOTION_UPLOAD,		0x00, NULL},
	{SPI_CMD_NTP, 				0x00, spicmd_process_ntp},
	{SPI_UPGRADE_GET_VERSION, 	0x00, spicmd_upgrade_getversion},
	{SPI_CMD_PLAY_DINGDONG, 	0x00, spicmd_play_dingdong},
	{SPI_CMD_UDID, 				0x00, spicmd_process_udid}, // UDID
	{SPI_CMD_TOKEN, 			0x00, spicmd_process_token}, // token
	{SPI_CMD_URL,	 			0x00, spicmd_process_url}, // url
	{SPI_CMD_FILE_TRANSFER,	 	0x00, spicmd_process_filetransfer}, // file transfer
	{SPI_CMD_PLAY_VOICE,	 	0x00, spicmd_process_voice},
	//  command power off
	{SPI_CMD_VOLUME,	 		0x00, spicmd_process_volume},
	{SPI_CMD_FACTORY_TEST,	 	0x00, spicmd_process_factory_test},
	{0x00,						0x00, NULL},
};

/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/

void spi_cmd_dump_data(char *data)
{
	int i = 0;
	char acDump[1024] = {0x00, };
	memset(acDump, 0x00, sizeof(acDump));
	for(i = 0; i < 20; i++)
	{
		sprintf(acDump + strlen(acDump), "%02x ", *(data + i));
	}
	printf("%s!\n", acDump);
}
int spicmd_set_ntp(int value)
{
	int iRet = 0;
	long lRet = 0;
	struct ak_date systime;
	long local_second = 0;

	if(value > 0 && g_ntp_updated == 0)
	{
		iRet = ak_get_localdate(&systime);
		ak_print_error_ex("Current Local date: %04d %02d %02d %02d %02d %02d (ret %d)\n", \
			systime.year, systime.month, systime.day, \
			systime.hour, systime.minute, systime.second, iRet);
		if(iRet != 0)
		{
			printf("Error get localdate\r\n");
			iRet = -1;
		}
		else
		{
			local_second = ak_date_to_seconds(&systime);
			local_second += value;
			printf("localtime: %d\n", local_second);
			lRet = ak_seconds_to_date(local_second, &systime);
			if(lRet != 0)
			{
				printf("Error ak_seconds_to_date\r\n");
				iRet = -1;
			}
			else
			{
				iRet = ak_set_localdate(&systime);
				ak_print_error_ex("After Local date: %04d %02d %02d %02d %02d %02d (ret %d)\n", \
						systime.year, systime.month, systime.day, \
						systime.hour, systime.minute, systime.second, iRet);
				
				g_ntp_updated = 1;
			}
		}
	}
	else
	{
		if(g_ntp_updated == 1)
		{
			ak_print_error_ex("NTP was be set! value receive:%d\r\n", value);
			iRet = ak_get_localdate(&systime);
			ak_print_error_ex("Current Local date: %04d %02d %02d %02d %02d %02d (ret %d)\n", \
				systime.year, systime.month, systime.day, \
				systime.hour, systime.minute, systime.second, iRet);
		}
	}
	return iRet;
}



/*---------------------------------------------------------------------------*/
/*
AK send command and get response
*/
/* NOTE: This command doesn't need to send response to CC3200 */
int spicmd_process_getversion(char *pCmd, char *pResp)
{
	printf("CC3200 version: %d.%d.%d\n", *(pCmd + 1),*(pCmd + 2),*(pCmd + 3));
	// No response -> must return 0
	return 0;
}
/* NOTE: This command doesn't need to send response to CC3200 */
int spicmd_process_getudid(char *pCmd, char *pResp)
{
	char *pUdid = pCmd + 1;
	printf("UDID: %s\n", pUdid);
	// No response -> must return 0
	return 0;
}
/* NOTE: This command doesn't need to send response to CC3200 */
int spicmd_process_getstatus(char *pCmd, char *pResp)
{
	char cStatus = *(pCmd + 1);
	switch(cStatus)
	{
		case SPICMD_GETSTATUS_FACTORY_RESET:
			printf("CC Status: FACTORY RESET\n");
			break;
		case SPICMD_GETSTATUS_NOT_READY:
			printf("CC Status: NOT READY\n");
			break;
		case SPICMD_GETSTATUS_READY:
			printf("CC Status: RAEDY\n");
			break;
		case SPICMD_GETSTATUS_STREAMING:
			printf("CC Status: STREAMING\n");
			break;
		default:
			printf("Invalid CC status\n");
			break;
	}
	// No response -> must return 0
	return 0;
}
/*
AK send command and get response
*/
/* NOTE: This command doesn't need to send response to CC3200 */
int spicmd_process_getmediapacket0x85(char *pCmd, char *pResp)
{
	// Nothing to do
	return 0;
}
/*
AK send command and get response
*/
/* NOTE: This command doesn't need to send response to CC3200 */
int spicmd_process_cmd_unknown(char *pCmd, char *pResp)
{
	// Nothing to do
	return 0;
}

/*
Flicker
CC send command and get response
*/
int FlickerValue_update = 0, ck = 0;
int flip_update_bt = -1, flip_update_ud = -1;
extern int pwm_value;
int spicmd_process_flicker(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iFlickerValue = 0;
	int iFlipUD = 0;
	int iFlipLR = 0;
	int iBitrate = 0;
	int iFramerate = 0;
	int iResolutionHeight = 0;
	int iSpkGain = 0;
	int iMicGain = 0;
	int iGOP = 0;
	int ibitrateadaptive_status = 0;
	int iAgc = 0;
	int itemp;
	int light_value=0;
	//FlickerValue_update = 0;
	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			// read config and fill response
			iRet = doorbell_cfg_get_flicker(&iFlickerValue);
			if(iRet != 0)
			{
				ak_print_error_ex("Get flicker failed!\n");
				iFlickerValue = -1;
			}
			iRet = doorbell_cfg_get_flipupdown(&iFlipUD);
			if(iRet != 0)
			{
				ak_print_error_ex("Get flip updown failed!\n");
				iFlickerValue = -1;
			}
			iRet = doorbell_cfg_get_fliptopbottom(&iFlipLR);
			if(iRet != 0)
			{
				ak_print_error_ex("Get flip updown failed!\n");
				iFlickerValue = -1;
			}
			iRet = doorbell_cfg_get_bitrate(&iBitrate);
			if(iRet != 0)
			{
				ak_print_error_ex("Get bitrate failed!\n");
				iBitrate = -1;
			}
			iRet = doorbell_cfg_get_framerate(&iFramerate);
			if(iRet != 0)
			{
				ak_print_error_ex("Get framerate failed!\n");
				iFramerate = -1;
			}
			iRet = doorbell_cfg_get_resheight(&iResolutionHeight);
			if(iRet != 0)
			{
				ak_print_error_ex("Get RESOLUTION HEIGHT failed!\n");
				iResolutionHeight = -1;
			}
			// read spk gain
			iRet = doorbell_cfg_get_spka_gain(&iSpkGain);
			if(iRet != 0 || \
				(iSpkGain < SYS_MICSPK_ANALOG_MIN || iSpkGain > SYS_MICSPK_DIGITAL_MAX))
			{
				ak_print_error_ex("Get speaker gain failed!\n");
				iSpkGain = -1;
			}
			// read mic gain
			iRet = doorbell_cfg_get_mica_gain(&iMicGain);
			if(iRet != 0 || \
				(iMicGain < SYS_MICSPK_ANALOG_MIN || iMicGain > SYS_MICSPK_DIGITAL_MAX))
			{
				ak_print_error_ex("Get mic gain failed!\n");
				iMicGain = -1;
			}
			iRet = doorbell_cfg_get_goplen(&iGOP);
			if(iRet != 0)
			{
				ak_print_error_ex("Get GOP Len failed!\n");
				iGOP = -1;
			}
			// Read agc nr
			iRet = doorbell_cfg_get_agcnr(&iAgc);
			if(iRet != 0)
			{
				ak_print_error_ex("Get agc nr failed!\n");
				iAgc = -1;
			}
			*(pResp + SPIRESP_OFFSET_DATA) = (char)iFlickerValue & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +1) = (char)iFlipUD & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +2) = (char)iFlipLR & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +3) = (char)(iBitrate>>8) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +4) = (char)(iBitrate) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +5) = (char)(iFramerate) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +6) = (char)(iResolutionHeight>>8) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +7) = (char)(iResolutionHeight) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +8) = 0; //NA
			*(pResp + SPIRESP_OFFSET_DATA +9) = (char)(iSpkGain) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +10) = (char)(iMicGain) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +11) = (char)(iGOP) & 0xFF;
			*(pResp + SPIRESP_OFFSET_DATA +12) = 1; // Always full duplex in FW
			*(pResp + SPIRESP_OFFSET_DATA +13) = ibitrateadaptive_status;
			*(pResp + SPIRESP_OFFSET_DATA +14) = iAgc;
			break;
		case SPICMD_TYPE_SET:
			// write to config and send response
			iFlickerValue = *(pCmd + SPICMD_OFFSET_DATA);
			iFlipUD = *(pCmd + SPICMD_OFFSET_DATA+1);
			iFlipLR = *(pCmd + SPICMD_OFFSET_DATA+2);
			iBitrate = *(pCmd + SPICMD_OFFSET_DATA+3);
			iBitrate = (iBitrate<<8);
			iBitrate |= *(pCmd + SPICMD_OFFSET_DATA+4);
			iFramerate = *(pCmd + SPICMD_OFFSET_DATA+5);
			iResolutionHeight = *(pCmd + SPICMD_OFFSET_DATA+6);
			iResolutionHeight = (iResolutionHeight<<8);
			iResolutionHeight |= *(pCmd + SPICMD_OFFSET_DATA+7);
			iSpkGain = *(pCmd + SPICMD_OFFSET_DATA+9);
			iMicGain = *(pCmd + SPICMD_OFFSET_DATA+10);
			iGOP = *(pCmd + SPICMD_OFFSET_DATA+11);
			light_value = *(pCmd + SPICMD_OFFSET_DATA+16);
			pwm_value = light_value;
			led_update();
			
			// Audio duplex NA
			ibitrateadaptive_status = *(pCmd + SPICMD_OFFSET_DATA+13);
			iAgc = *(pCmd + SPICMD_OFFSET_DATA+14);
			// Check Flicker change
			iRet = 0;
			if(FlickerValue_update != iFlickerValue)
			{
				iRet = doorbell_cfg_set_flicker(iFlickerValue);
				FlickerValue_update = iFlickerValue;
				ck = 1;
				ak_print_normal_ex("FlickerValue_update_in:%d\r\n", FlickerValue_update);
			} else 
				ak_print_normal_ex("No Change to flicker\n");
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
			}
			ak_print_normal_ex("Set Flicker Value: 0x%x (%d) Byte: 0x%x\n", \
				iFlickerValue, iFlickerValue, *(pResp + SPIRESP_OFFSET_DATA));
			ak_sleep_ms(5);
			// Check Flip change
			iRet = 0;
			iFlipUD = *(pCmd + SPICMD_OFFSET_DATA + 1);
			doorbell_cfg_get_flipupdown(&itemp);
			ak_print_normal_ex("flip_update_ud_frist:%d\r\n", itemp);
			if(itemp != iFlipUD)
			{
				iRet = doorbell_cfg_set_flipupdown(iFlipUD);
				flip_update_ud = iFlipUD;
				ak_print_normal_ex("flip_update_up_down_in:%d\r\n", flip_update_ud);
				ak_print_normal_ex("iFlipUD_Set_up_down_in:%d\r\n", iFlipUD);
			}
			else
			{
				ak_print_normal_ex("flip_up_down no change\n");
			}
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA+1) = -1;
				ak_print_error_ex("Get flipupdown failed!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA+1) = iFlipUD;
			}
			ak_print_normal_ex("Set FlipUD Value: 0x%x (%d) Byte: 0x%x\n", \
				iFlipUD, iFlipUD, *(pResp + SPIRESP_OFFSET_DATA + 1));
			ak_sleep_ms(5);
			iRet = 0;
			iFlipLR = *(pCmd + SPICMD_OFFSET_DATA + 2);
			doorbell_cfg_get_fliptopbottom(&itemp);
			ak_print_normal_ex("flip_update_bt_frist:%d\r\n",itemp);
			if(itemp != iFlipLR)
			{
				iRet = doorbell_cfg_set_fliptopbottom(iFlipLR);
				flip_update_bt = iFlipLR;
				ak_print_normal_ex("flip_update_bottom_top_in:%d\r\n", flip_update_bt);
				ak_print_normal_ex("iFlipLR_bottom_top_in:%d\r\n", iFlipLR);
			}
			else
			{
				ak_print_normal_ex("flip LR no change\n");
			}
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA + 2) = -1;
				ak_print_error_ex("Get fliptopbottom failed!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA + 2) = iFlipLR;
			}
			ak_print_normal_ex("Set FlipLR Value: 0x%x (%d) Byte: 0x%x\n", \
				iFlipLR, iFlipLR, *(pResp + SPIRESP_OFFSET_DATA + 2));
			ak_sleep_ms(5);
			// Set bitrate
			iRet = doorbell_cfg_set_bitrate(iBitrate);
			if(iRet != 0)
			{
				// TODO: will fix :D
				*(pResp + SPIRESP_OFFSET_DATA + 3) = -1;
				*(pResp + SPIRESP_OFFSET_DATA + 4) = -1;
				ak_print_error_ex("Set value FAILED!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA + 3) = iBitrate>>8;
				*(pResp + SPIRESP_OFFSET_DATA + 4) = iBitrate&0xFF;
			}
			ak_print_normal_ex("Set Bitrate Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iBitrate, iBitrate, *(pResp + SPIRESP_OFFSET_DATA + 3), \
						*(pResp + SPIRESP_OFFSET_DATA + 4));
			ak_sleep_ms(5);
			// Framerate
			iRet = doorbell_cfg_set_framerate(iFramerate);
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA + 5) = -1;
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA + 5) = iFramerate;
			}
			ak_print_normal_ex("Set Framerate Value: 0x%x (%d) Byte: 0x%x\n", \
						iFramerate, iFramerate, *(pResp + SPIRESP_OFFSET_DATA + 5));
			ak_sleep_ms(5);
			// Resolution
			iRet = doorbell_cfg_set_resheight(iResolutionHeight);
			if(iRet != 0)
			{
				// TODO: will fix :D
				*(pResp + SPIRESP_OFFSET_DATA + 6) = -1;
				*(pResp + SPIRESP_OFFSET_DATA + 7) = -1;
				ak_print_error_ex("Set value FAILED!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA + 6) = iResolutionHeight>>8;
				*(pResp + SPIRESP_OFFSET_DATA + 7) = iResolutionHeight&0xff;
			}
			ak_print_normal_ex("Set Resolution Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iResolutionHeight, iResolutionHeight, *(pResp + SPIRESP_OFFSET_DATA + 6), \
						*(pResp + SPIRESP_OFFSET_DATA + 7));
			/*
			// Spk volume
			if(iSpkGain < SYS_MICSPK_ANALOG_MIN || iSpkGain > SYS_MICSPK_DIGITAL_MAX)
			{
				ak_print_error_ex("Value SPK GAIN invalid");
			}
			else
			{
				iRet = doorbell_cfg_set_spka_gain(iSpkGain);
				if(iRet != 0)
				{
					*(pResp + SPIRESP_OFFSET_DATA + 9) = -1;
					ak_print_error_ex("Set value SPK GAIN FAILED!(%d)\n", iRet);
				}
				else
				{
					*(pResp + SPIRESP_OFFSET_DATA + 9) = iSpkGain;
					ak_print_normal_ex("Set value SPK GAIN OK!\n");
					// g_speaker_volume = cValueSet;
					talkback_set_volume_onflight();
				}
				// TODO: set on flight
				// talkback_set_volume_onflight();
				talkback_set_volume_onflight_with_value(iSpkGain);
			}
			ak_print_normal_ex("Set SpkGain Value: 0x%x (%d) Byte: 0x%x\n", \
						iSpkGain, iSpkGain, *(pResp + SPIRESP_OFFSET_DATA + 9));
			ak_sleep_ms(5);
			// MIC volume
			if(iMicGain < SYS_MICSPK_ANALOG_MIN || iMicGain > SYS_MICSPK_DIGITAL_MAX)
			{
				ak_print_error_ex("Value MIC GAIN invalid");
			}
			else
			{
				iRet = doorbell_cfg_set_mica_gain(iMicGain);
				if(iRet != 0)
				{
					*(pResp + SPIRESP_OFFSET_DATA + 10) = -1;
					ak_print_error_ex("Set value MIC GAIN FAILED!(%d)\n", iRet);
				}
				else
				{
					*(pResp + SPIRESP_OFFSET_DATA + 10) = iMicGain;
					ak_print_normal_ex("Set value MIC GAIN OK!\n");
					// g_mic_volume = cValueSet;
					// g_volume_updated = 1;
					audio_set_volume_onflight();
				}
				// TODO: set on flight
				// audio_set_volume_onflight();
				audio_set_volume_onflight_with_value(iMicGain);
			}
			ak_sleep_ms(70);
			ak_print_normal_ex("Set MicGain Value: 0x%x (%d) Byte: 0x%x\n", \
						iMicGain, iMicGain, *(pResp + SPIRESP_OFFSET_DATA + 10));
			*/
			// Set GOP
			iRet = doorbell_cfg_set_goplen(iGOP);
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA + 11) = -1;
				ak_print_error_ex("Set value FAILED!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA + 11) = iGOP;
			}
			ak_print_normal_ex("Set GOP: 0x%x (%d) Byte: 0x%x\n", \
						iGOP, iGOP, *(pResp + SPIRESP_OFFSET_DATA + 11));
			// Set audio duplex -> NA
			// Set auto bitrate control option
			g_bitrateadaptive_status = ibitrateadaptive_status;
			*(pResp + SPIRESP_OFFSET_DATA + 13) = ibitrateadaptive_status;
			ak_print_normal_ex("Set BITRATELIMIT option %d\n", ibitrateadaptive_status);
			
			// agc nr
			iRet = doorbell_cfg_set_agcnr(iAgc);
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA + 14) = -1;
				ak_print_error_ex("Set Agc FAILED!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA + 14) = iGOP;
				ak_print_normal_ex("Set value AGC NR OK %d!\n", iAgc);
			} // TODO: set on flight
			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}
/*
Flip
CC send command and get response
*/
int spicmd_process_flip(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue1 = 0;
	int iValue2 = 0;
	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			// read config and fill response
			iRet = doorbell_cfg_get_flipupdown(&iValue1);
			if(iRet != 0)
			{
				ak_print_error_ex("Get flipupdown failed!\n");
				iValue1 = -1;
			}
			*(pResp + SPICMD_OFFSET_TYPE) = (char)iValue1 & 0xFF;

			iRet = doorbell_cfg_get_fliptopbottom(&iValue2);
			if(iRet != 0)
			{
				ak_print_error_ex("Get fliptopbottom failed!\n");
				iValue2 = -1;
			}
			*(pResp + SPICMD_OFFSET_TYPE + 1) = (char)iValue2 & 0xFF;
			break;

		case SPICMD_TYPE_SET:
			// write to config and send response
			iValue1 = *(pCmd + SPICMD_OFFSET_DATA);
			ak_print_normal_ex("\nflip_update_ud_value_frist:%d\r\n", flip_update_ud);
			doorbell_cfg_get_fliptopbottom(&flip_update_ud);
			if(flip_update_ud != iValue1)
			{
				iRet = doorbell_cfg_set_flipupdown(iValue1);
				flip_update_ud = iValue1;
				ak_print_normal_ex("\nflip_update_up_down_in:%d\r\n", flip_update_ud);
				ak_print_normal_ex("\niValue1_SET_updown:%d\r\n", iValue1);
			}
			else
			{
				ak_print_normal_ex("\nflip_up_down value Old\r\n");
			}
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
				ak_print_error_ex("Get flipupdown failed!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA) = iValue1;
			}
			iValue2 = *(pCmd + SPICMD_OFFSET_DATA + 1);
			ak_print_normal_ex("\nflip_update_bt_value_frist:%d\r\n", flip_update_bt);
			doorbell_cfg_get_fliptopbottom(&flip_update_bt);
			if(flip_update_bt != iValue2)
			{
				iRet = doorbell_cfg_set_fliptopbottom(iValue2);
				flip_update_bt = iValue2;
				ak_print_normal_ex("\nflip_update_bottom_top_in:%d\r\n", flip_update_bt);
				ak_print_normal_ex("\niValue2_SET_bottom_top:%d\r\n", iValue2);
			}
			else
			{
				ak_print_normal_ex("\nflip_bottom_top value Old\r\n");
			}
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA + 1) = -1;
				ak_print_error_ex("Get fliptopbottom failed!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA + 1) = iValue2;
			}

			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}

/*
Bitrate
CC send command and get response
*/
int spicmd_process_bitrate(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue = 0;

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			// read config and fill response
			iRet = doorbell_cfg_get_bitrate(&iValue);
			if(iRet != 0)
			{
				ak_print_error_ex("Get bitrate failed!\n");
				iValue = -1;
			}
			*(pResp + SPIRESP_OFFSET_DATA) = (iValue >>8) & 0xFF; // byte high
			*(pResp + SPIRESP_OFFSET_DATA + 1) = (iValue) & 0xFF; // byte low
			ak_print_normal_ex("Get Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA +1));
			break;
		case SPICMD_TYPE_SET:
			// write to config and send response
			iValue = (*(pCmd + SPICMD_OFFSET_DATA) << 8) & 0xFF00; // byte high
			iValue |= *(pCmd + SPICMD_OFFSET_DATA + 1) & 0xFF; // byte low

			iRet = doorbell_cfg_set_bitrate(iValue);
			if(iRet != 0)
			{
				// TODO: will fix :D
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
				*(pResp + SPIRESP_OFFSET_DATA + 1) = -1;
				ak_print_error_ex("Set value FAILED!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
				*(pResp + SPIRESP_OFFSET_DATA + 1) = *(pCmd + SPICMD_OFFSET_DATA + 1);
			}
			ak_print_normal_ex("Set Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA +1));
			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}

int spicmd_process_framerate(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue = 0;
	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			iRet = doorbell_cfg_get_framerate(&iValue);
			if(iRet != 0)
			{
				ak_print_error_ex("Get framerate failed!\n");
				iValue = -1;
			}
			*(pResp + SPIRESP_OFFSET_DATA) = (char)iValue & 0xFF;
			break;
		case SPICMD_TYPE_SET:
			// write to config and send response
			iValue = *(pCmd + SPICMD_OFFSET_DATA);
			iRet = doorbell_cfg_set_framerate(iValue);
			if(iRet != 0)
			{
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
			}
			ak_print_normal_ex("Set Value: 0x%x (%d) Byte: 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA));
			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}



/*
Resolution
CC send command and get response
*/
int spicmd_process_resolution(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue = 0;

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			// read config and fill response
			iRet = doorbell_cfg_get_resheight(&iValue);
			if(iRet != 0)
			{
				ak_print_error_ex("Get RESOLUTION HEIGHT failed!\n");
				iValue = -1;
			}
			*(pResp + SPIRESP_OFFSET_DATA) = (iValue >>8) & 0xFF; // byte high
			*(pResp + SPIRESP_OFFSET_DATA + 1) = (iValue) & 0xFF; // byte low
			ak_print_normal_ex("Get Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iValue, iValue,*(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA +1));
			break;
		case SPICMD_TYPE_SET:
			// write to config and send response
			iValue = (*(pCmd + SPICMD_OFFSET_DATA) << 8) & 0xFF00; // byte high
			iValue |= *(pCmd + SPICMD_OFFSET_DATA + 1) & 0xFF; // byte low
			ak_print_normal_ex("Set Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA +1));

			iRet = doorbell_cfg_set_resheight(iValue);
			if(iRet != 0)
			{
				// TODO: will fix :D
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
				*(pResp + SPIRESP_OFFSET_DATA + 1) = -1;
				ak_print_error_ex("Set value FAILED!\n");
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
				*(pResp + SPIRESP_OFFSET_DATA + 1) = *(pCmd + SPICMD_OFFSET_DATA + 1);
			}
			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}


/*
AES key
CC send command and get response
*/
int spicmd_process_aeskey(char *pCmd, char *pResp)
{
	int iRet = 0;
	int i = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iLenKey = 0, iLenRnd = 0, iLenSip = 0, iLenSp = 0;
	int iValue = 0;
	char aes_key[17] = {0x00, };
	char ps_rnd[13] = {0x00, };
	char ps_sip[16] = {0x00, };
	char ps_sp[8] = {0x00, };

	char pDebug[256] = {0x00, };
	char *pKey = NULL, *pRnd = NULL, *pSip = NULL, *pSp = NULL;
	char *pKey_res = NULL, *pRnd_res = NULL, *pSip_res = NULL, *pSp_res = NULL;
	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	iLenKey = *(pCmd + SPICMD_OFFSET_DATA);
	iLenRnd = *(pCmd + SPICMD_OFFSET_DATA + 1 + iLenKey);
	iLenSip = *(pCmd + SPICMD_OFFSET_DATA + 2 + iLenKey + iLenRnd);
	iLenSp = *(pCmd + SPICMD_OFFSET_DATA + 3 + iLenKey + iLenRnd + iLenSip);
	pKey = pCmd + SPICMD_OFFSET_DATA + 1;
	pRnd = pKey + iLenKey + 1;
	pSip = pRnd + iLenRnd + 1;
	pSp = pSip + iLenSip + 1;
	
	pKey_res = pResp + SPIRESP_OFFSET_DATA + 1;
	pRnd_res = pKey_res + iLenKey + 1;
	pSip_res = pRnd_res + iLenRnd + 1;
	pSp_res = pSip_res + iLenSip + 1;
	
	ak_print_normal_ex("iLenKey: %d\n", iLenKey);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	*(pKey_res-1) = *(pKey-1);
	*(pRnd_res-1) = *(pRnd-1);
	*(pSip_res-1) = *(pSip-1);
	*(pSp_res-1) = *(pSp-1);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			// read config and fill response
			iRet = doorbell_cfg_get_aeskey(aes_key);
			if(iRet != 0)
			{
				ak_print_error_ex("Get aes_key failed!\n");
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_DATA) = 16;
				pKey = pResp + SPIRESP_OFFSET_DATA + 1;
				memcpy(pKey, aes_key, 16);
				ak_print_normal_ex("Get key: %s\n", pKey);
			}
			break;
		case SPICMD_TYPE_SET:
			// write to config and send response
			if(iLenKey != 16)
			{
				ak_print_error_ex("Length of Key is invalid\n");
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
			}
			else
			{
				//pKey = pCmd + SPICMD_OFFSET_DATA + 1;
				//*(pKey + 16) = '\0';
				// Key copy
				memset(aes_key,'\0',sizeof(aes_key));
				for(i=0; i < iLenKey; i++)
				{
					aes_key[i] = *(pKey + i);
					sprintf(pDebug + strlen(pDebug), "%02X ", aes_key[i]);
				}
				ak_print_normal_ex("Key: %s\n", pDebug);
				
				// Rnd copy
				memset(ps_rnd,'\0',sizeof(ps_rnd));
				for(i=0; i < iLenRnd; i++)
				{
					ps_rnd[i] = *(pRnd + i);
					sprintf(pDebug + strlen(pDebug), "%02X ", ps_rnd[i]);
				}
				ak_print_normal_ex("Rnd: %s\n", pDebug);
				
				// Sip copy
				memset(ps_sip,'\0',sizeof(ps_sip));
				for(i=0; i < iLenSip; i++)
				{
					ps_sip[i] = *(pSip + i);
					sprintf(pDebug + strlen(pDebug), "%02X ", ps_sip[i]);
				}
				ak_print_normal_ex("Sip: %s\n", pDebug);
				
				// Sp copy
				memset(ps_sp,'\0',sizeof(ps_sp));
				for(i=0; i < iLenSp; i++)
				{
					ps_sp[i] = *(pSp + i);
					sprintf(pDebug + strlen(pDebug), "%02X ", ps_sp[i]);
				}
				ak_print_normal_ex("Sp: %s\n", pDebug);
				
				// Set key
				iRet = doorbell_cfg_set_aeskey(aes_key);
				if(iRet != 0)
				{
					ak_print_error_ex("Set aes_key failed!\n");
					*(pResp + SPIRESP_OFFSET_DATA) = -1;
					break;
				}
				else
				{
					memcpy(pKey_res, \
						 pKey, iLenKey);
					ak_print_normal_ex("Set key: %s\n", pKey);
				}
				
				// Set rnd
				iRet = doorbell_cfg_set_psrnd(ps_rnd);
				if(iRet != 0)
				{
					ak_print_error_ex("Set ps_rnd failed!\n");
					*(pResp + SPIRESP_OFFSET_DATA) = -1;
					break;
				}
				else
				{
					memcpy(pRnd_res, \
						 pRnd, iLenRnd);
					ak_print_normal_ex("Set rnd: %s\n", pRnd);
				}
				
				// Set sip
				iRet = doorbell_cfg_set_pssip(ps_sip);
				if(iRet != 0)
				{
					ak_print_error_ex("Set ps_sip failed!\n");
					*(pResp + SPIRESP_OFFSET_DATA) = -1;
					break;
				}
				else
				{
					memcpy(pSip_res, \
						 pSip, iLenSip);
					ak_print_normal_ex("Set sip: %s\n", pSip);
				}
				
				// Set sp
				iRet = doorbell_cfg_set_pssp(ps_sp);
				if(iRet != 0)
				{
					ak_print_error_ex("Set ps_sp failed!\n");
					*(pResp + SPIRESP_OFFSET_DATA) = -1;
					break;
				}
				else
				{
					memcpy(pSp_res, \
						 pSp, iLenSp);
					ak_print_normal_ex("Set sp: %s\n", pSp);
				}
			}
			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}

/*
* CDS value
*
*/
int g_iCdsValue = 0;

int spicmd_process_getcds(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue = 0;
	int iThresholdOn = 0, iThresholdOff = 0; 

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);

	/*-----------------------------------------------------------------------*/
	// ak_print_normal_ex("CDS: Found CC3200 send CDS!!!\n");
	// iValue = (*(pdata + SPICMD_OFFSET_DATA) << 8) & 0xFF00; // byte high
	// iValue |= *(pdata + SPICMD_OFFSET_DATA + 1) & 0xFF; // byte low
	// ak_print_normal_ex("Set Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
	// 			iValue, iValue, *(pdata + SPICMD_OFFSET_DATA), \
	// 			*(pdata + SPICMD_OFFSET_DATA + 1));
	// g_iCdsValue = iValue;
	/*-----------------------------------------------------------------------*/
	
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			iValue = g_iCdsValue;
			*(pResp + SPIRESP_OFFSET_DATA) = (iValue >>8) & 0xFF; // byte high
			*(pResp + SPIRESP_OFFSET_DATA + 1) = (iValue) & 0xFF; // byte low
			ak_print_normal_ex("Get Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA +1));
			break;
		case SPICMD_TYPE_SET:
			// write to config and send response
			ak_print_normal_ex("START g_iCdsValue_in\r\n");
			iValue = (*(pCmd + SPICMD_OFFSET_DATA) << 8) & 0xFF00; // byte high
			iValue |= *(pCmd + SPICMD_OFFSET_DATA + 1) & 0xFF; // byte low

#ifdef DOORBELL_TINKLE820
//extreme bright light CDS 500  Good Exposure 100
//normal light         CDS 636  Good Exposure 600
//extreme dark light   CDS 1200 Good Exposure 600
			if (iValue<550)
				g_exposure = 100;
			else if (iValue<600)
				g_exposure = 200;
			else if (iValue<800)
				g_exposure = 400;
			else
				g_exposure = 600;
#else
				g_exposure = 600;
#endif



			g_iCdsValue = iValue;
			*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
			*(pResp + SPIRESP_OFFSET_DATA + 1) = *(pCmd + SPICMD_OFFSET_DATA + 1);
			
			/* Read config to get value threshold led on */
			iRet = doorbell_cfg_get_cds_on(&iThresholdOn);
			ak_print_normal_ex("iThresholdOn: %d (%d)\r\n", iThresholdOn, iRet);
			if(iRet != 0)
			{
				iThresholdOn = SYS_CDS_THRESOLD_ON;
				iRet = doorbell_cfg_set_cds_on(SYS_CDS_THRESOLD_ON);
				ak_print_normal_ex("Write default config for CDS ON (%d)\r\n", iRet);
			}
			
			/* Read config to get value threshold led off */
			iRet = doorbell_cfg_get_cds_on(&iThresholdOff);
			ak_print_normal_ex("iThresholdOn: %d (%d)\r\n", iThresholdOff, iRet);
			if(iRet != 0)
			{
				iThresholdOff = SYS_CDS_THRESOLD_OFF;
				iRet = doorbell_cfg_set_cds_off(SYS_CDS_THRESOLD_OFF);
				ak_print_normal_ex("Write default config for CDS OFF (%d)\r\n", iRet);
			}

			if(g_iCdsValue > iThresholdOn)
			{
				if(LED_STATE_OFF == led_get_state())
				{
					/* Turn light on */
					led_turn_on();
				}
				else
				{
					ak_print_normal_ex("Led state is being on!\r\n");
				}
			}
			else if(g_iCdsValue < iThresholdOff)
			{
				if(LED_STATE_ON == led_get_state())
				{
					/* Turn light off */
					led_turn_off();
				}
				else
				{
					ak_print_normal_ex("Led state is being off!\r\n");
				}
			}
			else
			{
				ak_print_normal_ex("No change state! Led state:%d\r\n", led_get_state());
			}

			ak_print_normal_ex("Set Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA +1));
			
			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}

/*
* Process command support factory test
*/
static int spicmd_factory_test_light(int value)
{
	int iRet = 0;
	// check AK light
	switch(value)
	{
		case 0x01:
			led_turn_on();
			break;
		case 0x02:
			led_turn_off();
			break;
		case 0x00:
			// keep currently state. Nothing to do
			break;
		default:
			ak_print_error_ex("Receive spi cmd factory test with wrong value!");
			iRet = -1;
			break;
	}
		
	return iRet;
}

/* Tone generation */
static int spicmd_factory_test_tonegen(char on_off, int freq)
{
	int iRet = 0;
	char acFreq[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	switch(on_off)
	{
		case 0x01:
			// turn on tone gen
			iRet = snprintf(acFreq, sizeof(acFreq), "on %d", freq);
			break;
		case 0x02:
			// turn off tone gen
			iRet = snprintf(acFreq, sizeof(acFreq), "off %d", freq);
			break;
		case 0x00:
			// keep currently state. Nothing to do
			break;
		default:
			ak_print_error_ex("Receive spi cmd factory test with wrong value!");
			iRet = -1;
			break;
	}
	ftest_cmd_tonegen_handler(acFreq, iRet);
	return 0;
}

/* audio loopback */
static int spicmd_factory_test_loopback(char on_off)
{
	int iRet = 0;
	char acCmd[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	switch(on_off)
	{
		case 0x01:
			// turn on audio loopback
			iRet = snprintf(acCmd, sizeof(acCmd), "on ");
			break;
		case 0x02:
			// turn off tone gen
			iRet = snprintf(acCmd, sizeof(acCmd), "off ");
			break;
		case 0x00:
			// keep currently state. Nothing to do
			ak_print_error_ex("keep currently state. Nothing to do");
			break;
		default:
			ak_print_error_ex("Receive spi cmd factory test with wrong value!");
			iRet = -1;
			break;
	}
	ftest_cmd_audioloopback_handler(acCmd, iRet);
	return 0;
}

static int cnt_send_bitrate = 0;
int spicmd_factory_test_send_bitrate(int bitrate)
{
	int iRet = 0;
	char *pMsg = NULL;
	if(eSysModeGet() == E_SYS_MODE_FTEST)
	{
		cnt_send_bitrate++;
		if(cnt_send_bitrate == 10){
			ak_print_normal_ex("bitrate: %d byte\n", bitrate);
			cnt_send_bitrate = 0;
		}
		if(bitrate >= 0)
		{
			pMsg = (char *)malloc(sizeof(char)*MAX_SPI_PACKET_LEN);
			if(pMsg == NULL)
			{
				return -1;
			}
			memset(pMsg, 0x00, MAX_SPI_PACKET_LEN);
			*(pMsg + 0) = SPI_CMD_FACTORY_TEST;
			*(pMsg + 1) = 0x00; // AK light
			*(pMsg + 2) = 0x00; // Tonegen
			*(pMsg + 3) = 0x00; // Tonegen
			*(pMsg + 4) = 0x00; // Tonegen
			*(pMsg + 5) = (char)(bitrate >> 8 & 0xFF);
			*(pMsg + 6) = (char)(bitrate & 0xFF);
			spi_cmd_send(pMsg, MAX_SPI_PACKET_LEN);
			
			free(pMsg);
		}
	}
	return iRet;
}

int spicmd_process_factory_test(char *pCmd, char *pResp)
{
	int iRet = 0;
	int iFreq = 0;
	int iBitrate = 0;
	char cLoopBack = 0;
	ak_pthread_t  ftest_id;

	SpiCmdFactory_t *pSpiCmdFactory = NULL;

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	pSpiCmdFactory = (SpiCmdFactory_t *)(pCmd + 1);
	ak_print_normal("AKLight:%x Tonegen:%x freq:%02X %02X bitrate:%02X %02X\n", \
					pSpiCmdFactory->ak_light, \
					pSpiCmdFactory->tone_gen, \
					pSpiCmdFactory->freq[0], pSpiCmdFactory->freq[1],\
					pSpiCmdFactory->bitrate[0], pSpiCmdFactory->bitrate[1]);
	iFreq = (int)(pSpiCmdFactory->freq[0] << 8) & 0xFFFFFFFF;
	iFreq |= (int)pSpiCmdFactory->freq[1] & 0xFFFFFFFF;
	iBitrate = (int)(pSpiCmdFactory->bitrate[0] << 8) & 0xFFFFFFFF;
	iBitrate |= (int)pSpiCmdFactory->bitrate[1] & 0xFFFFFFFF;
	cLoopBack = *(pCmd + 7);

	ak_print_error_ex("Freq: %d, bitrate: %d\n", iFreq, iBitrate);

	// check ftest mode and enter ftest mode
	if(eSysModeGet() == E_SYS_MODE_NORMAL) // system is normal mode
	{
		ak_thread_create(&ftest_id, ftest_main_process, NULL, 32*1024, CFG_FTEST_TASK_PRIO);
		ak_sleep_ms(2000); // delay 2000ms
	}
	else
	{
		if(eSysModeGet() == E_SYS_MODE_FTEST)
		{
			ak_print_normal("System is in ftest mode\n");
		}
		else 
		{
			ak_print_normal("System is in fwupgrade mode\n");
			return -1;
		}
	}
	/* AK Light */
	if(spicmd_factory_test_light(pSpiCmdFactory->ak_light) != 0)
	{
		ak_print_error_ex("Cmd test AK light failed! (value:0x%x)\n", pSpiCmdFactory->ak_light);
	}
	/* Tone generation */
	if(spicmd_factory_test_tonegen(pSpiCmdFactory->tone_gen, iFreq) != 0)
	{
		ak_print_error_ex("Cmd test tonegen failed! (value:0x%x)\n", pSpiCmdFactory->tone_gen, iFreq);
	}
	
	if(iFreq == 0)
	{
		// check loop back
		if(spicmd_factory_test_loopback(cLoopBack) != 0)
		{
			ak_print_error_ex("Cmd test audio loopback failed! (value:0x%x)\n", cLoopBack);
		}
	}
	else
	{
		ak_print_error_ex("Tone Generation is running (f:%d Hz)!\n", iFreq);
	}
	return iRet;
}



/*
process spi cmd ntp
*/
int spicmd_process_ntp(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue = 0;

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			*(pResp + SPIRESP_OFFSET_DATA) = (iValue >>8) & 0xFF; // byte high
			*(pResp + SPIRESP_OFFSET_DATA + 1) = (iValue) & 0xFF; // byte low
			ak_print_normal_ex("Get Value: 0x%x (%d) ByteH: 0x%x ByteL: 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA +1));
			break;
		case SPICMD_TYPE_SET:
			// write to config and send response
			iValue = (*(pCmd + SPICMD_OFFSET_DATA) << 24) & 0xFF000000; // byte high
			iValue |= (*(pCmd + SPICMD_OFFSET_DATA + 1) << 16) & 0x00FF0000;
			iValue |= (*(pCmd + SPICMD_OFFSET_DATA + 2) << 8) & 0x0000FF00;
			iValue |= *(pCmd + SPICMD_OFFSET_DATA + 3) & 0x000000FF; // byte low

			iRet = spicmd_set_ntp(iValue);
			// g_ntp_updated = 1;

			*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
			*(pResp + SPIRESP_OFFSET_DATA + 1) = *(pCmd + SPICMD_OFFSET_DATA + 1);
			*(pResp + SPIRESP_OFFSET_DATA + 2) = *(pCmd + SPICMD_OFFSET_DATA + 2);
			*(pResp + SPIRESP_OFFSET_DATA + 3) = *(pCmd + SPICMD_OFFSET_DATA + 3);

			ak_print_normal_ex("Set Value: 0x%x (%d) ByteH: 0x%x 0x%x 0x%x 0x%x\n", \
						iValue, iValue, *(pResp + SPIRESP_OFFSET_DATA), \
						*(pResp + SPIRESP_OFFSET_DATA + 1), \
						*(pResp + SPIRESP_OFFSET_DATA + 2), \
						*(pResp + SPIRESP_OFFSET_DATA + 3));

			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}

int spicmd_upgrade_getversion(char *pCmd, char *pResp)
{
	int iRet = 0;
	int i = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);

	ak_print_normal_ex(" Version: %s\r\n", fw_version);
    *(pResp + 1) = fw_version[7];
    *(pResp + 2) = fw_version[8];
    *(pResp + 3) = fw_version[10];
    *(pResp + 4) = fw_version[11];
    *(pResp + 5) = fw_version[13];
    *(pResp + 6) = fw_version[14];
    *(pResp + 7) = '\0';

    for(i = 0; i < 8; i++)
    {
        printf("%02X ", *(pResp + i));
    }
    printf("\r\n");
	return iRet;
}


/*
* UDID Doorbell need UDID to upload snapshot and clip to server
*/
int spicmd_process_command(char cCmdId, char *pCmd, char *pResp)
{
	int iRet = 0;
	int i = 0;
	int iLenKey = 0;
	int iValue = 0;
	char acTempKey[SYS_MAX_LENGTH_KEY + 1] = {0x00, };
	char pDebug[256] = {0x00, };
	char *pKey = NULL;
	char cCmdType = 0;

	if(pCmd == NULL || pResp == NULL || \
		(cCmdId != SPI_CMD_UDID && \
			cCmdId != SPI_CMD_TOKEN && \
			cCmdId != SPI_CMD_URL))
	{
		if(cCmdId != SPI_CMD_UDID && \
			cCmdId != SPI_CMD_TOKEN && \
			cCmdId != SPI_CMD_URL)
		{
			ak_print_error_ex("cCmdId != SPI_CMD_UDID, SPI_CMD_TOKEN and SPI_CMD_URL\r\n");
		}
		else
		{
			ak_print_error_ex("Parameter is NULL\r\n");
		}
		return -1;
	}

	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	iLenKey = *(pCmd + SPICMD_OFFSET_DATA);
	ak_print_normal_ex("LenKey: %d\n", iLenKey);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	memset(acTempKey, 0x00, SYS_MAX_LENGTH_KEY + 1);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			if(cCmdId == SPI_CMD_UDID)
			{
				iRet = doorbell_cfg_get_udid(acTempKey);
			}
			else if(cCmdId == SPI_CMD_TOKEN)
			{
				iRet = doorbell_cfg_get_token(acTempKey);
			}
			else if(cCmdId == SPI_CMD_URL)
			{
				iRet = doorbell_cfg_get_url(acTempKey);
			}
			else
			{
				iRet = -2;
			}

			if(iRet != 0)
			{
				ak_print_error_ex("Get aes_key failed!\n");
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
			}
			else
			{
				*(pResp + SPIRESP_OFFSET_LEN_KEY) = iLenKey;
				pKey = pResp + SPIRESP_OFFSET_DATA_KEY;
				memcpy(pKey, acTempKey, iLenKey);
				ak_print_normal_ex("Get key: %s, length: %d\n", pKey, iLenKey);
			}
			break;
		case SPICMD_TYPE_SET:
			if(iLenKey > SYS_MAX_LENGTH_KEY)
			{
				ak_print_error_ex("Length of Key is invalid\n");
				*(pResp + SPIRESP_OFFSET_DATA) = -1;
				iRet = -1;
			}
			else
			{
				// write to config and send response
				pKey = pCmd + SPICMD_OFFSET_DATA + 1; // for key
				*(pKey + iLenKey) = '\0';
				for(i = 0; i < iLenKey; i++)
				{
					acTempKey[i] = *(pKey + i);
					sprintf(pDebug + strlen(pDebug), "%02X ", acTempKey[i]);
				}
				acTempKey[iLenKey] = '\0';
				ak_print_normal_ex("Key: %s acTempKey:%s (len:%d)\n", pDebug, acTempKey, iLenKey);

				if(cCmdId == SPI_CMD_UDID)
				{
					iRet = doorbell_cfg_set_udid(acTempKey);
				}
				else if(cCmdId == SPI_CMD_TOKEN)
				{
					iRet = doorbell_cfg_set_token(acTempKey);
				}
				else if(cCmdId == SPI_CMD_URL)
				{
					iRet = doorbell_cfg_set_url(acTempKey);
				}
				else
				{
					iRet = -2;
				}

				if(iRet != 0)
				{
					ak_print_error_ex("Set keyGROUP failed!\n");
					*(pResp + SPIRESP_OFFSET_DATA) = -1;
				}
				else
				{
					memcpy(pResp + SPIRESP_OFFSET_DATA_KEY, pKey, iLenKey);
					ak_print_normal_ex("Set key: %s\n", pKey);
				}
			}
			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}

int spicmd_process_udid(char *pCmd, char *pResp)
{
	int iRet = 0;
	iRet = spicmd_process_command(SPI_CMD_UDID, pCmd, pResp);
	return iRet;
}

/*
* Token Doorbell need token to upload snapshot and clip to server
*/
int spicmd_process_token(char *pCmd, char *pResp)
{
	int iRet = 0;
	iRet = spicmd_process_command(SPI_CMD_TOKEN, pCmd, pResp);
	return iRet;
}

int spicmd_process_url(char *pCmd, char *pResp)
{
	int iRet = 0;
	iRet = spicmd_process_command(SPI_CMD_URL, pCmd, pResp);
	return iRet;
}
char g_voicevalue = 0x00;
int g_receive_update = 0;
int spicmd_process_voice(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue1 = 0;
	ak_print_error_ex("Receive cmd play voice prompt!\r\n");
	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
//	if(g_voicevalue != cCmdType)
//	{
		g_receive_update = 1;

		g_voicevalue = cCmdType;
//	}
	ak_print_normal_ex("g_voicevalue_in:%p\r\n",g_voicevalue);
	return iRet;
}

/*
* return 0: file validate
* return != 0; file invalid
*/
static int filetransfer_check_file_validate(char *pCmd, long *pfile_size)
{
	int iRet = 0;
	char *pFilename = NULL;
	char acFilePath[256] = {0};
	int iLength = 0;
	long file_size;
	//FILE *fp = NULL;


	if(pCmd == NULL)
	{
		iRet = -1;
	}
	else
	{
		iLength = (*(pCmd + SPICMD_CTRL_1_LEN_OFFSET) & 0x000000FF) << 8;
		iLength |= (*(pCmd + SPICMD_CTRL_1_LEN_OFFSET + 1) & 0x000000FF) << 0;
		if(iLength <= 0)
		{
			ak_print_error_ex("Length of filename is invalid (%d)\r\n", iLength);
			iRet = -2;
		}
		else
		{
			iLength += 1; // for \0 character
			pFilename = (char *)malloc(sizeof(char) * iLength);
			if(pFilename == NULL)
			{
				ak_print_error_ex("Allocate memory pFilename failed!\r\n");
				iRet = -3;
			}
			else
			{
				iRet = 0;
				memset(pFilename, 0x00, iLength);
				memcpy(pFilename, pCmd + SPICMD_CTRL_1_DATA_OFFSET, iLength - 1);
				ak_print_error_ex("P2p: file name: %s\r\n", pFilename);
				// open file and return abc xyz :D
				// fix me: full path and directory!!!!!!
				if(strlen(g_acDirName_Clip) == 0)
				{
					ak_print_error_ex("Folder name is not created yet!\r\n");
					iRet = video_get_system_info_groupname();
				}
				
				// ak_mount_fs(1, 0, "a:");
				if(sdcard_get_status() != E_SDCARD_STATUS_MOUNTED )
				{
					sdcard_mount();
				}
				else
				{
					ak_print_normal("SDcard was mounted! Ready!\r\n");
				}

				if(iRet == 0)
				{
					sprintf(acFilePath, "a:/%s/%s", g_acDirName_Clip, pFilename);

					if(strcmp(acFilePath, g_SpiCmdFileStream) == 0)
					{
						// file is same
						ak_print_error_ex("File is open '%s' \r\n", acFilePath);
					}
					else //Thuan add - 13/10/2017
					{
						g_pvFile = fopen(acFilePath, "r");
						if (g_pvFile == NULL)
						{
							ak_print_error_ex("open file '%s' failed\r\n", acFilePath);
							iRet = -4;
						}
						else //open file success
						{
							ak_print_error_ex("OK g_pvFile: %p\r\n", g_pvFile);

							file_size = p2p_get_size_file(g_pvFile);

							*pfile_size = file_size;

							if (file_size <= 0)
							{
								ak_print_error_ex("File error: '%s', file size: %ld byte \r\n", acFilePath, file_size);
								iRet = -4;
							}
							else
						    {   
								ak_print_error_ex("The valid file: '%s', file size: %ld byte \r\n", acFilePath, file_size);
								memset(g_SpiCmdFileStream, 0x00, sizeof(g_SpiCmdFileStream));
								memcpy(g_SpiCmdFileStream, acFilePath, strlen(acFilePath));
								iRet = 0;
							}
						} // if (!fp)

						//Thuan add -09/10/2017
						if (g_pvFile != NULL)
						{
							fclose(g_pvFile);
							g_pvFile = NULL;
						}
					}
				}
			} // if(pFilename == NULL)
		} // if(iLength <= 0)
	} // if(pCmd == NULL)
	// open file
	return iRet;
}

int spicmd_process_filetransfer(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cFileValidate = SPICMD_CTRL_2_FILE_NOK;
	FILE *fp = NULL;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	long lfile_size;
	char cfile_size_onebyte; 

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	// parse byte control
	// 0x01: select file
	// 0x03: file transfer start
	// 0x05: file transfer stop (resume streaming)
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);

	switch(cCmdType)
	{
		case SPICMD_CONTROL_SELECT_FILE:
		    ak_print_error_ex("-----------SPICMD_CONTROL_SELECT_FILE-----------\r\n");
			// open file in SDCard and return response to CC OK or NOK
			iRet = filetransfer_check_file_validate(pCmd, &lfile_size);
			if(iRet == 0) // file validate
			{
				g_bValid = true;
				cFileValidate = SPICMD_CTRL_2_FILE_OK;
			
				// init semaphore
				// iRet = ak_thread_sem_init(&sem_SpiCmdStartTransfer, 0);
				ak_thread_sem_init(&sem_SpiCmdStartTransfer, 0);
				ak_print_normal_ex("Init semaphore ret (%d)\r\n", iRet);
			}
			else
			{
				g_bValid = false;
				lfile_size = 0;
				ak_print_error_ex("p2p failed: iRet: %d\r\n", iRet);
				cFileValidate = (char)(iRet & 0xFF); // return error code :D
			}
			*(pResp + 1) = SPICMD_CONTROL_FILE_VALIDATE; // byte control
			*(pResp + 2) = cFileValidate; // byte validate

			//Thuan add -13/10/2017
			//lfile_size => byte_MSB -> byte_LSB
			//memcpy(pResp + 3, &file_size, sizeof(file_size));

			//byte_3
			cfile_size_onebyte =  (char)((lfile_size >>24) & 0xFF);
			*(pResp + 3) = cfile_size_onebyte;
			//byte_2
			cfile_size_onebyte =  (char)((lfile_size >>16) & 0xFF);
			*(pResp + 4) = cfile_size_onebyte;
			//byte_1
			cfile_size_onebyte =  (char)((lfile_size >>8) & 0xFF);
			*(pResp + 5) = cfile_size_onebyte;
			//byte_0
			cfile_size_onebyte =  (char)((lfile_size >>0) & 0xFF);
			*(pResp + 6) = cfile_size_onebyte;

			printf("Validate: Resp byte offset 2: 0x%02x\r\n", *(pResp + 2));
			break;
		
		case SPICMD_CONTROL_TRANSFER_START:
			// stop streaming, start transfer
			// ak_thread_sem_post(&sem_SpiCmdStartTransfer);
			// ak_thread_sem_init(&sem_SpiCmdStopTransfer, 0);
			ak_print_error_ex("-----------SPICMD_CONTROL_TRANSFER_START-----------\r\n");
			if (g_bValid)
			{
				g_iP2pSdStreamStart = 1;
				ak_print_error_ex("a Trung Do have started xfer!!!!\r\n");
			}
			else
			{
				ak_print_error_ex("TRANSFER_START: file is invalid, not start!!!!\r\n");
			}
			
			*(pResp + 1) = SPICMD_CONTROL_TRANSFER_START;
			// no need to response???
			break;
		case SPICMD_CONTROL_TRANSFER_STOP:
			// stop send file and resume streaming
			ak_print_error_ex("a Trung Do have stopped xfer!!!!\r\n");
			g_iP2pSdStreamStart = 0; // allow streaning, talkback, audio, video, snapshot
			g_bValid = false;
			// ak_thread_sem_post(&sem_SpiCmdStopTransfer);
			*(pResp + 1) = SPICMD_CONTROL_TRANSFER_STOP;
			// no need to response
			break;
		// case SPICMD_CONTROL_FILE_VALIDATE:
		// 	//CC never send this control :D
		// 	break;
		// case SPICMD_CONTROL_TRANSFER_DONE:
		// 	// CC never send this control :D
		// 	break;
		default:
			iRet = -1;
			ak_print_error_ex("Unknown control of cmd! (%02x)\n", cCmdType);
			break;
	}

	return iRet;
}

/*
"0x21"<SpeakerVol1><MicVol1><Echo><NoiseRed> Master
"0xA1"<Get/Set1><SpeakerVol1><MicVol1><Echo><NoiseRed>
Slave    Set speaker and microphone volume (0->255), 
EchoCancellation/NoiseReduction On(1)/off(0)
*/
int spicmd_process_volume(char *pCmd, char *pResp)
{
	int iRet = 0;
	char cCmdType = SPICMD_TYPE_UNKNOWN;
	int iValue = 0;
	char cValueSet = 0;

	if(pCmd == NULL || pResp == NULL)
	{
		return 0;
	}
	cCmdType = *(pCmd + SPICMD_OFFSET_TYPE);
	memset(pResp, 0x00, MAX_SPI_PACKET_LEN);
	*(pResp + SPIRESP_OFFSET_ID) = (*pCmd)&(~CC_SEND_SPI_PACKET_MASK);
	switch(cCmdType)
	{
		case SPICMD_TYPE_GET:
			// read spk gain
			iRet = doorbell_cfg_get_spka_gain(&iValue);
			if(iRet != 0 || \
				(iValue < SYS_MICSPK_ANALOG_MIN || iValue > SYS_MICSPK_DIGITAL_MAX))
			{
				ak_print_error_ex("Get speaker gain failed!\n");
				iValue = -1;
			}
			*(pResp + SPICMD_SPK_GAIN_OFFSET) = iValue & 0xFF;
			ak_print_normal_ex("SPK VOL Value: %d\n", iValue);
			
			// read mic gain
			iValue = 0;
			iRet = doorbell_cfg_get_mica_gain(&iValue);
			if(iRet != 0 || \
				(iValue < SYS_MICSPK_ANALOG_MIN || iValue > SYS_MICSPK_DIGITAL_MAX))
			{
				ak_print_error_ex("Get mic gain failed!\n");
				iValue = -1;
			}
			*(pResp + SPICMD_MIC_GAIN_OFFSET) = iValue & 0xFF;
			ak_print_normal_ex("MIC VOL Value: %d\n", iValue);
			
			// Read echo
			iValue = 0;
			iRet = doorbell_cfg_get_echocancellation(&iValue);
			if(iRet != 0)
			{
				ak_print_error_ex("Get echo failed!\n");
				iValue = -1;
			}
			*(pResp + SPICMD_ECHO_OFFSET) = iValue & 0xFF;
			ak_print_normal_ex("ECHO Value: %d\n", iValue);

			// Read agc nr
			iValue = 0;
			iRet = doorbell_cfg_get_agcnr(&iValue);
			if(iRet != 0)
			{
				ak_print_error_ex("Get agc nr failed!\n");
				iValue = -1;
			}
			*(pResp + SPICMD_AGCNR_OFFSET) = iValue & 0xFF;
			ak_print_normal_ex("AGC NR Value: %d\n", iValue);

			break;

		case SPICMD_TYPE_SET:
			// speaker volume
			cValueSet = *(pCmd + SPICMD_OFFSET_DATA);
			if(cValueSet < SYS_MICSPK_ANALOG_MIN || cValueSet > SYS_MICSPK_DIGITAL_MAX)
			{
				ak_print_error_ex("Value SPK GAIN invalid");
			}
			else
			{
				iRet = doorbell_cfg_set_spka_gain(cValueSet);
				if(iRet != 0)
				{
					*(pResp + SPIRESP_OFFSET_DATA) = -1;
					ak_print_error_ex("Set value SPK GAIN FAILED!(%d)\n", iRet);
				}
				else
				{
					*(pResp + SPIRESP_OFFSET_DATA) = *(pCmd + SPICMD_OFFSET_DATA);
					ak_print_normal_ex("Set value SPK GAIN OK!\n");
					// g_speaker_volume = cValueSet;
					talkback_set_volume_onflight();
				}
				// TODO: set on flight
				// talkback_set_volume_onflight();
				talkback_set_volume_onflight_with_value(cValueSet);
			}

			// MIC volume
			cValueSet = *(pCmd + SPICMD_OFFSET_DATA + 1);
			if(cValueSet < SYS_MICSPK_ANALOG_MIN || cValueSet > SYS_MICSPK_DIGITAL_MAX)
			{
				ak_print_error_ex("Value MIC GAIN invalid");
			}
			else
			{
				iRet = doorbell_cfg_set_mica_gain(cValueSet);
				if(iRet != 0)
				{
					*(pResp + SPIRESP_OFFSET_DATA + 1) = -1;
					ak_print_error_ex("Set value MIC GAIN FAILED!(%d)\n", iRet);
				}
				else
				{
					*(pResp + SPIRESP_OFFSET_DATA + 1) = *(pCmd + SPICMD_OFFSET_DATA + 1);
					ak_print_normal_ex("Set value MIC GAIN OK!\n");
					// g_mic_volume = cValueSet;
					// g_volume_updated = 1;
					audio_set_volume_onflight();
				}
				// TODO: set on flight
				// audio_set_volume_onflight();
				audio_set_volume_onflight_with_value(cValueSet);
			}

			// echo cancellation
			cValueSet = *(pCmd + SPICMD_OFFSET_DATA + 2);
			if(cValueSet != 0 && cValueSet != 1)
			{
				ak_print_error_ex("Value Echo cancellation invalid: value: %d\r\n", cValueSet);
			}
			else
			{
				iRet = doorbell_cfg_set_echocancellation(cValueSet);
				if(iRet != 0)
				{
					*(pResp + SPIRESP_OFFSET_DATA + 2) = -1;
					ak_print_error_ex("Set value Echo cancellation FAILED!(%d)\n", iRet);
				}
				else
				{
					*(pResp + SPIRESP_OFFSET_DATA + 2) = *(pCmd + SPICMD_OFFSET_DATA + 2);
					ak_print_normal_ex("Set value Echo cancellation OK!\n");
				}
				// TODO: set on flight
			}

			// agc nr
			cValueSet = *(pCmd + SPICMD_OFFSET_DATA + 3);
			if(cValueSet != 0 && cValueSet != 1)
			{
				ak_print_error_ex("Value AGC NR invalid: value: %d\r\n", cValueSet);
			}
			else
			{
				iRet = doorbell_cfg_set_agcnr(cValueSet);
				if(iRet != 0)
				{
					*(pResp + SPIRESP_OFFSET_DATA + 3) = -1;
					ak_print_error_ex("Set value AGC NR FAILED!(%d)\n", iRet);
				}
				else
				{
					*(pResp + SPIRESP_OFFSET_DATA + 3) = *(pCmd + SPICMD_OFFSET_DATA + 3);
					ak_print_normal_ex("Set value AGC NR OK!\n");
				}
				// TODO: set on flight
			}

			break;
		default:
			ak_print_error_ex("Unknown type of cmd!\n");
			break;
	}
	return iRet;
}


/*
* For play dingdong cmd, only receive cmd and play.
*/
static int flag_first_recv_cmd_snapshot = 0;
static unsigned long ulTickLastCmdDing = 0;
char g_event_info[2048];
int spicmd_play_dingdong(char *pCmd, char *pResp)
{
	unsigned long ulTickCurrent = 0;
	ak_print_error_ex("RECV cmd play dingdong!\r\n");
//	ak_thread_sem_post(&sem_SpiCmdSnapShot);
//	ak_print_normal("%s thread_sem_post -> sem_SpiCmdSnapShot\r\n", __func__);
	int l_option = *(pCmd + SPICMD_DINGDONG_OPTION_OFFSET);
	if (l_option==SPICMD_DINGDONG_OPTION_DINGDONG){
		g_play_dingdong = 1;
//		g_event_info[0]=0x00;
	} else if (l_option==SPICMD_DINGDONG_OPTION_SNAPSHOT){
		int l_len=(*(pCmd+SPICMD_DINGDONG_OPTA0_LEN_OFFSET))<<8;
		l_len|=(*(pCmd+SPICMD_DINGDONG_OPTA0_LEN_OFFSET+1));
		memset(g_event_info,0x00,sizeof g_event_info);
		memcpy(g_event_info,pCmd+SPICMD_DINGDONG_OPTA0_DATA_OFFSET,l_len);
	} else
		return 0;

	if(flag_first_recv_cmd_snapshot == 0)
	{
		ulTickLastCmdDing = get_tick_count();
		flag_first_recv_cmd_snapshot = 1;
		g_SpiCmdSnapshot = 1;
		printf("First SPICMD g_iDoneSnapshot: %d, g_isDoneUpload: %d, g_SpiCmdSnapshot: %d\r\n", g_iDoneSnapshot, g_isDoneUpload, g_SpiCmdSnapshot);
		//ak_thread_sem_post(&sem_SpiCmdSnapShot);
		ak_print_normal("%s thread_sem_post -> sem_SpiCmdSnapShot\r\n", __func__);
	}
	else
	{
		ulTickCurrent = get_tick_count();
		if(ulTickCurrent - ulTickLastCmdDing <= SYS_COOLDOWN_PUSH)
		{
			// g_SpiCmdSnapshot = 0;
			ak_print_normal("still cooldown, not allow pushing, cd:%u\r\n", ulTickCurrent - ulTickLastCmdDing);
			printf("CD: SPICMD g_iDoneSnapshot: %d, g_isDoneUpload: %d, g_SpiCmdSnapshot: %d\r\n", g_iDoneSnapshot, g_isDoneUpload, g_SpiCmdSnapshot);
			return 0;
		}
		else
		{
			ulTickLastCmdDing = get_tick_count();
			g_SpiCmdSnapshot = 1;
			//ak_thread_sem_post(&sem_SpiCmdSnapShot);
			printf("SPICMD g_iDoneSnapshot: %d, g_isDoneUpload: %d, g_SpiCmdSnapshot: %d\r\n", g_iDoneSnapshot, g_isDoneUpload, g_SpiCmdSnapshot);
			ak_print_normal("%s OK let's go ->SnapShot\r\n", __func__);
		}
	}
	return 0;
}


/*
*   Brief: Process command data
*   Param: [pCmdData] pointer to spi command data packet
*   Param: [pResponse] pointer to Buffer response data after processing
*   Return: <0 error
*           =0 OK
*			>0 Send response with length (function return length response)
*   Author: Tien Trung
*   Date: 23-June-2017
*/
static int spi_cmd_process_data(char *pCmdData, char *pResponse)
{
	int ret = 0;
	unsigned char ucCmdId = 0;
	int i = 0;
	int len_table = sizeof(SPI_CMD_TABLE) / sizeof(SpiCmd_t);

	if(!pCmdData || !pResponse)
	{
		return -1;
	}
	ucCmdId = (unsigned char )(*pCmdData & (~CC_SEND_SPI_PACKET_MASK));
	dzlog_info("%s CmdType:%02x, len_table:%d\n", __func__, ucCmdId, len_table);
	for(i = 0; i < len_table; i++)
	{
		if(SPI_CMD_TABLE[i].cmd_type == ucCmdId)
		{
			ret = SPI_CMD_TABLE[i].pfHandle(pCmdData, pResponse);
			// memcpy(pResponse, pCmdData, MAX_SPI_PACKET_LEN);
			// *(pResponse + 0) &= 0x7F; 
			dzlog_info("\n[CMD][%02x]", ucCmdId);
			spi_cmd_dump_data(pResponse);
			break;
		}
	}
	return ret;
}

/*
*start a thread to process command SPI
*/
void *spi_command_task(void *arg)
{
	int ret = 0;
	char *pBufRecv = NULL;
	char *pResponse = NULL;
	int iLengthResponse = 0;

	int cnt = 0;
	// test p2p config
	int i = 0;
	uint8_t UT_send_data[128];
    memset(UT_send_data, 0xFA, sizeof(UT_send_data));
	/* Init semaphore */
//	ak_thread_sem_init(&sem_SpiCmdSnapShot, 0);

	pBufRecv = (char *)malloc(MAX_SPI_PACKET_LEN);
	if(pBufRecv == NULL)
	{
		dzlog_error("%s Cannot allocate memory for pBufRecv\n", __func__);
		goto exit;
	}
	pResponse = (char *)malloc(MAX_SPI_PACKET_LEN);
	if(pResponse == NULL)
	{
		dzlog_error("%s Cannot allocate memory for pResponse\n", __func__);
		goto exit;
	}

	while(1)
	{
		// receive spi command packet
		ret = spi_cmd_receive(pBufRecv, MAX_SPI_PACKET_LEN);
		if(ret < 0)
		{
			dzlog_error("%s spi cmd recv failed!\n", __func__);
		}
		else if(ret > 0)
		{
			dzlog_info("%s RECV SPI CMD: ", __func__);
			spi_cmd_dump_data(pBufRecv);
			iLengthResponse = spi_cmd_process_data(pBufRecv, pResponse);
			ret = spi_cmd_send(pResponse, MAX_SPI_PACKET_LEN);			
		}
		else
		{
			ak_sleep_ms(20);
		}

		if(g_sd_update == CK_STATE_SD_RECORD_START)
		{
			if((i % 40)==0){
				if(E_SDCARD_STATUS_MOUNTED == sdcard_get_status())
				{
					pResponse[0] = SPICMD_AK_CUT_PW;
					spi_cmd_send(pResponse, MAX_SPI_PACKET_LEN);
					dzlog_info("send power ak\r\n");
				}
			}
		}
		i++;
	}
exit:
	if(pBufRecv)
	{
		free(pBufRecv);
	}
	if(pResponse)
	{
		free(pResponse);
	}
	ak_thread_exit();
	return NULL;
}

/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/
