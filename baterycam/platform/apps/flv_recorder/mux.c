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

/* FILE   : mux.c
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : FLV Muxer
 */

#include "flv_rec_os.h"
#include "mux.h"
#include "audio_conv.h"
#include "file_on_mem.h"

typedef enum eH624FrameType
    {
    eH264_SPS_UNKNOWN = 0x00000000,
    eH264_SPS_FRAME = 0x00000001,
    eH264_PPS_FRAME = 0x00000002,
    eH264_I_FRAME = 0x00000004,
    eH264_P_FRAME = 0x00000008,
    }eH624FrameType;

typedef struct tFLVPktHdr
    {
    uint8_t  type;
    uint8_t  dataSize[3];
    uint8_t  timestamp[3];
    uint8_t  timestampExt;
    uint8_t  streamID[3];
    }tFLVPktHdr;

typedef struct tH624FrameInfo
    {
    eH624FrameType  eFrameType;
    uint32_t        u32IPOffset;
    uint32_t        u32IPLen;
    uint32_t        u32SPSOffset;
    uint32_t        u32SPSLen;
    uint32_t        u32PPSOffset;
    uint32_t        u32PPSLen;
    }tH624FrameInfo;

typedef struct tBuffer
    {
    void    *data;
    unsigned int curSize;
    unsigned int maxSize;
    }tBuffer;

typedef struct tFLVMuxer
    {
    tBuffer     packet;
    tBuffer     header;
    long        curTimeStamp;
    tBuffer     sps;
    tBuffer     pps;
    FILE        *pFile;
    }tFLVMuxer;

/*
 * PURPOSE : Dump data in hex value
 * INPUT   : [data]       - Data pointer
 *           [size]       - Data size
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: None
 */
static void
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
    // mylog_info("%s : %s\n", string, str);
    }

/*
 * PURPOSE : Check FLV packet is correct format
 * INPUT   : [data]       - Packet pointer
 *           [size]       - Packet size
 * OUTPUT  : [pFrameInfo] - Frame info
 * RETURN  : 1 if good
 * DESCRIPT: None
 */
static int
FLVPacketCheck(uint8_t* data, size_t pktSize)
    {
    unsigned int dataSize = 0;
    unsigned int trailingSize = 0;
    dataSize = ((unsigned int) data[1] << 16) |
               ((unsigned int) data[2] << 8) |
               (unsigned int) data[3];
    trailingSize = (((unsigned int) data[pktSize - 4] & 0xFF) << 24) |
                   (((unsigned int) data[pktSize - 3] & 0xFF) << 16) |
                   (((unsigned int) data[pktSize - 2] & 0xFF) << 8) |
                   ((unsigned int) data[pktSize - 1] & 0xFF);
    if((pktSize != (dataSize + 15)) || ((trailingSize - dataSize) != 11))
        {
        printf("Wrong FLV packet type %u (Packet Size = %u. Data Size = %u. "
                "Trailing Size = %u [%0x2, %0x2, %0x2, %0x2])\n",
                data[0], pktSize, dataSize, trailingSize,
                data[pktSize - 4], data[pktSize - 3],
                data[pktSize - 2], data[pktSize - 1]);
        return 0;
        }
    else
        return 1;
    }

/*
 * PURPOSE : Create FLV packet AVC sequence header
 * INPUT   : [muxer]     - Muxer pointer
 * OUTPUT  : [pktSize]   - FLV packet size
 * RETURN  : FLV packet pointer
 * DESCRIPT: None
 */
