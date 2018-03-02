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

/* FILE   : mux.h
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : FLV Muxer
 */

#ifndef FLV_MUX_H_
#define FLV_MUX_H_

#include <stdint.h>

/*
 * PURPOSE : Remove FLV muxer
 * INPUT   : [muxer] - Muxer pointer
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: None
 */
void
FLVMuxerRemove(void* muxer);

/*
 * PURPOSE : Create new FLV muxer (H264 + AAC)
 * INPUT   : [size] - Buffer size
 * OUTPUT  : None
 * RETURN  : Muxer pointer
 * DESCRIPT: Buffer size must big enough to store one IFrame
 */
void*
FLVMuxerCreate(int size);

/*
 * PURPOSE : Create new FLV muxer at specific file
 * INPUT   : [fileName] - File name
 *           [size]     - Buffer size
 * OUTPUT  : None
 * RETURN  : Muxer pointer
 * DESCRIPT: Buffer size must big enough to store one IFrame
 */
void*
FLVMuxerFileCreate(char* fileName, int size);

/*
 * PURPOSE : Get FLV header
 * INPUT   : [muxer]     - Muxer pointer
 *           [duration]  - Clip duration
 * OUTPUT  : [hdrSize]   - FLV header size
 * RETURN  : FLV header pointer
 * DESCRIPT: None
 */
void*
FLVMuxerHeaderGet(void* muxer, int duration, int* hdrSize);

/*
 * PURPOSE : Create FLV packet from video frame
 * INPUT   : [muxer]     - Muxer pointer
 *           [frame]     - H624 Frame pointer
 *           [frameSize] - H264 frame size
 *           [ts]        - H264 frame time stamp. 0 for auto generate
 * OUTPUT  : [pktSize]   - FLV packet size
 *           [pktType]   - FLV packet type
 *           [ts_exceed] - Time stamp is exceed 4 bytes. Only applied for auto generate
 * RETURN  : FLV packet pointer
 * DESCRIPT: None
 */
void*
FLVMuxerFillVideo(void* muxer, void* frame, int frameSize, uint64_t ts, int* pktSize, int* pktType, int* ts_exceed);

/*
 * PURPOSE : Create FLV packet from audio frame
 * INPUT   : [muxer]     - Muxer pointer
 *           [frame]     - Audio Frame pointer
 *           [frameSize] - Audio frame size
 *           [ts]        - Audio frame time stamp. 0 for auto generate
 * OUTPUT  : [pktSize]   - FLV packet size
 *           [ts_exceed] - Time stamp is exceed 4 bytes. Only applied for auto generate
 * RETURN  : FLV packet pointer
 * DESCRIPT: None
 */
void*
FLVMuxerFillAudio(void* muxer, void* frame, int frameSize, uint64_t ts, int* pktSize, int* ts_exceed);

/*
 * PURPOSE : Write data to muxer file
 * INPUT   : [muxer] - Muxer pointer
 *           [data]  - Data pointer
 *           [size]  - data size
 * OUTPUT  : None
 * RETURN  : Size actual write
 * DESCRIPT: None
 */
int
FLVMuxerWriteToFile(void* muxer, void* data, int size);


/*
 * PURPOSE : Get full flv header
 * INPUT   : [data]     - Data pointer
 *           [max_size] - Max size of data
 * OUTPUT  : [data]     - FLV header to be copied
 * RETURN  : FLV header size
 * DESCRIPT: None
 */
int
DoorBellFLVHeaderGen(void* data, int max_size);

/*
 * PURPOSE : Write first header to file
 * INPUT   : [pFile] - File pointer
 * OUTPUT  : None
 * RETURN  : FLV header size
 * DESCRIPT: None
 */
int
DoorBellFLVHeaderWrite(FILE* pFile);

#endif /* FLV_MUX_H_ */
