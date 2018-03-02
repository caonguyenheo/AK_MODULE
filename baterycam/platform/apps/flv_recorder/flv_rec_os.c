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

/* FILE   : flv_rec_os.c
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : OS Porting
 */

#include "flv_rec_os.h"

/*
 * PURPOSE : Get time stamp
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : Time stamp in milliseconds
 * DESCRIPT: None
 */
long
FLVMuxerGetTimeStamp()
    {
    return get_tick_count();
    }

/*
 * PURPOSE : Create task handler
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : Time stamp in milliseconds
 * DESCRIPT: 0 success, 1 failed
 */
int
FLVRecorderTaskCreate(flv_rec_task_id* id, flv_rec_task_hdl func, void* arg, int stacksize, int pri)
    {
    return ak_thread_create(id, func, arg, stacksize, pri);
    }

