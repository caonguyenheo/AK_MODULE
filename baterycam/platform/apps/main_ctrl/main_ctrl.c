/**
  ******************************************************************************
  * File Name          : main_ctrl.c
  * Description        :
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 CINATIC LTD COMPANY
  */


/*
*******************************************************************************
*                            INCLUDED FILES                                   *
*******************************************************************************
*/

#include "ak_apps_config.h"
#include "ak_video.h"
#include "ak_audio.h"
#include "aes_sw.h"
#include "packet.h"

// #include "p2p_config.h"
#include "spi_transfer.h"
#include "debug_latency.h"
#include "ftest.h"
#include "ak_talkback.h"
#include "packet_queue.h"

#include "spi_cmd.h"

#include "ak_login.h"
#include "doorbell_config.h"

#include "ak_partition.h"

#include "fwupgrade.h"
#include "upload_main.h"
#include "idle.h"
#include "rec.h"

#include "p2p_sdcard_streaming.h"
#include "led_cds.h"

#include "sdcard.h"

//#include "akos_api.h"
//#include "AKerror.h"

/*
*******************************************************************************
*                            DEFINITIONS                                      *
*******************************************************************************
*/

#define SAVE_PATH        "a:"
#define FILE_NAME        "video.h264"

#define FIRST_PATH       "ISPCFG"
#define BACK_PATH        "ISPCFG"

extern char fw_version[];

static char *help[]={
	"doorbell demo module",
	"usage:doorbell\n"
};


ak_pthread_t g_video_id, g_audio_id, g_spi_cmd_id, g_spi_id, g_test_id, \
        g_dbltc_id, g_packet_queue_id, g_talkback_id, g_spicmd_id, \
        g_takesnapshot_id, g_fwupg_id, g_uploader_id, g_p2psdstream_id, \
        g_sdcard_task_id;
ak_pthread_t g_ftest_id;

#ifdef CFG_DEBUG_VIDEO_WITH_SDCARD
ak_pthread_t g_db300id;
#endif


int g_killall_task = 0; // only ftest set this variable

int enc_count;  //encode this number frames

/*globle flag for video reques*/
int video_stop_flag = 0; 
int sys_idle_flag = 0;

// test upgrade upflag partition
#define UPGRADE_BUF_SIZE	(4*1024)

/* define a variable for mode action */
static ENUM_SYS_MODE_t g_eSysModeDoorbell = E_SYS_MODE_NORMAL;

/*
*******************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                               *
*******************************************************************************
*/
void vSysModeSet(ENUM_SYS_MODE_t eSysMode)
{
    if(eSysMode != g_eSysModeDoorbell)
    {
        ak_print_normal("SYS change mode: %d -> %d\n", g_eSysModeDoorbell, \
                        eSysMode);
        g_eSysModeDoorbell = eSysMode;
    }
}
ENUM_SYS_MODE_t eSysModeGet(void)
{
    return g_eSysModeDoorbell;
}

void main_set_flag_ftest(void)
{
    g_killall_task = 1;
}
int main_get_flag_ftest(void)
{
    return g_killall_task;
}


static void main_show_info(void)
{
    //#ifndef UPLOADER_DEBUG
    unsigned long idle = 0;//idle_get_cpu_idle();
    unsigned long usage = idle_get_cpu_usage();
    unsigned long total_ram = Fwl_GetTotalRamSize();
    unsigned long used_ram = Fwl_GetUsedRamSize();
    unsigned long remain_ram = Fwl_GetRemainRamSize();

	idle = 100 - usage;
    printf("Idle: %u %%, usage: %u %%, RAM: use %u/%u (avail %u)\n", \
       idle, usage, used_ram, total_ram, remain_ram);
    //printf("Idle: %u%%, usage: %u%%, RAM: use %5.2lfKB/%5.2lfKB (%5.2lfKB)\n", \
    //	idle, usage, (float)used_ram/1024, (float)total_ram/1024, (float)remain_ram/1024);
    //#endif
}

