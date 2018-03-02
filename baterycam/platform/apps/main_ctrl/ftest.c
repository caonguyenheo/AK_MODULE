/**
  ******************************************************************************
  * File Name          : ftest.c
  * Description        : This file contain functions for factory test
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  *
/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/

#include "ftest.h"
#include "ftest_command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "ak_thread.h"
#include "command.h"
#include "ak_common.h"
#include "kernel.h"
#include "sys_common.h"
#include "ak_common.h"
#include "arch_uart.h"

#include "main_ctrl.h"
#include "ak_apps_config.h"

#include "ringbuffer.h"
#include "hal_print.h"
#include "fm2018_drv.h"

#include "fm2018_config.h"

#include "ak_drv_wdt.h"

// #include "cmd_core.h"
#include "idle.h"
#include "version.h"

#include "ak_drv_detector.h"

#include "doorbell_config.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
#define MAX_CMD_LEN         128
extern ak_pthread_t g_video_id, g_audio_id, g_spi_cmd_id, g_spi_id, g_test_id, g_dbltc_id;

static char *help[]={
	"ftest module",
	"usage: audio loopback\n"
	"   this line for reserver\n"
};

// #define FTEST_MAX_BUFFER    1024
RingBuffer  *console_fifo;
int found_enter = 0;
char data[80];

static ak_pthread_t gFtestMonitorId[E_FCMD_MAX];

//Declare variable on of video task for UVC
static char g_cUVC_turn_on = 0;

/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/

static char *tonegen_help[]={
	"\r\natecmd tonegen <on/off> <frequency>\n",
    "frequency: 300, 1000, 2000, 3000 Hz\r\n",
    ""
};

static char *audio_loopback_help[]={
	"\r\natecmd audioloopback <on/off>\n",
    ""
};
static char *ntp_help[]={
	"\r\natecmd ntp <offset>\n",
    "atecmd ntp 0 -> get current time\n"
    "atecmd ntp 1502186017 -> set offset to system\n"
    ""
};
static char *fmset_reg_help[]={
	"\r\natecmd fmset <reg> <value>\n",
    "ex: atecmd fmset 1EA1 7FFF\r\n",
    ""
};
static char *fmget_reg_help[]={
	"\r\natecmd fmget <reg>\n",
    "ex: atecmd fmget 1EA1\r\n",
    ""
};

static char *i2cwr_help[]={
	"\r\natecmd i2cwr <address> <register> <value>\n",
    "atecmd i2cwr 1C 12AB 1234\r\n",
    ""
};
static char *i2crd_help[]={
	"\r\natecmd i2crd <address> <register>\n",
    "atecmd i2crd 1C 12AB\r\n",
    ""
};

static char *micd_help[]={
	"\r\natecmd mic_d <get/set> <value to set>\n",
    ""
};
static char *mica_help[]={
	"\r\natecmd mic_a <get/set> <value to set>\n",
    ""
};
static char *spkd_help[]={
	"\r\natecmd spk_d <get/set> <value to set>\n",
    ""
};
static char *spka_help[]={
	"\r\natecmd spk_a <get/set> <value to set>\n",
    ""
};

static char *led_help[]={
	"\r\natecmd led all <on/off>\n",
    ""
};

static char *writesn_help[]={
	"\r\natecmd writesn \"<serial_string>\" \n",
    ""
};

static char *readsn_help[]={
	"\r\natecmd readsn\n",
    ""
};

static char *datecode_help[]={
	"\r\natecmd datecode \n",
    ""
};


static char *videouvc_help[]={
	"\r\natecmd video <on/off> \n",
    ""
};

static char *cdsvalue_help[]={
	"\r\natecmd cdsvalue <off_value> <on_value> \n",
    ""
};

static char *agc_help[]={
	"\r\natecmd agc get/set <index> <min>\n",
	"\r\natecmd agc time <milisecond>\n",
	"\r\natecmd agc <on/off>\n",
    ""
};


void ftest_cmd_help_handler(char *arg, int length);
void ftest_cmd_ver_handler(char *arg, int length);
void ftest_cmd_reboot_handler(char *arg, int length);
void ftest_cmd_reset_handler(char *arg, int length);
void ftest_cmd_test_handler(char *arg, int length);
void ftest_cmd_cpu_handler(char *arg, int length);
void ftest_cmd_tonegen_handler(char *arg, int length);
void ftest_cmd_audioloopback_handler(char *arg, int length);
void ftest_cmd_ntp_handler(char *arg, int length);
void ftest_cmd_config_handler(char *arg, int length);
void ftest_cmd_fm2018_handler(char *arg, int length);
void ftest_cmd_fmset_reg_handler(char *arg, int length);
void ftest_cmd_fmget_reg_handler(char *arg, int length);

void ftest_cmd_i2c_write_handler(char *arg, int length);
void ftest_cmd_i2c_read_handler(char *arg, int length);

void ftest_cmd_mic_digital_handler(char *arg, int length);
void ftest_cmd_mic_analog_handler(char *arg, int length);
void ftest_cmd_spk_digital_handler(char *arg, int length);
void ftest_cmd_spk_analog_handler(char *arg, int length);

void ftest_cmd_sddetect_handler(char *arg, int length);
void ftest_cmd_spktest_handler(char *arg, int length);

void ftest_cmd_led_handler(char *arg, int length);
void ftest_cmd_write_serialnumber_handler(char *arg, int length);
void ftest_cmd_read_serialnumber_handler(char *arg, int length);
void ftest_cmd_datecode_handler(char *arg, int length);
void ftest_cmd_uvc_handler(char *arg, int length);
void ftest_cmd_getset_cds_threshold_handler(char *arg, int length);
void ftest_cmd_agc_handler(char *arg, int length);

static FTEST_CMD_t g_aTableFtestCmd[E_FCMD_MAX] = 
{
    {"ver", E_FCMD_VERSION, {0x0}, ftest_cmd_ver_handler, "version firmware AK"},
    {"help", E_FCMD_HELP,  {0x0}, ftest_cmd_help_handler, "list commands are supported"},
    {"cpu", E_FCMD_CPU,  {0x0}, ftest_cmd_cpu_handler, "show status cpu info"},
    {"tonegen", E_FCMD_PLAY_BEEP,  {0x0}, ftest_cmd_tonegen_handler, "tone generatio (frequency 300, 1k, 2k, 3kHz) for speaker test"},
    {"audiolb", E_FCMD_AUDIO_LOOPBACK,  {0x0}, ftest_cmd_audioloopback_handler, "audio loopback"},
    {"reboot", E_FCMD_REBOOT,  {0x0}, ftest_cmd_reboot_handler, "use wdt reboot AK chip"},
    {"swreset", E_FCMD_RESET,  {0x0}, ftest_cmd_reset_handler, "reset software"},
    {"ntp", E_FCMD_NTP,  {0x0}, ftest_cmd_ntp_handler, "test local time, ntp"},
    {"cfg", E_FCMD_CONFIG,  {0x0}, ftest_cmd_config_handler, "test write all config"},
// #ifdef DOORBELL_TINKLE820
    {"fm", E_FCMD_FM,  {0x0}, ftest_cmd_fm2018_handler, "test fm2018 config"},
    {"fmset", E_FCMD_FM_SET,  {0x0}, ftest_cmd_fmset_reg_handler, "fm2018 set register"},
    {"fmget", E_FCMD_FM_GET,  {0x0}, ftest_cmd_fmget_reg_handler, "fm2018 get register"},
// #endif
    {"i2cwr", E_FCMD_I2C_WRITE,  {0x0}, ftest_cmd_i2c_write_handler, "i2c write a register with address"},
    {"i2crd", E_FCMD_I2C_READ,  {0x0}, ftest_cmd_i2c_read_handler, "i2c read a register with address"},

    {"mic_d", E_FCMD_MICDIGITAL,  {0x0}, ftest_cmd_mic_digital_handler, "mic digital get set gain"},
    {"mic_a", E_FCMD_MICANALOG,  {0x0}, ftest_cmd_mic_analog_handler, "mic analog get set gain"},
    {"spk_d", E_FCMD_SPKDIGITAL,  {0x0}, ftest_cmd_spk_digital_handler, "speaker digital get set gain"},
    {"spk_a", E_FCMD_SPKANALOG,  {0x0}, ftest_cmd_spk_analog_handler, "speaker analog get set gain"},

    {"sddetect", E_FCMD_SDCARD_DETECT,  {0x0}, ftest_cmd_sddetect_handler, "sdcard detect"},
    {"spktest", E_FCMD_TEST_SPEAKER,  {0x0}, ftest_cmd_spktest_handler, "only test speaker"},
    {"writesn", E_FCMD_WRITESN,  {0x0}, ftest_cmd_write_serialnumber_handler, "write serial number"},
    {"readsn", E_FCMD_READSN,  {0x0}, ftest_cmd_read_serialnumber_handler, "read serial number"},
    {"datecode", E_FCMD_DATECODE,  {0x0}, ftest_cmd_datecode_handler, "read datecode"},
    {"video", E_FCMD_UVC, {0x0}, ftest_cmd_uvc_handler, "test video by using UVC-MPEG"},
    {"led", E_FCMD_LED,  {0x0}, ftest_cmd_led_handler, "on/off led"},
    {"cdsvalue", E_FCMD_CDSVALUE,  {0x0}, ftest_cmd_getset_cds_threshold_handler, "get or set the cds threshold value (ftest command: cdsvalue <off> <on>)"},

	{"agc", E_FCMD_AGC, {0x0}, ftest_cmd_agc_handler, "automatic gain control get/set parameters" }
    // {"test", E_FCMD_TEST,  {0x0}, ftest_cmd_test_handler, "test structure ftest"},
};

static int show_help(char *help[])
{
    int i = 0;
    int size = 0;
    if(help == NULL)
    {
        return -1;
    }

    while(help[i] != NULL)
    {
        if(strlen(help[i])){
            printf("%s\n", help[i]);
            i++;
        }
        else
        {
            break;
        }
    }
}


//Thuan add - 03/11/2017
void ftest_cmd_getset_cds_threshold_handler(char *arg, int length)
{
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;
    int off_value = 0;
    int on_value = 0;

    /*
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);
    ak_print_normal("*arg: 0x%02x\n", *arg);
    */

    if ((*pStrProcess == NULL)||((length == 1)&&(*pStrProcess == 0x06))||(strcmp(pStrProcess, " ") == 0))// no input parameters, print the current cds value
    {
        // print out the current cds value
        // printf("\r\n--------------------- Getting the current CDS (ON/OFF) value----------------------- \r\n");
        //get cds off value
        iRet = doorbell_cfg_get_cds_off(&off_value);
        if(iRet != 0)
        {
            off_value = SYS_CDS_THRESOLD_OFF;
            iRet = doorbell_cfg_set_cds_off(SYS_CDS_THRESOLD_OFF);
            printf("Write default config for CDS OFF (%d)\r\n", iRet);
        }

        //Get cds on value
        iRet = doorbell_cfg_get_cds_on(&on_value);
        if(iRet != 0)
        {
            on_value = SYS_CDS_THRESOLD_ON;
            iRet = doorbell_cfg_set_cds_on(SYS_CDS_THRESOLD_ON);
            printf("Write default config for CDS ON (%d)\r\n", iRet);
        }

        printf("\r\nCDS off_value: %d\n", off_value);
        printf("\r\nCDS on_value: %d\n", on_value);
        printf("\r\n OK \n");
    }
    else
    {
        pNextStr = strchr(pStrProcess, ' ');

        if (pNextStr == NULL)//only one input, return error
        {
            // print error
            printf("\r\nCommand invalid.. \r\n");
            printf("\r\nFAILED\r\n");
            show_help(cdsvalue_help);
        }
        else
        {
            *pNextStr = '\0';
            pNextStr++;
    
            // process the first input, cds off_value
            off_value = atoi(pStrProcess);

            pStrProcess = pNextStr;
            pNextStr = strchr(pStrProcess, ' ');
            if (pNextStr != NULL)
            {
                *pNextStr = '\0';
                pNextStr++;
            }

            if (strcmp(pStrProcess, "") == 0)
            {
                // print error
                printf("\r\nCommand invalid.. \r\n");
                printf("\r\nFAILED\r\n");
                show_help(cdsvalue_help);
                return;
            }
            else
            {
                // printf("\r\n------------------------ Setting the CDS (ON/OFF) value ------------------------- \r\n");
                // process the second input, cds on_value 
                on_value = atoi(pStrProcess);

                iRet = doorbell_cfg_set_cds_off(off_value);
                if (iRet < 0)
                {
                    printf("\r\nSet CDS off value fail! \r\n");
                    printf("\r\nFAILED\r\n");
                    show_help(cdsvalue_help);
                    return;
                }
                            
                iRet = doorbell_cfg_set_cds_on(on_value);
                if (iRet < 0)
                {
                    printf("\r\nSet CDS on value fail! \r\n");
                    printf("\r\nFAILED\r\n");
                    show_help(cdsvalue_help);
                    return;
                }
                
                printf("\r\nCDS off_value: %d\r\n", off_value);
                printf("\r\nCDS on_value: %d\r\n", on_value);
                printf("\r\n OK \r\n");
            }
        }
    }
    return;
}

