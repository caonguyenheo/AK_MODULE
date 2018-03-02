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

/* FILE   : flv_pp_buff.c
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : Contain all API for FLV ping pong buffer
 */

#include "flv_rec_os.h"
#include "flv_pp_buff.h"

enum eFLVTagType
    {
    eFLVTagAudio = 8,
    eFLVTagVideo = 9
    };

enum eFLVFrameType
    {
    eFLVFrameTypeKeyFrame = 1,
    eFLVFrameTypeInterFrame = 2
    };

typedef enum eBufferPos
    {
    eBufferPosPing,
    eBufferPosPong,
    eBufferPosTemp,
    eBufferPosInvalid,
    }eBufferPos;

typedef struct tFLVBuffer
    {
    int             maxSize;    //Max size of this buffer
    int             curSize;    //Current size of this buffer
    int             tagNo;      //Number of FLV tag in this buffer
    int             duration;   //Duration in milliseconds;
    void            *data;      //Data buffer
    }tFLVBuffer;

typedef struct tFLVPingPong
    {
    eBufferPos      wPos;       //Write buffer position
    eBufferPos      rPos;       //Read buffer position
    int             rFlag;      //Flag indicate data is ready to read
    flv_rec_mutex   lock;       //Protect buffer in case concurrent access
    volatile int    ready;
    tFLVBuffer      pingpong[eBufferPosInvalid];
    }tFLVPingPong;

#define GET_8(p)    ( (p)[0] )
#define GET_24(p)   ( ((p)[0]<<16) | ((p)[1]<<8) | ((p)[2]))
#define GET_32(p)   ( ((p)[0]<<24) | ((p)[1]<<16)| ((p)[2]<<8) |(p)[3])

/**************************************************************************
 *                          STATIC FUNCTION
 **************************************************************************/
/*
 * PURPOSE : Initialize FLV buffer
 * INPUT   : [size]    - Size of this buffer
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
static int
FLVBufferInit(tFLVBuffer* buff, int size)
    {
    if(buff == NULL)
        {
        mylog_error("Invalid input : NULL pointer");
        return -1;
        }

    if(size <= 0)
        {
        mylog_error("Invalid size : %d", size);
        return -2;
        }

    buff->data = malloc(sizeof(char) * size);
    if(buff->data == NULL)
        {
        mylog_error("Allocate memory error. Size %d", sizeof(char) * size);
        return -3;
        }
    buff->maxSize = size;
    buff->curSize = 0;
    buff->tagNo = 0;
    buff->duration = 0;
    return 0;
    }

/*
 * PURPOSE : Free FLV buffer
 * INPUT   : [size]    - Size of this buffer
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
static void
FLVBufferDeInit(tFLVBuffer* buff)
    {
    if(buff)
        {
        if(buff->data)
            free(buff->data);
        }
    }

/*
 * PURPOSE : Search IFrame in buffer
 * INPUT   : [buff] - Buffer
 *           [size] - Size of buffer
 * OUTPUT  : None
 * RETURN  : Pointer to tag contain IFrame
 * DESCRIPT: Returned pointer will point to buff. NULL if not found
 */
static void*
FLVBufferIFramSearch(void* buff, int size)
    {
    uint8_t     tagType = 0;
    uint8_t     *pBuffer = NULL;
    uint8_t     *pLimit = NULL;
    uint32_t    dataSize = 0;
    uint8_t     frameType = 0;
    uint32_t    prevTagSize = 0;
    uint32_t    iPktCount = 0;

    if(buff == NULL || size < 15)
        return NULL;

    pBuffer = (uint8_t*)buff;
    pLimit = (uint8_t*)buff + size;

    while(pBuffer < pLimit)
        {
        tagType = GET_8(pBuffer);
        dataSize = GET_24(pBuffer + 1);

        /* Check overflow buffer */
        if(pBuffer + 11 + dataSize + 4 > pLimit)
            {
            mylog_error("Overflow buffer at pkt %d: %d in total %d", iPktCount, dataSize, pLimit - pBuffer);
            return NULL;
            }

        if(tagType == eFLVTagVideo)
            {
            frameType = (GET_8(pBuffer + 11) & 0xF0) >> 4;
            if(frameType == eFLVFrameTypeKeyFrame)
                {
                /* Check valid FLV packet */
                prevTagSize = GET_32(pBuffer + dataSize + 11);
                if(prevTagSize != dataSize + 11)
                    {
                    mylog_error("Wrong FLV packet");
                    return NULL;
                    }
                mylog_info("Got IFrame at packet %u. Size %u", iPktCount, dataSize);
                return pBuffer;
                }
            }
        pBuffer += 11 + dataSize + 4;
        iPktCount++;
        }
    return NULL;
    }