static void model_info(void)
{
    ak_print_normal("\r\n------------->>>This is ModelID %s."\
        "Powered by Premielink.com !!!!------------->>>\n", MODEL_ID_STR);
    ak_print_normal_ex("Release version: %s\n", fw_version);
    ak_print_normal_ex("Local dev version: %s\r\n", LOCAL_VERSION_STR);
    ak_print_normal("---------------------------------------------"\
        "--------------------------------------\r\n");
}

/*
*main process to start up
*/
void* main_process(void *arg)
{
    int ret = 0;
    model_info();
    ak_print_error("Time start %u\r\n", get_tick_count());
    led_init();
    prepare_data_play_dingdong();

    /* Init config */
	doorbell_config_init_ini();

    ak_print_normal("time 1 %d \n", get_tick_count()); //this will print current time in ms
    ak_thread_create(&g_spi_id,  spi_task, NULL, 32*1024, CFG_SPI_TASK_PRIO);
	
	ak_thread_create(&g_uploader_id, uploader_task_new_push, NULL, 64*1024, 40);
	
// #ifndef DOORBELL_TINKLE820
#if (RECORDING_SDCARD == 1) 
    ak_print_normal("Running FLV recorder task %d\n", StreamMgrRecordingRun(32*1024, CFG_FLV_RECORDER_TASK_PRIO));
#endif


    ak_thread_create(&g_video_id,  ak_video_encode_task, NULL, 64*1024, CFG_VIDEO_TASK_PRIO);

    ak_thread_create(&g_takesnapshot_id,  ak_video_take_snapshot_task_2, NULL, 32*1024, CFG_SNAPSHOT_TASK_PRIO);


    packet_init_mutex(); // only init mutex before video, audio send
    
    ret = init_aes_engine();
    ak_print_normal("time 2 %d \n", get_tick_count());
    ak_print_normal_ex("init aes (%d)\n", ret);
    
    // ak_thread_create(&g_audio_id,  ak_audio_encode_task, NULL, 32*1024, CFG_AUDIO_TASK_PRIO);
    ak_thread_create(&g_audio_id, ak_audio_test_driver_task, NULL, 32*1024, CFG_AUDIO_TASK_PRIO);
    ak_thread_create(&g_talkback_id, ak_talkback_task, NULL, 32*1024, CFG_TALKBACK_TASK_PRIO);
  
    #ifdef CFG_SPI_COMMAND_ENABLE
    ak_thread_create(&g_spicmd_id, spi_command_task, NULL, 32*1024, CFG_SPI_CMD_TASK_PRIO);
    #endif

    ak_thread_create(&g_fwupg_id, fwupgrade_task, NULL, 32*1024, CFG_FWUPG_TASK_PRIO);

    // ak_thread_create(&g_uploader_id, uploader_task, NULL, 64*1024, 40);
	

#if (SDCARD_ENABLE == 1)
    ak_thread_create(&g_p2psdstream_id, p2p_sdcard_streaming_task, NULL, 32*1024, 40);
#endif

#if (SDCARD_ENABLE == 1)
    ak_thread_create(&g_sdcard_task_id, sdcard_main_process, NULL, 32*1024, 30);
#endif

    while(1)
    {
        ak_sleep_ms(1000);
        if(eSysModeGet() != E_SYS_MODE_FTEST)
        {
            main_show_info();
        }
        if(main_get_flag_ftest() == 1)
        {
           break;
        }
    }

}

/* regist module start into _initcall */
static int main_ctrl(void) 
{
    ak_pthread_t id;

    ak_login_all_filter();
	ak_login_all_encode();
	ak_login_all_decode();

    ak_thread_create(&id, main_process, 0, 32*1024, CFG_MAIN_TASK_PRIO);
    ak_print_normal("Main control was created!\n");
    // ak_thread_join(id);
    return 0;
}
#if 0
static int cmd_doorbell_reg(void)
{
    cmd_register("doorbell", main_ctrl, help);
    return 0;
}
cmd_module_init(cmd_doorbell_reg)
#else
cmd_module_init(main_ctrl)
#endif


/*
*******************************************************************************
*                           END OF FILES                                      *
*******************************************************************************
*/