/*
*  Show list commands which ftest support
*/
void ftest_cmd_help_handler(char *arg, int length)
{
    int i = 0;
    ak_print_normal("%s\n", __func__);
    for(i = 0; i < E_FCMD_MAX; i++)
    {
        ak_print_normal("%s\t\t\t: %s\n", g_aTableFtestCmd[i].name, \
            g_aTableFtestCmd[i].description);
    }
    ak_print_normal("--------------------------------------\r\n");
}

/*
* Show version firmware of AK Chip
*/
void ftest_cmd_ver_handler(char *arg, int length)
{
    printf("%s\n", fw_version);
    printf("\r\nOK\r\n");
}

/*
* show status cpu info
*/
void ftest_cmd_cpu_handler(char *arg, int length)
{
    unsigned long idle = 0;
    unsigned long usage = 0;
    idle = idle_get_cpu_idle();
    usage = idle_get_cpu_usage();
    printf("Idle: %u %%, usage: %u %%\n", idle, usage);
    printf("GetTotalRamSize: %u B\n", Fwl_GetTotalRamSize());
    printf("RamUsedBlock: %u\n", Fwl_RamUsedBlock());
    printf("GetUsedRamSize: %u\n", Fwl_GetUsedRamSize());
    printf("RamGetBlockNum: %u\n", Fwl_RamGetBlockNum());
    printf("RamGetBlockLen: %u\n", Fwl_RamGetBlockLen());
    printf("GetRemainRamSize: %u\n", Fwl_GetRemainRamSize());
    printf("GetLargestSize_Allocable: %u B\n", Fwl_GetLargestSize_Allocable());   
    cmd_show_jobs();
    ak_print_normal("--------------------------------------\r\n");
}


