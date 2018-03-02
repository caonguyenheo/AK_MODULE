#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "audio_conv.h"

int sound_val_seg(short i32Val)
    {
    int i32Ret = 0;
    i32Val >>= 7;
    if (i32Val & 0xf0)
        {
        i32Val >>= 4;
        i32Ret += 4;
        }
    if (i32Val & 0x0c)
        {
        i32Val >>= 2;
        i32Ret += 2;
        }
    if (i32Val & 0x02)
        {
        i32Ret += 1;
        }
    return i32Ret;
    }

unsigned char sound_s16_to_alaw(short i32PcmVal)
    {
    int	i32Mask = 0;
    int	i32Seg = 0;
    unsigned char ucAVal = 0;

    if (i32PcmVal >= 0)
        {
        i32Mask = 0xD5;
        }
    else
        {
        i32Mask = 0x55;
        i32PcmVal = -i32PcmVal;
        if (i32PcmVal > 0x7fff)
            i32PcmVal = 0x7fff;
        }

    if (i32PcmVal < 256)
        ucAVal = i32PcmVal >> 4;
    else
        {
        /* Convert the scaled magnitude to segment number. */
        i32Seg = sound_val_seg(i32PcmVal);
        ucAVal = (i32Seg << 4) | ((i32PcmVal >> (i32Seg + 3)) & 0x0f);
        }
    return ucAVal ^ i32Mask;
    }

int sound_alaw_to_s16(unsigned char ucAVal)
    {
    int	i32t = 0;
    int	i32Seg = 0;

    ucAVal ^= 0x55;
    i32t = ucAVal & 0x7f;
    if (i32t < 16)
        i32t = (i32t << 4) + 8;
    else
        {
        i32Seg = (i32t >> 4) & 0x07;
        i32t = ((i32t & 0x0f) << 4) + 0x108;
        i32t <<= i32Seg -1;
        }
    return ((ucAVal & 0x80) ? i32t : -i32t);
    }

unsigned char sound_s16_to_ulaw(int i32PcmVal)	/* 2's complement (16-bit range) */
    {
    int i32Mask = 0;
    int i32Seg = 0;
    unsigned char ucUVal = 0;

    if (i32PcmVal < 0)
        {
        i32PcmVal = 0x84 - i32PcmVal;
        i32Mask = 0x7f;
        }
    else
        {
        i32PcmVal += 0x84;
        i32Mask = 0xff;
        }
    if (i32PcmVal > 0x7fff)
        i32PcmVal = 0x7fff;

    /* Convert the scaled magnitude to segment number. */
    i32Seg = sound_val_seg(i32PcmVal);

    /*
     * Combine the sign, segment, quantization bits;
     * and complement the code word.
     */
    ucUVal = (i32Seg << 4) | ((i32PcmVal >> (i32Seg + 3)) & 0x0f);
    return ucUVal ^ i32Mask;
    }

int sound_ulaw_to_s16(unsigned char ucUVal)
    {
    int t;

    /* Complement to obtain normal u-law value. */
    ucUVal = ~ucUVal;

    /*
     * Extract and bias the quantization bits. Then
     * shift up by the segment number and subtract out the bias.
     */
    t = ((ucUVal & 0x0f) << 3) + 0x84;
    t <<= (ucUVal & 0x70) >> 4;

    return ((ucUVal & 0x80) ? (0x84 - t) : (t - 0x84));
    }

void sound_mulaw_dec(char *pc_mulaw_data /* contains 160 char */,
                     char *pc_s16_data    /* contains 320 char */ )
    {
    int i = 0;
    for(i=0; i<160; i++)
        {
        *((signed short*)pc_s16_data)=sound_ulaw_to_s16( (unsigned char) pc_mulaw_data[i]);
        pc_s16_data+=2;
        }
    }

void sound_mulaw_enc(char *pc_s16_data    /* contains pcm_size char */,
                     char *pc_mulaw_data  /* contains pcm_size/2 char */,
                     int i32_pcm_size)
    {
    int i = 0;
    int limit = i32_pcm_size/2;

    for(i=0; i<limit; i++)
        {
        pc_mulaw_data[i]=sound_s16_to_ulaw( *((signed short*)pc_s16_data) );
        pc_s16_data+=2;
        }
    }

void sound_alaw_dec(char *pc_alaw_data   /* contains 160 char */,
                    char *pc_s16_data    /* contains 320 char */,
                    int i32Limit )
    {
    int i = 0;
    for(i=0; i<i32Limit; i++)
        {
        ((signed short*)pc_s16_data)[i]=sound_alaw_to_s16( (unsigned char) pc_alaw_data[i]);
        }
    }

void sound_alaw_enc(char *pc_s16_data   /* contains 320 char */,
                    char *pc_alaw_data  /* contains 160 char */,
                    int i32_pcm_size)
    {
    int   i = 0;
    int   limit = i32_pcm_size/2;
    short s16;
    char  *pS16 = (char*)(&s16);

    for(i=0; i<limit; i++)
        {
        pS16[0] = pc_s16_data[1];
        pS16[1] = pc_s16_data[0];
        pc_alaw_data[i] = sound_s16_to_alaw(s16);
        pc_s16_data += 2;
        }
    }
void sound_alaw_le_enc(char *pc_s16_le_data   /* contains 320 char */,
                       char *pc_alaw_data  /* contains 160 char */,
                       int i32_pcm_size)
    {
    int i = 0;
    int limit = i32_pcm_size/2;
    short s16;
    char  *pS16 = (char*)(&s16);

    for(i=0; i<limit; i++)
        {
        pS16[0] = pc_s16_le_data[0];
        pS16[1] = pc_s16_le_data[1];
        pc_alaw_data[i] = sound_s16_to_alaw(s16);
        pc_s16_le_data += 2;
        }
    }

