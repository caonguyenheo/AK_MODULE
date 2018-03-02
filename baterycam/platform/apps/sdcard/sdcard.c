/**
  ******************************************************************************
  * File Name          : sdcard.c
  * Description        : 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/

#include "sdcard.h"
#include <stdlib.h>
#include "ak_apps_config.h"
#include "ak_drv_detector.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
E_SDCARD_STATE_t g_eSdcardStatus = E_SDCARD_STATUS_NO_DETECT;

/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/
E_SDCARD_STATE_t sdcard_get_status(void)
{
    return g_eSdcardStatus;
}

int sdcard_mount(void)
{
    int iRet = 0;
    iRet = ak_mount_fs(1, 0, "a:");
    g_eSdcardStatus = E_SDCARD_STATUS_MOUNTED;
    return iRet;
}
int sdcard_umount(void)
{
    int iRet = 0;
    iRet = ak_unmount_fs(1, 0, "a:");
    g_eSdcardStatus = E_SDCARD_STATUS_UMOUNT;
    return iRet;
}

void* sdcard_main_process(void *arg)
{
    int *fd;
    int event;

    // auto mount
    fd = ak_drv_detector_open(SD_DETECTOR); //open SD detect
	if(NULL == fd)
	{
		ak_print_error("open detector fail\n");
		return;
	}
	
	if(-1 == ak_drv_detector_mask(fd, 1))
	{
		ak_print_error("open detector enable fail\n");
        ak_drv_detector_close(fd);
        return;
    }
    
    // check status of SDcard
    while(1)
	{
		if(-1 != ak_drv_detector_poll_event(fd, &event))   //get SD detect status
		{
            if(event)
            {
                // ak_print_normal("sd have\n");
                if((g_eSdcardStatus != E_SDCARD_STATUS_DETECTED) && \
                    (g_eSdcardStatus != E_SDCARD_STATUS_MOUNTED))
                {
                    ak_print_normal("Change state of SD to E_SDCARD_STATUS_DETECTED\n");
                    g_eSdcardStatus = E_SDCARD_STATUS_DETECTED;
                }
            }
            else 
            {
                if(g_eSdcardStatus != E_SDCARD_STATUS_NO_DETECT)
                {
                    ak_print_normal("No SD\n");
                    g_eSdcardStatus = E_SDCARD_STATUS_NO_DETECT;
                }
            }
		}
        else
        {
            ak_print_error("cannot detect SDcard\n");
        }
        
        ak_sleep_ms(100);
	}
    ak_drv_detector_close(fd);
    ak_thread_exit();

}


/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/