void ftest_cmd_test_handler(char *arg, int length)
{
    ak_pthread_t thread_id;
    int iRet = 0;
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    iRet = ak_thread_create(&thread_id,  ftest_test_structure_thread, arg, 16*1024, 10);
    ak_print_normal_ex("Created thread id: %u, iRet: %d\n", thread_id, iRet);
    ak_print_normal("\r\nOK\r\n");
    // gFtestMonitorId[E_FCMD_TEST] = thread_id;
}

/*
* Using watchdog timer to reboot AK chip
*/
void ftest_cmd_reboot_handler(char *arg, int length)
{
    unsigned int fd = 0;
    unsigned int feed_time = 100;
    //start watchdog timer
    printf("watchdog reboot!\r\n");
    fd = ak_drv_wdt_open(feed_time);    
	if(0 != fd)
	{
		ak_print_error("open watchdog fail!\n");
		return ;
	}
	printf("watchdog stopped\n");
    ak_print_normal("\r\nOK\r\n");
}

/*
* Using system reset to reboot AK chip
*/
void ftest_cmd_reset_handler(char *arg, int length)
{
    printf("Software reset!\r\n");
    sys_reset();
    ak_print_normal("\r\nOK\r\n");
}


void ftest_cmd_tonegen_handler(char *arg, int length)
{
    ak_pthread_t thread_id;
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;
    int freq  =0;
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    if(strcmp(pStrProcess, "on") == 0)
    {
        if(tone_gen_get_running() == 0){
            tone_gen_set_on();
            pStrProcess = pNextStr;
            pNextStr = strchr(pStrProcess, ' ');
            if (pNextStr != NULL)
            {
                *pNextStr = '\0';
                pNextStr++;
            }
            freq = atoi(pStrProcess);
            ak_print_normal_ex("freq: %d\n", freq);
            iRet = ak_thread_create(&thread_id,  ftest_speaker_tone_generate_thread, pStrProcess, 16*1024, 10);
            ak_print_normal_ex("Created thread id: %u, iRet: %d\n", thread_id, iRet);
            if(iRet == 0)
            {
                ak_print_normal("\r\nOK\r\n");
            }
            else
            {
                ak_print_normal("\r\nFAILED\r\n");
            }
        }
        else
        {
            ak_print_normal_ex("Tone gen is running! Need to turn off before restarting!\n");
        }
    }
    else if(strcmp(pStrProcess, "off") == 0)
    {
        tone_gen_set_off();
        ak_print_normal("\r\nOK\r\n");
    }
    else
    {
        ak_print_normal("Command invalid.");
        ak_print_normal("\r\nFAILED\r\n");
        show_help(tonegen_help);
    }
}

// audio loopback handler
void ftest_cmd_audioloopback_handler(char *arg, int length)
{
    ak_pthread_t thread_id;
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;
    int freq  =0;
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    if(strcmp(pStrProcess, "on") == 0)
    {
        ftest_audioloop_start();
        pStrProcess = pNextStr;
        pNextStr = strchr(pStrProcess, ' ');
        if (pNextStr != NULL)
        {
            *pNextStr = '\0';
            pNextStr++;
        }
        
        // We use main thread audio and talkback for loopback
        // iRet = ak_thread_create(&thread_id,  ftest_audio_loopback_thread, NULL, 16*1024, 10);
        // ak_print_normal_ex("Created thread id: %u, iRet: %d\n", thread_id, iRet);
        // gFtestMonitorId[E_FCMD_TEST] = thread_id;
        if(iRet == 0){
            ak_print_normal("\r\nOK\r\n");
        }
        else{
            ak_print_normal("\r\nFAILED\r\n");
        }
    }
    else if(strcmp(pStrProcess, "off") == 0)
    {
        ftest_audioloop_stop();
        ak_print_normal("\r\nOK\r\n");
    }
    else
    {
        ak_print_normal("Command invalid.");
        ak_print_normal("\r\nFAILED\r\n");
        show_help(audio_loopback_help);
    }
}


