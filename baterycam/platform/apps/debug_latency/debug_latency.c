/**
  ******************************************************************************
  * File Name          : debug_latency.c
  * Description        : This file contain functions for debug latency
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of NXCOMM PTE LTD nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/

#include "debug_latency.h"
#include "ak_apps_config.h"
#include "drv_gpio.h"

#include <stdlib.h>

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

// static unsigned long    g_ulStartTime = 0;
static DB_LATENCY_t        g_sDbLtc;

/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/
int dbltc_start_time(DB_LATENCY_t *psDebugLatency)
{
    int i32Ret = 0;
    
    if(psDebugLatency == NULL)
    {
        apps_error("psDebugLatency NULL!");
        return -1;
    }
    if(psDebugLatency->running == DBLTC_NOT_RUNNING)
    {
        psDebugLatency->start_time = get_tick_count();
        psDebugLatency->running == DBLTC_RUNNING;
    }
    else
    {
        apps_warn("This id is running!");
        i32Ret = 1;
    }
    return i32Ret;
}

int dbltc_stop_time(DB_LATENCY_t *psDebugLatency)
{
    int i32Ret = 0;
    if(psDebugLatency == NULL)
    {
        apps_error("psDebugLatency NULL!");
        return -1;
    }
    if(psDebugLatency->running == DBLTC_RUNNING)
    {
        psDebugLatency->end_time = get_tick_count();
        psDebugLatency->running == DBLTC_NOT_RUNNING;
    }
    else
    {
        apps_warn("This id is NOT running!");
        i32Ret = 1;
    }
    return i32Ret;
}

int dbltc_start(void)
{
    return dbltc_start_time(&g_sDbLtc);
}
void *dbltc_thread(void *arg)
{
    int i32Ret = 0;
    // config interrupt from CC3200
    gpio_set_pin_as_gpio(SPI_CC3200_READ_INT);
	gpio_set_pin_dir(SPI_CC3200_READ_INT, 0); // input
    gpio_int_control(SPI_CC3200_READ_INT, 1);
    while(1)
    {
        i32Ret = dbltc_stop_time(&g_sDbLtc);
        if(i32Ret == 0)
        {
            apps_warn("STOP DBLTC! time:%d\n", g_sDbLtc.end_time - g_sDbLtc.start_time);
        }
        ak_sleep_ms(10);
    }
}



/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/