/*static void
FLVMuxerHexDump(char* string, void *data, int size)
    {
    char    str[2048] = {0};
    int     i;
    int     length = 0;

    if(data == NULL)
        return;
    if(size > sizeof(str) / 3)
        length = sizeof(str)/3;
    else
        length = size;
    for(i = 0; i < length; i++)
        sprintf(str + strlen(str), "%02X ", ((uint8_t*)data)[i]);
    printf("%s : %s\n", string, str);
    }*/

/*
 * PURPOSE : Compute duration in this buffer
 * INPUT   : [buff]       - Buffer
 *           [size]       - Size of buffer
 *           [max_dur]    - Max duration for seeking. Set to -1 for ignore
 * OUTPUT  : [videoTagNo] - Number of video frame written. Can be NULL
 *           [size]       - Size of correct data
 *           [ts]         - First time stamp of this data
 * RETURN  : Duration in milliseconds
 * DESCRIPT: None
 */
static int
FLVBufferDurationGet(void* buff, int *size, int max_dur, int* videoTagNo, int* ts)
    {
    uint8_t     *pBuffer = NULL;
    uint8_t     *pLimit = NULL;
    uint32_t    dataSize = 0;
    uint32_t    firstTs = 0;
    uint32_t    curTs = 0;
    uint32_t    prevTagSize = 0;
    uint32_t    iPktCount = 0;
    uint8_t     iShouldCheckDur = 0;

    if(buff == NULL || size == NULL || *size < 15)
        return 0;

    if(max_dur > 0)
        iShouldCheckDur = 1;

    pBuffer = (uint8_t*)buff;
    pLimit = (uint8_t*)buff + (*size);
    firstTs = GET_24(pBuffer + 4) | (GET_8(pBuffer + 7) << 24);
    curTs = firstTs + 66;
    while(pBuffer < pLimit)
        {
        if(videoTagNo)
            {
            if(GET_8(pBuffer) == eFLVTagVideo)
                (*videoTagNo) += 1;
            }
        dataSize = GET_24(pBuffer + 1);
        curTs = GET_24(pBuffer + 4) | (GET_8(pBuffer + 7) << 24);

        /* Check overflow buffer */
        if(pBuffer + 11 + dataSize + 4 > pLimit)
            {
            mylog_error("Overflow buffer at pkt %d: %d in total %d", iPktCount, dataSize, pLimit - pBuffer);
            *size = (pBuffer - (uint8_t*)buff);
            return curTs - firstTs;
            }

        /* Check valid FLV packet */
        prevTagSize = GET_32(pBuffer + dataSize + 11);
        if(prevTagSize != dataSize + 11)
            {
            mylog_error("Wrong FLV packet");
            *size = (pBuffer - (uint8_t*)buff);
            return curTs - firstTs;
            }
        pBuffer += 11 + dataSize + 4;
        iPktCount++;

        /* Check for amount of duration only */
        if(iShouldCheckDur)
            {
            if(curTs - firstTs >= max_dur)
                break;
            }
        }

    if(ts)
        *ts = firstTs;
    return curTs - firstTs;
    }

/*
 * PURPOSE : Prepare size in bytes at FLV ping pong buffer
 * INPUT   : [fppb] - FLV ping pong buffer
 *           [size] - Size need to prepare
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: This will update wPos and rPos accordingly
 */
static void
FLVBufferPrepareSize(tFLVPingPong* fppb, int size)
    {
    int remainSize = 0;
    int currentSize = 0;
    int currentDur = 0;

    if(fppb == NULL || size <= 0)
        return;

    /* Lock buffer */
    if(ENTER_CRITICAL(fppb->lock) != 0)
        return;

    currentSize = fppb->pingpong[fppb->wPos].curSize;
    remainSize = fppb->pingpong[fppb->wPos].maxSize - fppb->pingpong[fppb->wPos].curSize;
    currentDur = FLVBufferDurationGet(fppb->pingpong[fppb->wPos].data, &currentSize, -1, NULL, NULL);

    /* Switch buffer */
    if((remainSize < size) || (currentDur >= FLV_REC_PREREC_TIME_MS))
        {
        fppb->rFlag = 1;
        fppb->rPos = fppb->wPos;
        fppb->wPos = (fppb->wPos + 1) % (eBufferPosPong + 1);

        /* Reset new buffer */
        fppb->pingpong[fppb->wPos].curSize = 0;
        fppb->pingpong[fppb->wPos].tagNo = 0;
        fppb->pingpong[fppb->wPos].duration = 0;
        remainSize = fppb->pingpong[fppb->rPos].curSize;
        /*mylog_debug("Read buffer size %u. dur %u", remainSize,
                FLVBufferDurationGet(fppb->pingpong[fppb->rPos].data, &remainSize, NULL, NULL));*/
        fppb->ready = 1;
        }

    /* Unlock buffer */
    if(LEAVE_CRITICAL(fppb->lock) != 0)
        return;
    }