void ftest_cmd_ntp_handler(char *arg, int length)
{
    ak_pthread_t thread_id;
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;
    long offset_second = 0;
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }
    offset_second = atoi(pStrProcess);
    ak_print_normal_ex("offset_second: %d\n", offset_second);
    iRet = ak_thread_create(&thread_id,  ftest_test_ntp_thread, pStrProcess, 16*1024, 10);
    ak_print_normal_ex("Created thread id: %u, iRet: %d\n", thread_id, iRet);
    if(iRet == 0)
    {
        ak_print_normal("\r\nOK\r\n");
    }
    else
    {
        ak_print_normal("\r\nFAILED\r\n");
        show_help(ntp_help);
    }

}

void ftest_cmd_config_handler(char *arg, int length)
{
    doorbell_write_all_config();
}

void ftest_cmd_fm2018_handler(char *arg, int length)
{
    fmcfg_atecmd_test();
}
void ftest_cmd_fmset_reg_handler(char *arg, int length)
{
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;

    char acReg[16];
    char acValue[16];

    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    // if(strcmp(pStrProcess, "on") == 0)
    if(strlen(pStrProcess) == 4)
    {
        memcpy(acReg, pStrProcess, 4);
        acReg[4] = '\0';
        ak_print_normal_ex("Reg: %s\n", acReg);
        
        pStrProcess = pNextStr;
        pNextStr = strchr(pStrProcess, ' ');
        if (pNextStr != NULL)
        {
            *pNextStr = '\0';
            pNextStr++;
        }
        memcpy(acValue, pStrProcess, 4);
        acValue[4] = '\0';
    
        iRet = fmcfg_set_reg(acReg, acValue);
        if(iRet != 0)
        {
            ak_print_error_ex("Set reg failed! (%d))\r\n", iRet);
        }
    }
    else
    {
        ak_print_normal("Command invalid.");
        // ak_print_normal("\r\nFAILED\r\n");
        show_help(fmset_reg_help);
    }
}


void ftest_cmd_fmget_reg_handler(char *arg, int length)
{
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;

    char acReg[16];
    char acValue[16];
    unsigned short usValue = 0;

    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    // if(strcmp(pStrProcess, "on") == 0)
    if(strlen(pStrProcess) == 4)
    {
        // pStrProcess = pNextStr;
        // pNextStr = strchr(pStrProcess, ' ');
        // if (pNextStr != NULL)
        // {
        //     *pNextStr = '\0';
        //     pNextStr++;
        // }
        memcpy(acReg, pStrProcess, 4);
        acReg[4] = '\0';
        ak_print_normal_ex("Reg: %s\n", acReg);
    
        iRet = fmcfg_get_reg(acReg, &usValue);
        if(iRet != 0)
        {
            ak_print_error_ex("Get reg failed! (%d))\r\n", iRet);
        }
        else
        {
            ak_print_normal_ex("REG: %s %04X\r\n", acReg, usValue);
        }
    }
    else
    {
        ak_print_normal("Command invalid.");
        // ak_print_normal("\r\nFAILED\r\n");
        show_help(fmget_reg_help);
    }
}

/*
engineer will enter: i2cwrite <address> <register> <value>
*/
void ftest_cmd_i2c_write_handler(char *arg, int length)
{
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;

    char acAddress[8];
    char acReg[16];
    char acValue[16];
    unsigned short usValue = 0;

    void * handle;
    unsigned char dev_addr = 0;
    unsigned short reg_addr = 0;
    unsigned char data;

    /*
    1. parse address 2 characters, register, value
    2. open i2c handle
    3. write data
    */
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);
    if(length <= 0)
    {
        show_help(i2cwr_help);
        return;
    }

    memset(acAddress, 0x00, sizeof(acAddress));
    memset(acReg, 0x00, sizeof(acReg));
    memset(acValue, 0x00, sizeof(acValue));

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
    {
        *pNextStr = '\0';
        pNextStr++;
    }
    /* 2 bytes address */
    if(strlen(pStrProcess) == 2)
    {
        memcpy(acAddress, pStrProcess, 2);
        acAddress[2] = '\0';
        ak_print_normal_ex("Addr: %s\n", acAddress);
        // 4 bytes register
        pStrProcess = pNextStr; // shift pointer to next parameter
        pNextStr = strchr(pStrProcess, ' ');
        if (pNextStr != NULL)
        {
            *pNextStr = '\0';
            pNextStr++;
        }
        if(strlen(pStrProcess) == 4) // 4 bytes register
        {
            memcpy(acReg, pStrProcess, 4);
            acAddress[4] = '\0';
            // 4 bytes value
            pStrProcess = pNextStr; // shift pointer to next parameter
            pNextStr = strchr(pStrProcess, ' ');
            if (pNextStr != NULL)
            {
                *pNextStr = '\0';
                pNextStr++;
            }
            if(strlen(pStrProcess) == 2) // 4 bytes value
            {
                memcpy(acValue, pStrProcess, 2);
                acValue[2] = '\0';
                // everything is OK, let go
                dev_addr = atoi(acAddress);
                reg_addr = atoi(acReg);
                data = atoi(acValue);

                handle = ak_drv_i2c_open(dev_addr, 1);
                if(handle == NULL)
                {
                    ak_print_error_ex("Open device failed!\r\n");
                }
                else
                {
                    if(-1 != ak_drv_i2c_write(handle,reg_addr, data, 1))
                    {
                        ak_print_normal("\nwrite success! \n");
                    }
                    else
                    {
                        ak_print_error("\nwrite fails! \n");
                    }
                    ak_drv_i2c_close(handle);
                } // end of if(handle == NULL)
            }
            else
            {
                ak_print_normal("Command invalid.");
                show_help(i2cwr_help);
            }// end of else if(strlen(pStrProcess) == 4)
        } // if(strlen(pStrProcess) == 4)
        else
        {
            ak_print_normal("Command invalid.");
            show_help(i2cwr_help);
        } // end of else if(strlen(pStrProcess) == 4)
    }//if(strlen(pStrProcess) == 2)
    else
    {
        ak_print_normal("Command invalid.");
        show_help(i2cwr_help);
    }//end of else if(strlen(pStrProcess) == 2)
}

