#ifndef __SOUND_CONV_H__
#define __SOUND_CONV_H__

void sound_alaw_le_enc(char *pc_s16_le_data   /* contains 320 char */,
                       char *pc_alaw_data  /* contains 160 char */,
                       int i32_pcm_size);

#endif // #define __SOUND_CONV_H__
