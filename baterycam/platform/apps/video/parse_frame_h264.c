#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "parse_frame_h264.h"

void
ParseFrame(
    uint8_t *pu8BitStream,
    uint32_t u32BitStreamLen,
    S_H264_FRAME_INFO *psFrameInfo
)
{
    uint32_t u32NextSyncWordAdr = 0;

    memset(psFrameInfo, 0x00, sizeof(S_H264_FRAME_INFO));

    if((pu8BitStream[4] & 0x1F) == 0x05){
        // It is I-frame
        psFrameInfo->u32IPLen = u32BitStreamLen;
        psFrameInfo->eFrameType = eH264_I_FRAME;
        //dzlog_info("Got IFrame ");
        printf("Got IFrame ");
        return;
    }
    else if ((pu8BitStream[4] & 0x1F) == 0x01){
        // It is P-frame
        psFrameInfo->u32IPLen = u32BitStreamLen;
        psFrameInfo->eFrameType = eH264_P_FRAME;
        //dzlog_info("Got PFrame ");
        printf("Got PFrame ");
        return;
    }
    else 
    {
        // Search I/P frame
        uint32_t u32SyncWord = 0x01000000;
        uint32_t u32Len = (u32BitStreamLen - 4); //4:sync word len
        uint8_t u8FrameType;
        E_H264_FRAMETYPE_MASK eCurFrameType = eH264_SPS_UNKNOWN;
        
        while(u32Len)
        {
            if(memcmp(pu8BitStream + u32NextSyncWordAdr, &u32SyncWord, sizeof(uint32_t)) == 0)
            {
                u8FrameType = pu8BitStream[u32NextSyncWordAdr + 4] & 0x1F;

                if(eCurFrameType == eH264_SPS_FRAME){
                    psFrameInfo->u32SPSLen = u32NextSyncWordAdr - psFrameInfo->u32SPSOffset;
                }
                else if(eCurFrameType == eH264_PPS_FRAME){
                    psFrameInfo->u32PPSLen = u32NextSyncWordAdr - psFrameInfo->u32PPSOffset;
                }

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
                else 
                {
                    printf("Got other frame type **** (%x) \n", u8FrameType);
                }

            }
            u32NextSyncWordAdr ++;
            u32Len --;
        }

        if(u32Len == 0){
            //dzlog_info("RTSP plugin: Unknown H264 frame \n");
            u32NextSyncWordAdr = 0;
        }
    }

    return;
}