void ftest_cmd_i2c_read_handler(char *arg, int length)
{
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;

    char acAddress[8];
    char acReg[16];
    char acValue[16];
    unsigned short usValue = 0;

    void * handle;
    unsigned char dev_addr = 0;
    unsigned short reg_addr = 0;
    unsigned char data;

    /*
    1. parse address 2 characters, register, value
    2. open i2c handle
    3. write data
    */
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);
    if(length <= 0)
    {
        show_help(i2cwr_help);
        return;
    }

    memset(acAddress, 0x00, sizeof(acAddress));
    memset(acReg, 0x00, sizeof(acReg));
    memset(acValue, 0x00, sizeof(acValue));

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
    {
        *pNextStr = '\0';
        pNextStr++;
    }
    /* 2 bytes address */
    if(strlen(pStrProcess) == 2)
    {
        memcpy(acAddress, pStrProcess, 2);
        acAddress[2] = '\0';
        ak_print_normal_ex("Addr: %s\n", acAddress);
        // 4 bytes register
        pStrProcess = pNextStr; // shift pointer to next parameter
        pNextStr = strchr(pStrProcess, ' ');
        if (pNextStr != NULL)
        {
            *pNextStr = '\0';
            pNextStr++;
        }
        if(strlen(pStrProcess) == 4) // 4 bytes register
        {
            memcpy(acReg, pStrProcess, 4);
            acAddress[4] = '\0';
            // 4 bytes value
            pStrProcess = pNextStr; // shift pointer to next parameter
            pNextStr = strchr(pStrProcess, ' ');
            if (pNextStr != NULL)
            {
                *pNextStr = '\0';
                pNextStr++;
            }
            
            // everything is OK, let go
            dev_addr = atoi(acAddress);
            reg_addr = atoi(acReg);
            // data = atoi(acValue);

            handle = ak_drv_i2c_open(dev_addr, 1);
            if(handle == NULL)
            {
                ak_print_error_ex("Open device failed!\r\n");
            }
            else
            {
                if(-1 != ak_drv_i2c_read(handle,reg_addr, data, 1))
                {
                    ak_print_normal("\nread success! Reg: %x value: 0x%02x\n", reg_addr, data);
                }
                else
                {
                    ak_print_error("\nread fails! \n");
                }
                ak_drv_i2c_close(handle);
            } // end of if(handle == NULL)
        
        } // if(strlen(pStrProcess) == 4)
        else
        {
            ak_print_normal("Command invalid.");
            show_help(i2cwr_help);
        } // end of else if(strlen(pStrProcess) == 4)
    }//if(strlen(pStrProcess) == 2)
    else
    {
        ak_print_normal("Command invalid.");
        show_help(i2cwr_help);
    }//end of else if(strlen(pStrProcess) == 2)
}