static void*
FLVMuxerFillVideoHeader(void* muxer, void* data, int* pktSize)
    {
    uint8_t         *header_ptr = NULL;
    tFLVMuxer       *mux = NULL;
    uint32_t        avcDataSize = 0;
    tFLVPktHdr      *pPktHdr = NULL;

    /* Check input */
    if(muxer == NULL)
        return NULL;

    mux = (tFLVMuxer*)muxer;
    avcDataSize = mux->sps.curSize + mux->pps.curSize + 5;

    /* Prepare header */
    pPktHdr = (tFLVPktHdr*)data;
    memset(pPktHdr, 0, sizeof(tFLVPktHdr));
    header_ptr = (uint8_t *) data + sizeof(tFLVPktHdr);
    pPktHdr->type = 0x09;
    pPktHdr->dataSize[0] = (avcDataSize >> 16) & 0xFF;
    pPktHdr->dataSize[1] = (avcDataSize >> 8) & 0xFF;
    pPktHdr->dataSize[2] = (avcDataSize) & 0xFF;

    /* This frame is AVC (7) key frame (1) */
    *(header_ptr++) = 0x17;

    /* AVC Packet type : AVC sequence header */
    *(header_ptr++) = 0;
    *(header_ptr++) = 0;
    *(header_ptr++) = 0;
    *(header_ptr++) = 0;

    /* AVCDecoderConfigurationRecord */
    memcpy(header_ptr, mux->sps.data, mux->sps.curSize);
    header_ptr += mux->sps.curSize;
    memcpy(header_ptr, mux->pps.data, mux->pps.curSize);
    header_ptr += mux->pps.curSize;
    avcDataSize += 11;

    *(header_ptr++) = ((avcDataSize) >> 24) & 0xFF;
    *(header_ptr++) = ((avcDataSize) >> 16) & 0xFF;
    *(header_ptr++) = ((avcDataSize) >> 8) & 0xFF;
    *(header_ptr++) = (avcDataSize) & 0xFF;
    avcDataSize += 4;
    if(pktSize)
        *pktSize = avcDataSize;
    if(FLVPacketCheck(data, avcDataSize))
        return mux->packet.data;
    else
        return NULL;
    }

/*
 * PURPOSE : Parse H264 frame to get frame type and size
 * INPUT   : [pu8BitStream]    - Frame pointer
 *           [u32BitStreamLen] - Frame size
 * OUTPUT  : [psFrameInfo]     - Frame info
 * RETURN  : None
 * DESCRIPT: None
 */
static void
ParseFrame(uint8_t *pu8BitStream, uint32_t u32BitStreamLen, tH624FrameInfo *psFrameInfo)
    {
    uint32_t u32NextSyncWordAdr = 0;
    memset(psFrameInfo, 0x00, sizeof(tH624FrameInfo));

    if((pu8BitStream[4] & 0x1F) == 0x05)
        {
        // It is I-frame
        psFrameInfo->u32IPLen = u32BitStreamLen;
        psFrameInfo->eFrameType = eH264_I_FRAME;
        return;
        }
    else if((pu8BitStream[4] & 0x1F) == 0x01)
        {
        // It is P-frame
        psFrameInfo->u32IPLen = u32BitStreamLen;
        psFrameInfo->eFrameType = eH264_P_FRAME;
        return;
        }
    else
        {
        // Search I/P frame
        uint32_t u32SyncWord = 0x01000000;
        uint32_t u32Len = (u32BitStreamLen - 4);    //4:sync word len
        uint8_t u8FrameType;
        eH624FrameType eCurFrameType = -1;

        while(u32Len)
            {
            if(memcmp(pu8BitStream + u32NextSyncWordAdr, &u32SyncWord, sizeof(uint32_t)) == 0)
                {
                u8FrameType = pu8BitStream[u32NextSyncWordAdr + 4] & 0x1F;

                if(eCurFrameType == eH264_SPS_FRAME)
                    psFrameInfo->u32SPSLen = u32NextSyncWordAdr - psFrameInfo->u32SPSOffset;
                else if(eCurFrameType == eH264_PPS_FRAME)
                    psFrameInfo->u32PPSLen = u32NextSyncWordAdr - psFrameInfo->u32PPSOffset;

                if(u8FrameType == 0x07)
                    {
                    eCurFrameType = eH264_SPS_FRAME;
                    psFrameInfo->u32SPSOffset = u32NextSyncWordAdr;
                    psFrameInfo->eFrameType |= eH264_SPS_FRAME;
                    }
                else if(u8FrameType == 0x08)
                    {
                    eCurFrameType = eH264_PPS_FRAME;
                    psFrameInfo->u32PPSOffset = u32NextSyncWordAdr;
                    psFrameInfo->eFrameType |= eH264_PPS_FRAME;
                    }
                else if(u8FrameType == 0x05)
                    {
                    psFrameInfo->u32IPOffset = u32NextSyncWordAdr;
                    psFrameInfo->u32IPLen = u32BitStreamLen - u32NextSyncWordAdr;
                    psFrameInfo->eFrameType |= eH264_I_FRAME;
                    break;
                    }
                else if(u8FrameType == 0x01)
                    {
                    psFrameInfo->u32IPOffset = u32NextSyncWordAdr;
                    psFrameInfo->u32IPLen = u32BitStreamLen - u32NextSyncWordAdr;
                    psFrameInfo->eFrameType |= eH264_P_FRAME;
                    break;
                    }
                }
            u32NextSyncWordAdr++;
            u32Len--;
            }
        if(u32Len == 0)
            u32NextSyncWordAdr = 0;
        }
    return;
    }

