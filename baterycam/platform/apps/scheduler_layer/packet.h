#ifndef _SPI_PACKET_
#define _SPI_PACKET_

#define FAST_AUDIO_MODE         1

int init_aes_engine(void);

int packet_create(char packet_type, int *packet_counter, char *in_data, int in_len, char *out_data, int *out_len);
int packet_init_mutex(void);
void packet_lock_mutex(void);
void packet_unlock_mutex(void);
int packet_descrypt(char *i8Data, int i32LenData);
int encrypt_audio_frame(char *in_packet, int in_length);
int init_aes_engine_key(char *aes_key);

void deinit_aes_engine_key(void);

int packet_create_p2p_sd_streaming(char packet_type, int *packet_counter, 
    char *in_data, int in_len, char *out_data, int *out_len);

void packet_set_video_timestamp(unsigned int uiTimeStamp);    
void packet_set_audio_timestamp(unsigned int uiTimeStamp);


#endif /* _SPI_PACKET_ */