/*---------------------------------------------------------------------------*/
/*                          GET/SET MIC AND SPEAKER                          */
/*---------------------------------------------------------------------------*/
/*
This function will search string
Only use to parse get or set
If it match token, return 0, otherwise return -1
*/
static int ftest_parse_string(char *pToken, char *pInput, char *pValue)
{
    int iRet = 0;
    char *pPos = NULL;
    char *pArg = NULL;
    int iValue = 0;
    if(pToken == NULL || pInput == NULL)
    {
        ak_print_error_ex("Parameter is NULL: pToken: %p pInput: %p", \
                pToken, pInput);
        return -1;
    }

    pPos = strstr(pInput, pToken);
    if(pPos == NULL)
    {
        iRet = -1;
    }
    else
    {
        if(pValue != NULL)
        {
            pArg = strchr(pPos, ' ');
            if(pArg != NULL)
            {
                pArg++;
                iValue = atoi(pArg);
                if(iValue >=0 && iValue <= 12)
                {
                    *pValue = (char)iValue;
                }
                else
                {
                    iRet = -1;
                }
            }
        }
    }
    return iRet;
}
/*
This function will parse arg get or set config
*/
void ftest_cmd_mic_digital_handler(char *arg, int length)
{
    int iRet = 0;
    int cIsCmdRet = 0;
    char cValue = 0;
    int iValueGet = 0;
    ak_print_normal_ex("length: %d, arg: %s\n", length, arg);

    cIsCmdRet = ftest_parse_string("get", arg, NULL);
    if(cIsCmdRet == 0) // command get
    {
        // read data and show value
        iRet = doorbell_cfg_get_micd_gain(&iValueGet);
        if(iRet != 0)
        {
            ak_print_error_ex("Get value MIC Digital failed! (%d)\r\n", iRet);
        }
        else
        {
            ak_print_normal("MIC Digital: %d\r\n", iValueGet);
        }
    }
    else // check command set
    {
        cIsCmdRet = ftest_parse_string("set", arg, &cValue);
        if(cIsCmdRet == 0)
        {
            // set data to flash
            iRet = doorbell_cfg_set_micd_gain((int)cValue);
            if(iRet != 0)
            {
                ak_print_error_ex("Set value MIC Digital failed! (%d)\r\n", iRet);
            }
            else
            {
                ak_print_normal("Set MIC Digital OK\r\n");
            }
            if(ftest_mic_set_volume_onflight_with_value((int)cValue) != 0)
            {
                ak_print_error_ex("Set micd vol %d failed!\r\n", cValue);
            }
            else
            {
                ak_print_error_ex("Set micd vol %d OK!\r\n", cValue);
            }
        }
        else
        {
            ak_print_normal("Command invalid.");
            show_help(micd_help);
        }
    }
}
void ftest_cmd_mic_analog_handler(char *arg, int length)
{
    int iRet = 0;
    int cIsCmdRet = 0;
    char cValue = 0;
    int iValueGet = 0;
    ak_print_normal_ex("length: %d, arg: %s\n", length, arg);

    cIsCmdRet = ftest_parse_string("get", arg, NULL);
    if(cIsCmdRet == 0) // command get
    {
        // read data and show value
        iRet = doorbell_cfg_get_mica_gain(&iValueGet);
        if(iRet != 0)
        {
            ak_print_error_ex("Get value MIC Analog failed! (%d)\r\n", iRet);
        }
        else
        {
            ak_print_normal("MIC Analog: %d\r\n", iValueGet);
        }
    }
    else // check command set
    {
        cIsCmdRet = ftest_parse_string("set", arg, &cValue);
        if(cIsCmdRet == 0)
        {
            // set data to flash
            iRet = doorbell_cfg_set_mica_gain((int)cValue);
            if(iRet != 0)
            {
                ak_print_error_ex("Set value MIC Analog failed! (%d)\r\n", iRet);
            }
            else
            {
                ak_print_normal("Set MIC Analog OK\r\n");
            }
            if(ftest_mic_set_volume_onflight_with_value((int)cValue) != 0)
            {
                ak_print_error_ex("Set mica vol %d failed!\r\n", cValue);
            }
            else
            {
                ak_print_error_ex("Set mica vol %d OK!\r\n", cValue);
            }
        }
        else
        {
            ak_print_normal("Command invalid.");
            show_help(mica_help);
        }
    }
}
void ftest_cmd_spk_digital_handler(char *arg, int length)
{
    int iRet = 0;
    int cIsCmdRet = 0;
    char cValue = 0;
    int iValueGet = 0;
    ak_print_normal_ex("length: %d, arg: %s\n", length, arg);

    cIsCmdRet = ftest_parse_string("get", arg, NULL);
    if(cIsCmdRet == 0) // command get
    {
        // read data and show value
        iRet = doorbell_cfg_get_spkd_gain(&iValueGet);
        if(iRet != 0)
        {
            ak_print_error_ex("Get value SPK digital failed! (%d)\r\n", iRet);
        }
        else
        {
            ak_print_normal("SPK Digital: %d\r\n", iValueGet);
        }
    }
    else // check command set
    {
        cIsCmdRet = ftest_parse_string("set", arg, &cValue);
        if(cIsCmdRet == 0)
        {
            // set data to flash
            iRet = doorbell_cfg_set_spkd_gain((int)cValue);
            if(iRet != 0)
            {
                ak_print_error_ex("Set value SPK Digital failed! (%d)\r\n", iRet);
            }
            else
            {
                ak_print_normal("Set SPK Digital OK\r\n");
            }
            if(ftest_speaker_set_volume_onflight_with_value((int)cValue) != 0)
            {
                ak_print_error_ex("Set speakerd vol %d failed!\r\n", cValue);
            }
            else
            {
                ak_print_error_ex("Set speakerd vol %d OK!\r\n", cValue);
            }
        }
        else
        {
            ak_print_normal("Command invalid.");
            show_help(spkd_help);
        }
    }
}

void ftest_cmd_spk_analog_handler(char *arg, int length)
{
    int iRet = 0;
    int cIsCmdRet = 0;
    char cValue = 0;
    int iValueGet = 0;
    ak_print_normal_ex("length: %d, arg: %s\n", length, arg);

    cIsCmdRet = ftest_parse_string("get", arg, NULL);
    if(cIsCmdRet == 0) // command get
    {
        // read data and show value
        iRet = doorbell_cfg_get_spka_gain(&iValueGet);
        if(iRet != 0)
        {
            ak_print_error_ex("Get value SPK Analog failed! (%d)\r\n", iRet);
        }
        else
        {
            ak_print_normal("SPK Analog: %d\r\n", iValueGet);
        }
    }
    else // check command set
    {
        cIsCmdRet = ftest_parse_string("set", arg, &cValue);
        if(cIsCmdRet == 0)
        {
            // set data to flash
            iRet = doorbell_cfg_set_spka_gain((int)cValue);
            if(iRet != 0)
            {
                ak_print_error_ex("Set value SPK Analog failed! (%d)\r\n", iRet);
            }
            else
            {
                ak_print_normal("Set SPK Analog OK\r\n");
            }
            if(ftest_speaker_set_volume_onflight_with_value((int)cValue) != 0)
            {
                ak_print_error_ex("Set speakera vol %d failed!\r\n", cValue);
            }
            else
            {
                ak_print_error_ex("Set speakera vol %d OK!\r\n", cValue);
            }
        }
        else
        {
            ak_print_normal("Command invalid.");
            show_help(spka_help);
        }
    }
}


void ftest_cmd_sddetect_handler(char *arg, int length)
{
    int *fd;
    int event;
    unsigned long ulTick = 0;
    
    ulTick = get_tick_count();

    fd = ak_drv_detector_open(SD_DETECTOR); //open SD detect
	if(NULL == fd)
	{
		ak_print_error("open detector fail\n");
		return ;
	}
	
	if(-1 == ak_drv_detector_mask(fd, 1))
	{
		ak_print_error("open detector enable fail\n");
		return ;
	}
	while(1)
	{
		if(-1 != ak_drv_detector_poll_event(fd, &event))   //get SD detect status
		{
            if(event)
            {
                ak_print_normal("sd have\n");
            }
            else 
            {
                ak_print_normal("no sd\n");
            }
		}
        else
        {
            ak_print_error("no get\n");
        }
        
        if(get_tick_count() - ulTick >= (20*1000))
        {
            printf(".");
            break;
        }
        ak_sleep_ms(100);
	}
	ak_drv_detector_close(fd);
}


void ftest_cmd_spktest_handler(char *arg, int length)
{
    int iRet = 0;
    ak_pthread_t thread_id;
    iRet = ak_thread_create(&thread_id,  ftest_speaker_thread, NULL, 16*1024, 10);
}

