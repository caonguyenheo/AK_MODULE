#ifndef _PARSE_FRAME_H264_H_
#define _PARSE_FRAME_H264_H_

#include <stdint.h>

typedef enum{
    eH264_SPS_UNKNOWN = 0x00000000,
    eH264_SPS_FRAME = 0x00000001,
    eH264_PPS_FRAME = 0x00000002,
    eH264_I_FRAME   = 0x00000004,
    eH264_P_FRAME   = 0x00000008,
}E_H264_FRAMETYPE_MASK;

typedef struct{
    E_H264_FRAMETYPE_MASK eFrameType;
    uint32_t u32SPSOffset;
    uint32_t u32SPSLen;
    uint32_t u32PPSOffset;
    uint32_t u32PPSLen;
    uint32_t u32IPOffset;
    uint32_t u32IPLen;
}S_H264_FRAME_INFO;


void ParseFrame(
    uint8_t *pu8BitStream,
    uint32_t u32BitStreamLen,
    S_H264_FRAME_INFO *psFrameInfo
);

#endif /* _PARSE_FRAME_H264_H_ */