/*
 * PURPOSE : Remove FLV muxer
 * INPUT   : [muxer] - Muxer pointer
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: None
 */
void
FLVMuxerRemove(void* muxer)
    {
    tFLVMuxer* mux = (tFLVMuxer*) muxer;
    if(mux)
        {
        mux->curTimeStamp = 0;
        mux->header.data = NULL;
        mux->header.curSize = 0;
        mux->header.maxSize = 0;
        if(mux->packet.data)
            {
            free(mux->packet.data);
            mux->packet.data = NULL;
            }
        mux->packet.curSize = 0;
        mux->packet.maxSize = 0;
        if(mux->pps.data)
            {
            free(mux->pps.data);
            mux->pps.data = NULL;
            }
        if(mux->sps.data)
            {
            free(mux->sps.data);
            mux->sps.data = NULL;
            }
        if(mux->pFile)
            {
            //fflush(mux->pFile);
            f_mem_flush(mux->pFile);
            //fclose(mux->pFile);
            f_mem_close(mux->pFile);
            mux->pFile = NULL;
            }
        free(mux);
        }
    muxer = NULL;
    }

/*
 * PURPOSE : Create new FLV muxer (H264 + AAC)
 * INPUT   : [size] - Buffer size
 * OUTPUT  : None
 * RETURN  : Muxer pointer
 * DESCRIPT: Buffer size must big enough to store one IFrame
 */
void*
FLVMuxerCreate(int size)
    {
    tFLVMuxer* mux = (tFLVMuxer*)malloc(sizeof(tFLVMuxer));
    if(mux)
        {
        mux->packet.curSize = 0;
        mux->packet.maxSize = size;
        mux->packet.data = malloc(sizeof(char) * mux->packet.maxSize);
        if(mux->packet.data == NULL)
            FLVMuxerRemove(mux);

        mux->sps.maxSize = 1024;
        mux->sps.curSize = 0;
        mux->sps.data = malloc(sizeof(char) * mux->sps.maxSize);
        if(mux->sps.data == NULL)
            FLVMuxerRemove(mux);
        else
            memset(mux->sps.data, 0, mux->sps.maxSize);

        mux->pps.maxSize = 1024;
        mux->pps.curSize = 0;
        mux->pps.data = malloc(sizeof(char) * mux->pps.maxSize);
        if(mux->pps.data == NULL)
            FLVMuxerRemove(mux);
        else
            memset(mux->pps.data, 0, mux->pps.maxSize);

        mux->header.maxSize = (sizeof(char) * (mux->pps.maxSize + mux->sps.maxSize));
        mux->header.data = malloc(mux->header.maxSize);
        if(mux->header.data == NULL)
            FLVMuxerRemove(mux);
        else
            memset(mux->header.data, 0, mux->header.maxSize);

        mux->header.curSize = 0;
        mux->curTimeStamp = 0;
        }
    return mux;
    }