void ftest_cmd_led_handler(char *arg, int length)
{
    // ak_pthread_t thread_id;
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;
    int freq  =0;
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    if(strcmp(pStrProcess, "on") == 0)
    {
        atecmdled_set_on();
        led_init();
        led_turn_on();
        ak_print_normal("\r\nOK\r\n");
    }
    else if(strcmp(pStrProcess, "off") == 0)
    {
        led_init();
        led_turn_off();
        ak_print_normal("\r\nOK\r\n");
    }
    else
    {
        ak_print_normal("Command invalid.");
        ak_print_normal("\r\nFAILED\r\n");
        show_help(led_help);
    }
}

void ftest_cmd_write_serialnumber_handler(char *arg, int length)
{
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrSn = arg;
    int len = 0;

    pNextStr = strchr(pStrSn, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    len = strlen(pStrSn);
    if(len > 0 && len < 50)
    {
        // write string to flash
        iRet = dbcfg_write_string_to_factory_config(FACTORY_ITEM_SERI, pStrSn, len);
        if(iRet == 0)
        {
            ak_print_normal("\nOK\r\n");
        }
        else
        {
            ak_print_normal("\nfail\r\n");
        }
    }
    else
    {
        if(len >= 50)
        {
            ak_print_normal("Length of SN too long!\n");
        }
        ak_print_normal("Command invalid.");
        ak_print_normal("\r\nFAILED\r\n");
        show_help(led_help);
    }
}

void ftest_cmd_read_serialnumber_handler(char *arg, int length)
{
    int iRet = 0;
    unsigned char ucLen = 0;
    char strSn[64];
    memset(strSn, 0x00, 64);
    // write string to flash
    iRet = dbcfg_read_string_to_factory_config(FACTORY_ITEM_SERI, strSn, &ucLen);
    if(iRet == 0)
    {
        if(ucLen > 0 && ucLen < 50)
        {
            ak_print_normal("\n%s\r\n", strSn);
            ak_print_normal("OK\r\n");
        }
        else
        {
            ak_print_normal("\nfail\r\n");
        }
    }
    else
    {
        ak_print_normal("\nfail\r\n");
    }

}
void ftest_cmd_datecode_handler(char *arg, int length)
{
    int iRet = 0;
    unsigned char ucLen = 0;
    char strSn[64];
    char *pNextStr = NULL;
    char *pStrProcess = arg;
    char acTemp[5] = {0x00};
    int iYear = 0;
    int iMonth = 0;
    int iDay = 0;

    ak_print_normal("Strlen arg: %d, length: %d, arg: %s\r\n", strlen(arg), length, arg);
    memset(strSn, 0x00, 64);
    
    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    // if(pStrProcess == NULL) // there is no character to parse
    if(length == 1)
    {
        // ak_print_normal("*arg: %x\n", *arg);
        iRet = dbcfg_read_string_to_factory_config(FACTORY_ITEM_DATECODE, strSn, &ucLen);
        if(iRet == 0)
        {
            if(ucLen > 0 && ucLen < 50)
            {
                // ak_print_normal("\n%s\r\n", strSn);
                ak_print_normal("OK\r\n");
            }
            else
            {
                ak_print_normal("len >0 <50\r\n");
                ak_print_normal("\nfail\r\n");
            }
        }
        else
        {
            ak_print_normal("read config failed\r\n");
            ak_print_normal("\nfail\r\n");
        }
    }
    else
    {
        // parse year, month, day
        if(strlen(pStrProcess)!= 8)
        {
            ak_print_normal("strlen != 8\r\n");
            ak_print_normal("\nfail\r\n");
        }
        else
        {
            memset(acTemp, 0x00, sizeof(acTemp));
            memcpy(acTemp, pStrProcess, 4);
            *(acTemp + 4) = '\0';
            ak_print_normal("str year: %s, %s ", acTemp, pStrProcess);
            iYear = atoi(acTemp);
            if(iYear < 1970 || iYear > 2100)
            {
                ak_print_normal("year: %d\r\n", iYear);
                ak_print_normal("\nfail\r\n");
            }
            else
            {
                memset(acTemp, 0x00, sizeof(acTemp));
                memcpy(acTemp, pStrProcess + 4, 2);
                *(acTemp + 2) = '\0';
                ak_print_normal("str month: %s ", acTemp);
                iMonth = atoi(acTemp);
                if(iMonth < 1 || iMonth > 12)
                {
                    ak_print_normal("month: %d\r\n", iMonth);
                    ak_print_normal("\nfail\r\n");
                }
                else
                {
                    memset(acTemp, 0x00, sizeof(acTemp));
                    memcpy(acTemp, pStrProcess + 4 + 2, 2);
                    *(acTemp + 2) = '\0';
                    ak_print_normal("str date: %s ", acTemp);
                    iDay = atoi(acTemp);
                    if(iDay < 1 || iDay > 31)
                    {
                        ak_print_normal("day: %d\r\n", iDay);
                        ak_print_normal("\nfail\r\n");
                    }
                    else
                    {
                        // OK
                        // write string to flash
                        iRet = dbcfg_write_string_to_factory_config(FACTORY_ITEM_DATECODE, pStrProcess, 8);
                        if(iRet == 0)
                        {
                            ak_print_normal("\nOK\r\n");
                        }
                        else
                        {
                            ak_print_normal("\nfail\r\n");
                        }
                    }//if(iDay < 1 || iDay > 31)
                }//if(iMonth < 1 || iMonth > 12)
            }//if(iYear < 1970 || iYear > 2100)
        }//if(strlen(pStrProcess)!= 8)
    }//if(pStrProcess == NULL)    
}



char ftest_uvc_get_state(void)
{
    return g_cUVC_turn_on;
}


void ftest_cmd_uvc_handler(char *arg, int length)
{
    // ftest_cmd_uvc_test(0);
    ak_pthread_t thread_id;
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;

    //TODO: disable video
    g_cUVC_turn_on = 1;
    ak_sleep_ms(3000); // delay wait video task exit
    
    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    if(strcmp(pStrProcess, "on") == 0)
    {
        if(0 == videouvc_get_status())
        {
            videouvc_set_enable();
            iRet = ak_thread_create(&thread_id,  ftest_cmd_uvc_test, NULL, 16*1024, 10);
        }
        else
        {
            ak_print_normal("UVC is running!\r\n");
        }
    }
    else if(strcmp(pStrProcess, "off") == 0)
    {
        if(1 == videouvc_get_status())
        {
            videouvc_set_disable();
        }
        else
        {
            ak_print_normal("UVC is not running!\r\n");
        }
    }
    else
    {
        ak_print_normal("Command invalid.");
        ak_print_normal("\nfail\r\n");
        show_help(videouvc_help);
    }
    
}


void ftest_cmd_agc_handler(char *arg, int length)
{
    ak_pthread_t thread_id;
    int iRet = 0;
    char *pNextStr = NULL;
    char *pStrProcess = arg;
    int freq  =0;
    ak_print_normal("%s\n", __func__);
    ak_print_normal("length: %d\n", length);
    ak_print_normal("arg: %s\n", arg);

    pNextStr = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }

    if(strcmp(pStrProcess, "on") == 0)
    {
    	// assignment a global variable
    }
    else if(strcmp(pStrProcess, "off") == 0)
    {
    	// assignment a global variable       
    }
    else if(strcmp(pStrProcess, "get") == 0)
    {
    	// show info
    }
	else if(strcmp(pStrProcess, "set") == 0)
    {
    	// set agc parameter
    }
	else if(strcmp(pStrProcess, "time") == 0)
    {
    	// set time for algorithm
    }
	else
	{
		// invalid command
	}
}



