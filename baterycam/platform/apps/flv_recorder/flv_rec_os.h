/*
 * Copyright (C) 2017 Cinatic
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* FILE   : flv_rec_os.h
 * AUTHOR : leon
 * DATE   : Sep 11, 2019
 * DESC   : OS porting
 */

#ifndef _FLV_RECORDER_OS_PORTING_H_
#define _FLV_RECORDER_OS_PORTING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <ak_thread.h>
#include <ak_common.h>
#include "ak_apps_config.h"
#include "ak_common.h"

typedef void* (* flv_rec_task_hdl)(void *param);
typedef ak_pthread_t       flv_rec_task_id;

typedef ak_sem_t           flv_rec_cond;
#define NEW_COND(x)        ak_thread_sem_init(&(x), 0)
#define DEL_COND(x)        ak_thread_sem_destroy(&(x))
#define SIGNAL_COND(x)     ak_thread_sem_post(&(x))
#define WAIT_COND(x)       ak_thread_sem_wait(&(x))

typedef ak_mutex_t         flv_rec_mutex;
#define NEW_CRITICAL(x)    ak_thread_mutex_init(&(x))
#define DEL_CRITICAL(x)    ak_thread_mutex_destroy(&(x))
#define ENTER_CRITICAL(x)  ak_thread_mutex_lock(&(x))
#define LEAVE_CRITICAL(x)  ak_thread_mutex_unlock(&(x))

#define mylog_error ak_print_normal
#define mylog_info  ak_print_normal
#define mylog_warn  ak_print_normal

#define msleep(x)   ak_sleep_ms(x)

#define FLV_REC_MAX_VID_FRAME_SIZE  (150*1024)
#define FLV_REC_MAX_BUF_SIZE        (150*1024)
#define FLV_REC_PREREC_TIME_MS      (3000)

/*
 * PURPOSE : Create task handler
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : Time stamp in milliseconds
 * DESCRIPT: 0 success, 1 failed
 */
int
FLVRecorderTaskCreate(flv_rec_task_id* id, flv_rec_task_hdl func, void* arg, int stacksize, int pri);

#endif	// _FLV_RECORDER_OS_PORTING_H_
