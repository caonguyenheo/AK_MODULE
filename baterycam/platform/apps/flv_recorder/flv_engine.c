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

/* FILE   : flv_engine.c
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : Mux audio and video then put it to ping pong buffer
 */

#include "flv_rec_os.h"
#include "mux.h"
#include "flv_pp_buff.h"
#include "audio_conv.h"
#include "file_on_mem.h"

typedef struct tFLVEngine
    {
    int initFLag;
    void* muxer;
    void* ppbuf;
    flv_rec_mutex lock;
    }tFLVEngine;

static tFLVEngine gFLVEngine = {0, 0, 0, 0};

/*
 * PURPOSE : Initialize FLV engine
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
FLVEngineInit()
    {
    if(gFLVEngine.initFLag)
        return 0;

    mylog_info("FLV Engine Init.");
    if(NEW_CRITICAL(gFLVEngine.lock) != 0)
        {
        mylog_info("FAILED\n");
        return -2;
        }

    mylog_info(".");
    gFLVEngine.muxer = FLVMuxerCreate(FLV_REC_MAX_VID_FRAME_SIZE);
    if(gFLVEngine.muxer == NULL)
        {
        mylog_info("FAILED\n");
        return -3;
        }

    mylog_info(".");
    gFLVEngine.ppbuf = FLVPPBuffCreate(FLV_REC_MAX_BUF_SIZE);
    if(gFLVEngine.ppbuf == NULL)
        {
        FLVMuxerRemove(gFLVEngine.muxer);
        mylog_info("FAILED\n");
        return -4;
        }

    mylog_info("OK\n");
    gFLVEngine.initFLag = 1;
    return 0;
    }

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
FLVEnginePushAudioData(void* data, int size, int ts)
    {
    int   flvPktSize = 0;
    void *flvPktData = NULL;
    int   ts_reset = 0;

    if(gFLVEngine.initFLag)
        {
        ENTER_CRITICAL(gFLVEngine.lock);
            {
            flvPktData = FLVMuxerFillAudio(gFLVEngine.muxer, data, size, ts, &flvPktSize, &ts_reset);
            if(ts_reset)
                FLVPPBuffResetAll(gFLVEngine.ppbuf);
            if(flvPktData && flvPktSize > 0)
                FLVPPBuffPushData(gFLVEngine.ppbuf, flvPktData, flvPktSize);
            }
        LEAVE_CRITICAL(gFLVEngine.lock);
        }
     return 0;
    }

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
FLVEnginePushVideoData(void* data, int size, int ts)
    {
    int   flvPktSize = 0;
    void *flvPktData = NULL;
    int   flvPktType = 0;
    int   ts_reset = 0;

    if(gFLVEngine.initFLag)
        {
        ENTER_CRITICAL(gFLVEngine.lock);
            {
            flvPktData = FLVMuxerFillVideo(gFLVEngine.muxer, data, size, ts, &flvPktSize, &flvPktType, &ts_reset);
            if(ts_reset)
                FLVPPBuffResetAll(gFLVEngine.ppbuf);
            if(flvPktData && flvPktSize > 0)
                FLVPPBuffPushData(gFLVEngine.ppbuf, flvPktData, flvPktSize);
            }
        LEAVE_CRITICAL(gFLVEngine.lock);
        }
    return 0;
    }

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
FLVEngineWriteToFile(FILE* pFile, int flag, int max_dur, int* dur)
    {
    if(gFLVEngine.initFLag)
       return FLVPPBuffFlushToFilePointer(gFLVEngine.ppbuf, pFile, flag, max_dur, NULL, dur, NULL);
    return 0;
    }

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
FLVEngineGetHeader(void* buff, int bufSize, int* size)
    {
    int hdrSize = 0;
    void* hdrData = NULL;

    if(gFLVEngine.initFLag)
        {
        hdrData = FLVMuxerHeaderGet(gFLVEngine.muxer, 0, &hdrSize);
        if(hdrData == NULL || hdrSize <= 0 || hdrSize >= bufSize)
            return -1;
        memcpy(buff, hdrData, hdrSize);
        if(size)
            *size = hdrSize;
        }
    return 0;
    }

/*
 * PURPOSE : Write header to file
 * INPUT   : [pFile]    - File pointer
 *           [duration] - File duration
 * OUTPUT  : None
 * RETURN  : Header size
 * DESCRIPT: None
 */
int
FLVEngineWriteHeader(FILE* pFile, int duration)
    {
    int hdrSize = 0;
    void* hdrData = NULL;

    if(gFLVEngine.initFLag)
        {
        if(DoorBellFLVHeaderWrite(pFile) != 0)
            mylog_info("Write header failed");
        hdrData = FLVMuxerHeaderGet(gFLVEngine.muxer, duration, &hdrSize);
        if(hdrData == NULL || hdrSize <= 0)
            return -1;
        //return fwrite(hdrData, sizeof(char), hdrSize, pFile);
        return f_mem_write(hdrData, sizeof(char), hdrSize, pFile);
        }
    return -2;
    }
