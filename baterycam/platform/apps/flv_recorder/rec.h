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

/* FILE   : rec.c
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : Main task for taking care record
 */

#ifndef STREAM_MGR_RECORDING_H_
#define STREAM_MGR_RECORDING_H_

/*
 * PURPOSE : Start record FLV Task
 * INPUT   : [stacksize] - Task stack size
 *           [pri]       - Task priority
 * OUTPUT  : None
 * RETURN  : Task ID of this task
 * DESCRIPT: None
 */
int
StreamMgrRecordingRun(int stacksize, int pri);

/*
 * PURPOSE : Record FLV to file
 * INPUT   : [filename]    - String
 *           [duration] - Maximum param can have
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
StreamMgrRecordingStart(char* filename, int duration);

/*
 * PURPOSE : Stop all recording is in progress
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
StreamMgrRecordingStop();

/*
 * PURPOSE : Exit record FLV Task
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: None
 */
void
StreamMgrRecordingExit();

/*
 * PURPOSE : Get status of recording process
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : status
 * DESCRIPT: None
 */
int
StreamMgrRecordingStatus();

#endif	// STREAM_MGR_RECORDING_H_
