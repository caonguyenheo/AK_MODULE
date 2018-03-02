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

/* FILE   : flv_engine.h
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : Mux audio and video then put it to ping pong buffer
 */

#ifndef _FLV_ENGINE_H_
#define _FLV_ENGINE_H_

/*
 * PURPOSE : Initialize FLV engine
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
FLVEngineInit();

/*
 * PURPOSE : Push audio alaw data to flv
 * INPUT   : [data] - Data pointer
 *           [size] - Data size
 *           [ts]   - Data time stamp
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
FLVEnginePushAudioData(void* data, int size, int ts);

/*
 * PURPOSE : Push video data to flv
 * INPUT   : [data] - Data pointer
 *           [size] - Data size
 *           [ts]   - Data time stamp
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
FLVEnginePushVideoData(void* data, int size, int ts);

/*
 * PURPOSE : Write FLV data to file
 * INPUT   : [pFile]    - File pointer
 *           [flag]     - Stat with I-Frame flag
 *           [max_dur]  - Max duration to write. -1 to ignore
 * OUTPUT  : [dur]      - Duration
 * RETURN  : Size written
 * DESCRIPT: None
 */
int
FLVEngineWriteToFile(FILE* pFile, int flag, int max_dur, int* dur);

/*
 * PURPOSE : Write header to file
 * INPUT   : [pFile]    - File pointer
 *           [duration] - File duration
 * OUTPUT  : None
 * RETURN  : Header size
 * DESCRIPT: None
 */
int
FLVEngineWriteHeader(FILE* pFile, int duration);

/*
 * PURPOSE : Get flv header
 * INPUT   : [buff]     - Buffer pointer
 *           [bufSize]  - Buffer Size
 * OUTPUT  : [buff]     - Data
 *           [size]     - Data size
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
FLVEngineGetHeader(void* buff, int bufSize, int* size);

#endif /* _FLV_ENGINE_H_ */
