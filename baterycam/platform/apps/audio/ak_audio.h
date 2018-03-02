#ifndef _AK_AUDIO_H_
#define _AK_AUDIO_H_

void *ak_audio_encode_task(void *arg);
int audio_ai_set_volume(void *pAiHandle);

int audio_set_volume_onflight(void);
int audio_set_volume_onflight_with_value(int iValue);

void *ak_audio_test_driver_task(void *arg);

int audio_loopback_send_queue(char *pBuffer, int length);
int audio_loopback_get_data(char *pBuffer, int amount);
int audio_loopback_flush_buffer(void);
#endif /* _AK_AUDIO_H_ */