/*---------------------------------------------------------------------------*/
/*                          FTEST STRUCTURE                                  */
/*---------------------------------------------------------------------------*/


// Ftest
int ftest_get_cmd(void)
{
    char getStringConsole[FTEST_LEN_BUF_CONSOLE];
    char *pNextStr = NULL;
    char *pStrProcess = getStringConsole;
    char *pCmdArg = NULL;
    unsigned int length = 0;
    int iRet = 0;
    int i = 0;
    static int iCntCmd = 0;

    memset(getStringConsole, 0x00, sizeof(getStringConsole));
    ak_gets(getStringConsole, FTEST_LEN_BUF_CONSOLE); 

    if(strlen(getStringConsole) == 0)
    {
        return E_FCMD_MAX;
    }

    pNextStr  = strchr(getStringConsole, ' ');
	if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }
    else
    {
        // nothing to do
    }
    // check prefix atecmd
    if(strcmp(pStrProcess, FTEST_PREFIX_CMD) != 0)
    {
        return -1;
    }

    // find cmd
    pStrProcess = pNextStr;
    pNextStr  = strchr(pStrProcess, ' ');
    if (pNextStr != NULL)
	{
		*pNextStr = '\0';
		pNextStr++;
    }
    else
    {
        // nothing to do
    }

    for(i = 0; i < E_FCMD_MAX; i++)
    {
        if(strcmp(pStrProcess, g_aTableFtestCmd[i].name) != 0)
        {
            // next command
        }
        else
        {
            memset(g_aTableFtestCmd[i].arg, 0x00, sizeof(g_aTableFtestCmd[i].arg));
            pStrProcess = pNextStr;
            if(strlen(pStrProcess) != 0)
            {
                length = strlen(pNextStr);
                memcpy(g_aTableFtestCmd[i].arg, pStrProcess, strlen(pStrProcess));
            }
            else
            {
                // no argument
            }
            break;
        }
    }

    if(i == E_FCMD_MAX)
    {
        iRet = -2;
    }
    else
    {
        iRet = i;
        // g_aTableFtestCmd[i].cmd_handle(g_aTableFtestCmd[i].arg, strlen(pStrProcess));
    }
    // ak_print_normal_ex("iRet: %d\n", iRet);
    return iRet;
}

int ftest_loop_process_cmd(void)
{
    int iRet = 0;
    // iRet = cmdif_get_cmd();
    iRet = ftest_get_cmd();
    return iRet;
}


void ftest_reboot_wdt(void)
{
    unsigned int fd = 0;
    unsigned int feed_time = 100;
    //start watchdog timer
    fd = ak_drv_wdt_open(feed_time);    
	if(0 != fd)
	{
		ak_print_error("open watchdog fail!\n");
		return ;
	}
    // ak_drv_wdt_close();    //close watchdog
	ak_print_normal("watchdog stopped\n");
}

// start this thread and waiting command from UART
void* ftest_main_process(void *arg)
{
    ak_pthread_t ftest_id;
    ak_pthread_t  ftest_audio_loop_id;
    ak_pthread_t ftest_spkear_id;

    int ret = 0;
    int cnt = 0;
    unsigned long len_read = 0;
    int cmd_index = 0;

    memset(gFtestMonitorId, 0x00, sizeof(gFtestMonitorId));
    // kill all tasks and enter ftest mode
    // main_set_flag_ftest();
    vSysModeSet(E_SYS_MODE_FTEST);

    ak_sleep_ms(1000);
    ak_print_normal("%s ftest killed all tasks\n", __func__);
    while(1)
    {
        ak_sleep_ms(20);
        cmd_index = ftest_loop_process_cmd();
        if(cmd_index == E_FCMD_MAX)
        {
            printf(">");
        }
        else if(cmd_index < 0 || cmd_index > E_FCMD_MAX)
        {
            printf("Invalid command!\n");
            ak_print_normal("\r\nFAILED\r\n");
        }
        else
        {
            g_aTableFtestCmd[cmd_index].cmd_handle( \
                    g_aTableFtestCmd[cmd_index].arg, \
                    strlen(g_aTableFtestCmd[cmd_index].arg));
            printf(">");
        }
    }
    return NULL;
}

static int init_param(int argc, char **args )
{      
    return 0;
}


void cmd_ftest(int argc, char **args)
{
	ak_pthread_t  ftest_id;

    if (0 != init_param(argc, args))
        return ;

    ak_thread_create(&ftest_id, ftest_main_process, NULL, 32*1024, CFG_FTEST_TASK_PRIO);
	ak_print_normal("Ftest module was started ...\n");
    while(1)
    {
        ak_sleep_ms(1000);
    }
	ak_thread_join(ftest_id);
}


static int cmd_ftest_reg(void)
{
    cmd_register("ftest", cmd_ftest, help);
    return 0;
}

cmd_module_init(cmd_ftest_reg)


/*
***********************************************************************
*                           END OF FILES                              *
***********************************************************************
*/