/*
 * PURPOSE : Create new FLV muxer at specific file
 * INPUT   : [fileName] - File name
 *           [size]     - Buffer size
 * OUTPUT  : None
 * RETURN  : Muxer pointer
 * DESCRIPT: Buffer size must big enough to store one IFrame
 */
void*
FLVMuxerFileCreate(char* fileName, int size)
    {
    void        *mux = NULL;
    tFLVMuxer   *muxer = NULL;

    if(fileName == NULL)
        return NULL;

    mux = FLVMuxerCreate(size);
    if(mux == NULL)
        return NULL;
    muxer = (tFLVMuxer*)mux;
    //muxer->pFile = fopen(fileName, "wb");
    muxer->pFile = f_mem_open(fileName, "wb");
    if(muxer->pFile == NULL)
        {
        FLVMuxerRemove(mux);
        return NULL;
        }
    return mux;
    }

/*
 * PURPOSE : Get FLV header
 * INPUT   : [muxer]     - Muxer pointer
 *           [duration]  - Clip duration
 * OUTPUT  : [hdrSize]   - FLV header size
 * RETURN  : FLV header pointer
 * DESCRIPT: None
 */
void*
FLVMuxerHeaderGet(void* muxer, int duration, int* hdrSize)
    {
    tFLVMuxer   *mux = (tFLVMuxer*) muxer;
    int         avcHeaderSize = 0;

    if(mux)
        {
        /* Open pre-generate header to get */
        //while(mux->header.curSize == 0)
            {
            /* Fill AVC frame to metadata */
            if(FLVMuxerFillVideoHeader(mux, mux->header.data + mux->header.curSize, &avcHeaderSize) != NULL)
                mux->header.curSize += avcHeaderSize;
            }
        if(hdrSize)
            *hdrSize = mux->header.curSize;
        return mux->header.data;
        }
    return NULL;
    }

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
FLVMuxerFillVideo(void* muxer, void* frame, int frameSize, uint64_t ts, int* pktSize, int* pktType, int* ts_exceed)
    {
    tH624FrameInfo  FrameInfo = {0};
    uint8_t         *header_ptr = NULL;
    uint32_t        u32FrameOffset;
    uint32_t        u32FrameLen;
    uint32_t        u32PktTs = 0;
    tFLVMuxer       *mux = NULL;
    tFLVPktHdr      *pPktHdr = NULL;

    /* Check input */
    if(muxer == NULL || frame == NULL || frameSize < 4)
        return NULL;

    mux = (tFLVMuxer*)muxer;

    /* Parses frame to get info */
    memset(&FrameInfo, 0, sizeof(tH624FrameInfo));
    ParseFrame((uint8_t*)frame, frameSize, &FrameInfo);

    /* Store SPS and PPS if we catch it */
    if(FrameInfo.u32SPSLen && (FrameInfo.eFrameType & eH264_SPS_FRAME))
        {
        if(mux->sps.maxSize > FrameInfo.u32SPSLen)
            {
            memcpy(mux->sps.data, frame + FrameInfo.u32SPSOffset, FrameInfo.u32SPSLen);
            mux->sps.curSize = FrameInfo.u32SPSLen;
            }
        FLVMuxerHexDump("SPS", mux->sps.data, FrameInfo.u32SPSLen);
        }

    if(FrameInfo.u32PPSLen && (FrameInfo.eFrameType & eH264_PPS_FRAME))
        {
        if(mux->pps.maxSize > FrameInfo.u32PPSLen)
            {
            memcpy(mux->pps.data, frame + FrameInfo.u32PPSOffset, FrameInfo.u32PPSLen);
            mux->pps.curSize = FrameInfo.u32PPSLen;
            }
        FLVMuxerHexDump("PPS", mux->pps.data, mux->pps.curSize);
        }

    /*if(FrameInfo.eFrameType & eH264_I_FRAME)
        {
        u32FrameOffset = FrameInfo.u32IPOffset;
        u32FrameLen = FrameInfo.u32IPLen;
        }
    else if(FrameInfo.eFrameType & eH264_P_FRAME)*/
        {
        u32FrameOffset = 0;
        u32FrameLen = frameSize;
        }

    /* Prepare packet buffer */
    mux->packet.curSize = u32FrameLen + 5;
    if(mux->packet.maxSize < mux->packet.curSize)
        return NULL;

    /* Get time stamp */
    if(mux->curTimeStamp == 0)
        {
        if(ts)
            mux->curTimeStamp = ts;
        else
            mux->curTimeStamp = FLVMuxerGetTimeStamp();
        }

    if(ts)
        u32PktTs = ts - mux->curTimeStamp;
    else
        {
        u32PktTs = FLVMuxerGetTimeStamp() - mux->curTimeStamp;
        /* Reset time stamp if it over 4 bytes */
        if(u32PktTs > 0xEFFFFFFF)
            {
            mylog_warn("Time stamp exceed maximum %u. Reset to 0", u32PktTs);
            mux->curTimeStamp = FLVMuxerGetTimeStamp();
            u32PktTs = 0;
            if(ts_exceed)
                *ts_exceed = 1;
            }
        else
            {
            if(ts_exceed)
                *ts_exceed = 0;
            }
        }

    /* Prepare header */
    pPktHdr = (tFLVPktHdr*)(mux->packet.data);
    memset(pPktHdr, 0, sizeof(tFLVPktHdr));
    header_ptr = (uint8_t *) mux->packet.data + sizeof(tFLVPktHdr);

    pPktHdr->type = 0x09;
    pPktHdr->dataSize[0] = (mux->packet.curSize >> 16) & 0xFF;
    pPktHdr->dataSize[1] = (mux->packet.curSize >> 8) & 0xFF;
    pPktHdr->dataSize[2] = (mux->packet.curSize) & 0xFF;
    pPktHdr->timestamp[0] = (u32PktTs >> 16) & 0xFF;
    pPktHdr->timestamp[1] = (u32PktTs >> 8) & 0xFF;
    pPktHdr->timestamp[2] = (u32PktTs) & 0xFF;
    pPktHdr->timestampExt = (u32PktTs >> 24) & 0xFF;

    if(FrameInfo.eFrameType & eH264_I_FRAME)
        {
        if(pktType)
            *pktType = 0x17;
        *(header_ptr++) = 0x17;
        }
    else
        {
        if(pktType)
            *pktType = 0x27;
        *(header_ptr++) = 0x27;
        }

    *(header_ptr++) = 0x01; /* AVC NALU */
    *(header_ptr++) = 0;
    *(header_ptr++) = 0;
    *(header_ptr++) = 0;

    /*if(u32FrameLen > 4)
        {
        const uint32_t u32NALSize = u32FrameLen - 4;
        uint8_t *pu8NAL = (uint8_t*) frame + u32FrameOffset;

        pu8NAL[0] = (uint8_t) (u32NALSize >> 24);
        pu8NAL[1] = (uint8_t) (u32NALSize >> 16);
        pu8NAL[2] = (uint8_t) (u32NALSize >> 8);
        pu8NAL[3] = (uint8_t) u32NALSize;
        }*/

    memcpy(header_ptr, frame + u32FrameOffset, u32FrameLen);
    header_ptr += u32FrameLen;
    mux->packet.curSize += 11;

    *(header_ptr++) = ((mux->packet.curSize) >> 24) & 0xFF;
    *(header_ptr++) = ((mux->packet.curSize) >> 16) & 0xFF;
    *(header_ptr++) = ((mux->packet.curSize) >> 8) & 0xFF;
    *(header_ptr++) = (mux->packet.curSize) & 0xFF;
    mux->packet.curSize += 4;
    if(pktSize)
        *pktSize = mux->packet.curSize;
    if(FLVPacketCheck(mux->packet.data, mux->packet.curSize))
        return mux->packet.data;
    else
        return NULL;
    }


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
FLVMuxerFillAudio(void* muxer, void* frame, int frameSize, uint64_t ts, int* pktSize, int* ts_exceed)
    {
    uint8_t         *header_ptr = NULL;
    tFLVMuxer       *mux = NULL;
    uint32_t        u32PktTs = 0;
    tFLVPktHdr      *pPktHdr = NULL;

    /* Check input */
    if(muxer == NULL || frame == NULL || frameSize < 4)
        return NULL;

    mux = (tFLVMuxer*) muxer;

    /* Prepare packet buffer */
    mux->packet.curSize = frameSize / 2 + 1;
    if(mux->packet.maxSize < mux->packet.curSize)
        return NULL;

    /* Get time stamp */
    if(mux->curTimeStamp == 0)
        {
        if(ts)
            mux->curTimeStamp = ts;
        else
            mux->curTimeStamp = FLVMuxerGetTimeStamp();
        }

    if(ts)
        u32PktTs = ts - mux->curTimeStamp;
    else
        {
        u32PktTs = FLVMuxerGetTimeStamp() - mux->curTimeStamp;
        /* Reset time stamp if it over 4 bytes */
        if(u32PktTs > 0xEFFFFFFF)
            {
            mylog_warn("Time stamp exceed maximum %u. Reset to 0", u32PktTs);
            mux->curTimeStamp = FLVMuxerGetTimeStamp();
            u32PktTs = 0;
            if(ts_exceed)
                *ts_exceed = 1;
            }
        else
            {
            if(ts_exceed)
                *ts_exceed = 0;
            }
        }

    /* Prepare header */
    pPktHdr = (tFLVPktHdr*)(mux->packet.data);
    memset(pPktHdr, 0, sizeof(tFLVPktHdr));
    header_ptr = (uint8_t *) mux->packet.data + sizeof(tFLVPktHdr);

    pPktHdr->type = 0x08;
    pPktHdr->dataSize[0] = (mux->packet.curSize >> 16) & 0xFF;
    pPktHdr->dataSize[1] = (mux->packet.curSize >> 8) & 0xFF;
    pPktHdr->dataSize[2] = (mux->packet.curSize) & 0xFF;
    pPktHdr->timestamp[0] = (u32PktTs >> 16) & 0xFF;
    pPktHdr->timestamp[1] = (u32PktTs >> 8) & 0xFF;
    pPktHdr->timestamp[2] = (u32PktTs) & 0xFF;
    pPktHdr->timestampExt = (u32PktTs >> 24) & 0xFF;

    /* Sound format AAC_44kHz_16bits_stereo */
    //*(header_ptr++) = 0xAF;
    *(header_ptr++) = 0x72;

    /* Feed data */
    sound_alaw_le_enc((char*)frame, (char*)header_ptr, frameSize);
    header_ptr += (mux->packet.curSize - 1);
    mux->packet.curSize += 11;

    *(header_ptr++) = ((mux->packet.curSize) >> 24) & 0xFF;
    *(header_ptr++) = ((mux->packet.curSize) >> 16) & 0xFF;
    *(header_ptr++) = ((mux->packet.curSize) >> 8) & 0xFF;
    *(header_ptr++) = (mux->packet.curSize) & 0xFF;
    mux->packet.curSize += 4;
    if(pktSize)
        *pktSize = mux->packet.curSize;
    if(FLVPacketCheck(mux->packet.data, mux->packet.curSize))
        return mux->packet.data;
    else
        return NULL;
    }

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
FLVMuxerWriteToFile(void* muxer, void* data, int size)
    {
    tFLVMuxer   *mux = NULL;
    int         iWroteSize = 0;
    /* Check input */
    if(muxer == NULL || data == NULL || size < 4)
        return 0;

    mux = (tFLVMuxer*) muxer;
//    iWroteSize = fwrite(data, size, 1, mux->pFile);
    iWroteSize = f_mem_write(data, size, 1, mux->pFile);
    printf("Write packet size %d\n", size);
    fflush(mux->pFile);
    return iWroteSize;
    }
