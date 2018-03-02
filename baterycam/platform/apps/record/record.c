/**
  ******************************************************************************
  * File Name          : record.c
  * Description        :
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 CINATIC LTD COMPANY
  */


/*---------------------------------------------------------------------------*/
/*                            INCLUDED FILES                                 */
/*---------------------------------------------------------------------------*/

#include <string.h>
#include "record.h"

#include "ak_apps_config.h"
#include "drv_gpio.h"
#include "ak_apps_config.h"
#include "command.h"
#include "sys_common.h"
#include "ak_common.h"
#include "ak_partition.h"
#include "kernel.h"
#include "main_ctrl.h"
#include "spi_transfer.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                   GLOBAL FUNCTIONS/PROCEDURES                             */
/*---------------------------------------------------------------------------*/

/** 
 * record_ctrl_init - init record control env and other sub-modules. 
 *      start record control thread.
 * @vi_handle[IN]: success opened video input handle
 * @ai_handle[IN]: success opened audio input handle
 * @file_type[IN]: record file type
 * return: none
 */
ak_pthread_t record_ctrl_init(void *vi_handle, void *ai_handle,
    enum record_file_type file_type , struct video_config * video_config)
{

    g_video_config = * video_config;

    ak_print_normal("\n---------------------------------------------------\n");
    file_type = RECORD_FILE_TYPE_MP4;
    switch(file_type) {
        case RECORD_FILE_TYPE_MP4:
            ak_print_notice_ex("^^^^^ record file type is mp4 ^^^^^\n");
            break;
        case RECORD_FILE_TYPE_AVI:
            ak_print_notice_ex("^^^^^ record file type is avi ^^^^^\n");
            break;
        default:
            ak_print_error_ex("^^^^^ unsupport record file_type=%d ^^^^^\n", file_type);
            return;
    }

    record_ctrl.run_flag = AK_TRUE;
    record_ctrl.alarm_init_flag = AK_FALSE;
    record_ctrl.except_type = (RECORD_EXCEPT_SD_REMOVED | RECORD_EXCEPT_SD_UMOUNT);
    //	record_ctrl.detect_type = SYS_DETECT_TYPE_NUM; //lgd remove

    /* plan record info */
    record_ctrl.plan.status = RECORD_PLAN_NONE;
    record_ctrl.plan.index = INVALID_VALUE;
    record_ctrl.plan.start_wday = INVALID_VALUE;
    record_ctrl.plan.start_time = 0;
    record_ctrl.plan.end_time = 0;
    record_ctrl.plan.next_time = 0;

    int ret = AK_FAILED;
    record_ctrl.vi_handle = vi_handle;
    record_ctrl.ai_handle = ai_handle;

    init_record_common(file_type);
    ak_pthread_t record_ctrl_thread;
    ret = ak_thread_create(&record_ctrl_thread, record_ctrl_main,
                            NULL, ANYKA_THREAD_MIN_STACK_SIZE, 95);
    if(AK_FAILED == ret)
    {
        ak_print_normal_ex("unable to create record_ctrl_main thread, ret = %d!\n", ret);
        record_ctrl.run_flag = AK_FALSE;
    }
    return record_ctrl_thread;
}



/*---------------------------------------------------------------------------*/
/*                          END OF FILES                                     */
/*---------------------------------------------------------------------------*/
