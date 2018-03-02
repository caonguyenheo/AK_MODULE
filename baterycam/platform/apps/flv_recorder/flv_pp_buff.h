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

/* FILE   : flv_pp_buff.h
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : Contain all API for FLV ping pong buffer
 */

#ifndef _FLV_PP_BUFF_H_
#define _FLV_PP_BUFF_H_

/*
 * PURPOSE : Create FLV ping pong buffer
 * INPUT   : [size]  - Size of buffer
 * OUTPUT  : None
 * RETURN  : Pointer to buffer
 * DESCRIPT: None
 */
void*
FLVPPBuffCreate(int size);

/*
 * PURPOSE : Push FLV packet to buffer
 * INPUT   : [fppb]       - FLV ping pong buffer pointer
 *           [flvPkt]     - FLV packet
 *           [flvPktSize] - FLV packet size
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: None
 */
void
FLVPPBuffPushData(void* fppb, void* flvPkt, int flvPktSize);

/*
 * PURPOSE : Flush current read FLV buffer to file pointer
 * INPUT   : [fppb]       - FLV ping pong buffer pointer
 *           [pFile]      - File handler
 *           [flag]       - Search IFrame (1) or not (0)
 *           [max_dur]    - Max duration for written. Negative for ignore
 * OUTPUT  : [ts]         - First time stamp of this data
 *           [duration]   - Duration in milliseconds. Can be NULL
 *           [videoTagNo] - Number of video frame written. Can be NULL
 * RETURN  : Number of bytes written
 * DESCRIPT: User must open/close file pointer before/after this function
 */
int
FLVPPBuffFlushToFilePointer(void* fppb, FILE* pFile, int flag, int max_dur, int* ts,  int *duration, int* videoTagNo);

/*
 * PURPOSE : Flush current write FLV buffer to file
 * INPUT   : [fppb]       - FLV ping pong buffer pointer
 *           [pFile]      - File handler
 *           [flag]       - Search IFrame (1) or not (0)
 * OUTPUT  : [videoTagNo] - Number of video frame written. Can be NULL
 *           [duration]   - Duration in milliseconds. Can be NULL
 * RETURN  : Number of bytes written
 * DESCRIPT: User must open/close file pointer before/after this function
 */
int
FLVPPBuffFlushCurrent(void* fppb, FILE* pFile, int flag, int *duration, int* videoTagNo);

/*
 * PURPOSE : Reset FLV buffer
 * INPUT   : [fppb]   - FLV ping pong buffer pointer
 * OUTPUT  : None
 * RETURN  : 1 if can flush out
 * DESCRIPT: None
 */
int
FLVPPBuffResetAll(void* fppb);

#endif /* _FLV_PP_BUFF_H_ */