/**************************************************************************
 *                          PUBLIC FUNCTION
 **************************************************************************/

/*
 * PURPOSE : Create FLV ping pong buffer
 * INPUT   : [size]  - Size of buffer
 * OUTPUT  : None
 * RETURN  : Pointer to buffer
 * DESCRIPT: None
 */
void*
FLVPPBuffCreate(int size)
    {
    int             i, j = 0;
    int             iRetCode = 0;
    tFLVPingPong    *buffer = NULL;

    if(size <= 0)
        {
        mylog_error("Invalid size : %d", size);
        return NULL;
        }

    buffer = malloc(sizeof(tFLVPingPong));
    if(buffer == NULL)
        {
        mylog_error("Allocate memory error. Size %d", sizeof(tFLVPingPong));
        return NULL;
        }

    buffer->wPos = eBufferPosPing;
    buffer->rPos = eBufferPosPong;
    buffer->rFlag = 0;
    if(NEW_CRITICAL(buffer->lock) != 0)
        {
        mylog_error("Initialize MUTEX failed");
        return NULL;
        }

    buffer->ready = 0;
    for(i = eBufferPosPing; i < eBufferPosInvalid; i++)
        {
        iRetCode = FLVBufferInit(&(buffer->pingpong[i]), size);
        if(iRetCode != 0)
            {
            for(j = eBufferPosPing; j < i; j++)
                FLVBufferDeInit(&(buffer->pingpong[j]));
            return NULL;
            }
        }
    return buffer;
    }

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
FLVPPBuffPushData(void* fppb, void* flvPkt, int flvPktSize)
    {
    tFLVPingPong *pFPPB = NULL;

    if(fppb == NULL || flvPkt == NULL || flvPktSize <= 0)
        return;

    pFPPB = (tFLVPingPong*)fppb;

    /* Preparing buffer and write to it */
    FLVBufferPrepareSize(pFPPB, flvPktSize);
    memcpy(pFPPB->pingpong[pFPPB->wPos].data + pFPPB->pingpong[pFPPB->wPos].curSize, flvPkt, flvPktSize);
    pFPPB->pingpong[pFPPB->wPos].curSize += flvPktSize;
    pFPPB->pingpong[pFPPB->wPos].tagNo += 1;
    }

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
FLVPPBuffFlushToFilePointer(void* fppb, FILE* pFile, int flag, int max_dur, int* ts,  int *duration, int* videoTagNo)
    {
    void            *pWritePtr = NULL;
    tFLVPingPong    *pFPPB = NULL;
    int             writeSize = 0;
    int             writtenDuration = 0;
    int             writtenVideoFrameNo = 0;
    int             returnSize = 0;

    if(fppb == NULL || pFile == NULL)
        return 0;

    pFPPB = (tFLVPingPong*)fppb;

    while(!pFPPB->ready)
        msleep(500);

    /* Lock buffer to copy to 3rd buffer */
    if(ENTER_CRITICAL(pFPPB->lock) != 0)
        return 0;

    if(pFPPB->rFlag)
        {
        pFPPB->pingpong[eBufferPosTemp].curSize = pFPPB->pingpong[pFPPB->rPos].curSize;
        pFPPB->pingpong[eBufferPosTemp].tagNo = pFPPB->pingpong[pFPPB->rPos].tagNo;
        pFPPB->pingpong[eBufferPosTemp].duration = pFPPB->pingpong[pFPPB->rPos].duration;
        memcpy(pFPPB->pingpong[eBufferPosTemp].data, pFPPB->pingpong[pFPPB->rPos].data, pFPPB->pingpong[pFPPB->rPos].curSize);
        pFPPB->rFlag = 0;
        }

    /* Unlock buffer */
    if(LEAVE_CRITICAL(pFPPB->lock) != 0)
        return 0;

    /* Search key frame if need */
    pWritePtr = pFPPB->pingpong[eBufferPosTemp].data;
    writeSize = pFPPB->pingpong[eBufferPosTemp].curSize;
    if(flag)
        {
        pWritePtr = FLVBufferIFramSearch(pFPPB->pingpong[eBufferPosTemp].data,
                                         pFPPB->pingpong[eBufferPosTemp].curSize);
        if(pWritePtr == NULL)
            return 0;
        writeSize = pFPPB->pingpong[eBufferPosTemp].curSize - (pWritePtr - pFPPB->pingpong[eBufferPosTemp].data);
        }

    if(writeSize <= 0)
        return 0;

    if(duration || videoTagNo)
        {
        writtenDuration = FLVBufferDurationGet(pWritePtr, &writeSize, max_dur, &writtenVideoFrameNo, ts);
        if(duration)
            *duration = writtenDuration;
        if(videoTagNo)
            *videoTagNo = writtenVideoFrameNo;
        }

    /* flush data to file */
    if(writeSize < 0)
        writeSize = 0;
//    returnSize = fwrite(pWritePtr, sizeof(char), writeSize, pFile);
    returnSize = f_mem_write(pWritePtr, sizeof(char), writeSize, pFile);
    if(returnSize < 0)
        mylog_error("Can not write enough data. Error code = %d", returnSize);

    /* Clear data in temp buffer */
    pFPPB->pingpong[eBufferPosTemp].curSize = 0;
    pFPPB->pingpong[eBufferPosTemp].tagNo = 0;
    pFPPB->pingpong[eBufferPosTemp].duration = 0;
    return returnSize;
    }

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
FLVPPBuffFlushCurrent(void* fppb, FILE* pFile, int flag, int *duration, int* videoTagNo)
    {
    tFLVPingPong    *pFPPB = NULL;

    if(fppb == NULL || pFile == NULL)
        return 0;

    pFPPB = (tFLVPingPong*)fppb;

    /* Lock buffer */
    if(ENTER_CRITICAL(pFPPB->lock) != 0)
        return 0;

    /* Copy current to read buffer */
    pFPPB->pingpong[pFPPB->rPos].curSize = pFPPB->pingpong[pFPPB->wPos].curSize;
    pFPPB->pingpong[pFPPB->rPos].tagNo = pFPPB->pingpong[pFPPB->wPos].tagNo;
    pFPPB->pingpong[pFPPB->rPos].duration = pFPPB->pingpong[pFPPB->wPos].duration;
    memcpy(pFPPB->pingpong[pFPPB->rPos].data, pFPPB->pingpong[pFPPB->wPos].data, pFPPB->pingpong[pFPPB->wPos].curSize);
    pFPPB->rFlag = 1;

    /* Reset write buffer */
    pFPPB->pingpong[pFPPB->wPos].curSize = 0;
    pFPPB->pingpong[pFPPB->wPos].tagNo = 0;
    pFPPB->pingpong[pFPPB->wPos].duration = 0;

    /* Unlock buffer */
    if(LEAVE_CRITICAL(pFPPB->lock) != 0)
        return 0;

    return FLVPPBuffFlushToFilePointer(fppb, pFile, flag, -1, NULL, duration, videoTagNo);
    }

/*
 * PURPOSE : Reset FLV buffer
 * INPUT   : [fppb]   - FLV ping pong buffer pointer
 * OUTPUT  : None
 * RETURN  : 1 if can flush out
 * DESCRIPT: None
 */
int
FLVPPBuffResetAll(void* fppb)
    {
    int             i = 0;
    tFLVPingPong    *pFPPB = NULL;

    if(fppb == NULL)
        return 0;

    pFPPB = (tFLVPingPong*)fppb;

    /* Lock buffer */
    if(ENTER_CRITICAL(pFPPB->lock) != 0)
        return 0;

    pFPPB->rFlag = 1;
    for(i = eBufferPosPing; i < eBufferPosInvalid; i++)
        {
        pFPPB->pingpong[i].curSize = 0;
        pFPPB->pingpong[i].tagNo = 0;
        pFPPB->pingpong[i].duration = 0;
        }
    /* Unlock buffer */
    if(LEAVE_CRITICAL(pFPPB->lock) != 0)
        return 0;

    return 1;
    }
