#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packet.h"
#include "encrypt.h"
#include "packet_queue.h"
#include "doorbell_config.h"

#include "ak_apps_config.h"

//#include "spi_command.h"
#define SPI_COMMAND_GET_VER         0x01
#define SPI_COMMAND_GET_UDID        0x02
#define SPI_COMMAND_SEND_M_PACKET   0x03
#define SPI_COMMAND_GET_STATUS      0x04
#define SPI_COMMAND_GET_M_PACKET    0x05
#define SPI_COMMAND_IOT_RESET       0x06
#define ENCRYPT_PORTION             (1024 + 16) // Designed value
#define ENCRYPT_LEN                 16

#if (FAST_AUDIO_MODE)
int g_start_pid_of_block_audio = 0;
int g_this_pid_of_block_audio = 0;
#endif
int g_start_pid_of_block = 0;
int g_this_pid_of_block = 0;
//extern zlog_category_t *zc;
static struct tEncryptEngine *spi_enc = NULL;

/*Mutex*/
static ak_mutex_t   mtx_packet;
//char aes_key_Khanh[] = "32424C6F366C367A4A34766A6E4C3066";
char Key_String[17] = "fIKtpLBUadrUnk3G";

/* timestamp for audio and video */
static unsigned int g_packet_audio_ts = 0;
static unsigned int g_packet_video_ts = 0;
static char acTsDebug[1024];


void packet_set_video_timestamp(unsigned int uiTimeStamp)
{
    g_packet_video_ts = uiTimeStamp;
}

void packet_set_audio_timestamp(unsigned int uiTimeStamp)
{
    g_packet_audio_ts = uiTimeStamp;
}

/*
This function must be called when system bootup
*/
int packet_init_mutex(void)
{
    int i32Ret = 0;
    i32Ret = ak_thread_mutex_init(&mtx_packet);
    if(i32Ret == -1){
        ak_print_normal("Init SPI mutex failed!\n");
    }
    return i32Ret;
}

void packet_lock_mutex(void)
{
    ak_thread_mutex_lock(&mtx_packet);
}

void packet_unlock_mutex(void)
{
    ak_thread_mutex_unlock(&mtx_packet);
}

int init_aes_engine(void) {
    int iRet = 0;
    char aes_key[_AES_KEY_MAX_BYTES_LEN_ + 1];
    // memset(aes_key, 0x00, sizeof(aes_key));
    // memcpy(aes_key, Key_String, _AES_KEY_MAX_BYTES_LEN_);
    iRet = doorbell_cfg_get_aeskey(aes_key);
    if(iRet != 0)
    {
        ak_print_error_ex("Get AES key failed!\n");
        return -1;
    }
    ak_print_normal("\n[INFO] Init AES engine - key: %s\n", aes_key);
    return init_aes_engine_key(aes_key);
}

/* init aes engine */
int init_aes_engine_key(char *aes_key) {
    int ret_val = 0;

    // dzlog_info("DEBUG-------init_aes_engine\n");
    ak_print_normal("---init_aes_engine---- [%s]\n", aes_key);
    if (aes_key == NULL) {
        ak_print_error_ex("aes_key NULL\r\n");
        goto exit;
    }
    // INIT
    spi_enc = AESEngineInit(aes_key);
    if (spi_enc == NULL) {
        ret_val = -1;
        ak_print_error_ex("init key failed! spi_enc null!\r\n");
        goto exit;
    }

exit:
    return ret_val;
}

/* deinit key aes */
void deinit_aes_engine_key(void)
{
    int ret_val = 0;
    ak_print_normal("deinit_aes_engine\n");
    if(spi_enc != NULL)
    {
        AESEngineDeInit(spi_enc);
        spi_enc = NULL;
    }
}


/* encrypt frame mode ecb with key get from cc3200 */
int encrypt_frame(char *in_packet, int in_length) {
    char *ptr_packet = NULL;
    int ret_val = 0;
    int num_portion = 0;
    int i = 0;

    int j = 0;
    char acDebug[512] = {0x00};

    // Init AES in the first time
    // NOTE: if there is a case that frame is sent before SPI init
    // how about get key failed?????
    if (spi_enc == NULL) {
        ret_val = -1;
        goto exit;
    }

    num_portion = in_length / ENCRYPT_PORTION;
    // if last portion length is greater or equal ENCRYPT_LEN, let's encrypt it
    num_portion += (in_length % ENCRYPT_PORTION >= ENCRYPT_LEN)?1:0;

    for (i = 0; i < num_portion; i++) {
        ptr_packet = in_packet + (i * ENCRYPT_PORTION);
        /*if(i == 0){
            memset(acDebug, 0x00, sizeof(acDebug));
            for(j = 0; j < ENCRYPT_LEN; j++){
                sprintf(acDebug + strlen(acDebug), "%02X ", *(ptr_packet + j));
            }
            printf("AES-IN : %s\n", acDebug);
        }*/
        // Encrypt
        Encrypt_AES_NonPadding(spi_enc, ptr_packet, ENCRYPT_LEN);

        /*if(i == 0){
            memset(acDebug, 0x00, sizeof(acDebug));
            for(j = 0; j < ENCRYPT_LEN; j++){
                sprintf(acDebug + strlen(acDebug), "%02X ", *(ptr_packet + j));
            }
            printf("AES-OUT: %s\n", acDebug);
        }*/
    }
exit:
    return ret_val;
}

/* encrypt audio frame mode ecb with key get from cc3200 */
int encrypt_audio_frame(char *in_packet, int in_length)
{
    int ret_val = 0;
    int num_block = 0;
    int i = 0;

    num_block = (in_length / _AES_BLK_SIZE_);

    // Init AES in the first time
    // NOTE: if there is a case that frame is send before SPI init
    // how about get key failed?????
    if (spi_enc == NULL) {
        //dzlog_info("spi_enc == NULL\n");
        // FIXME: need fix this
        // failed, send raw data
        ret_val = -1;
        goto exit;
    }

    for (i = 0; i < num_block; i++) {
        Encrypt_AES_NonPadding(spi_enc, in_packet + (i * _AES_BLK_SIZE_), _AES_BLK_SIZE_);
    }

exit:
    return ret_val;
}
/* API for the common packet handler: packet combine*/
int packet_padding(char *in_packet, int *in_length, char padding_char, int padding_length)
{
    int i;
    if(!in_packet || !in_length)
        return -1;
    for(i=0; i<padding_length; i++)
        in_packet[*in_length+i] = padding_char;
    *in_length +=padding_length;
    return 0;
}
int packet_combine(char *in_packet1, int in_length1, char *in_packet2, int in_length2, char *out_packet, int *out_length)
{
    if(!in_packet1 || !out_packet || !out_length)
        return -1;
    if(!in_packet2 || (in_length2==0))
    {
        memcpy(out_packet, in_packet1, in_length1);
        *out_length = in_length1;
    }else
    {
        *out_length = in_length1 + in_length2;
        memcpy(in_packet1+in_length1, in_packet2, in_length2);
        memcpy(out_packet, in_packet1, *out_length);
    }
    return 0;
}
/* API for the specific purpose: SPI header creating,  PS packet creating*/
int packet_spi_header_wrapper(char packet_type, void* option, char *header_buffer, int *header_len)
{
    unsigned char *tmp_buff = (unsigned char *)header_buffer;
    int tmp_len = 0;
    if(!header_buffer || !header_len)
        return -1;
    switch (packet_type)
    {
        case 0x03:
            /*
            "0x03<Len2><data>"
            "0x83<status 0|1>"
            Send a media packet, example 0x03+1019<0..1018>*/
            {
                int len;
                if(!option)
                    return -1;
                tmp_buff[tmp_len++] = 0x03;
                len = *(int*)option;
                /*tmp_buff[tmp_len++] = (len >> 24) & 0xFF;*/
                /*tmp_buff[tmp_len++] = (len >> 16) & 0xFF;*/
                tmp_buff[tmp_len++] = (len >> 8) & 0xFF;
                tmp_buff[tmp_len++] = (len) & 0xFF;
                *header_len = tmp_len;
            }
            break;
        case 0x01:
        case 0x02:
        case 0x04:
        case 0x05:
        case 0x06:
        default:
             tmp_buff[tmp_len++] = packet_type;
             *header_len = tmp_len;
            break;
    }
    return 0;
}
int packet_ps_header_wraper(char packet_type, int block_len, int packet_counter, 
    int block_index, int time_stamp, char *header_buffer, int *header_len, 
    int *block_pid, int *cur_pid)
{
    char *tmp_buff = header_buffer;
    int tmp_len = 0;
    if(!header_buffer || !header_len)
        return -1;

    // if(packet_type == 41)
    // {
    //     printf("fileblock:%d, file_pid:%d\r\n", block_len, (*block_pid +*cur_pid));
    // }
    tmp_buff[tmp_len++] = 'V';
    tmp_buff[tmp_len++] = 'L';
    tmp_buff[tmp_len++] = 'V';
    tmp_buff[tmp_len++] = 'L';
    tmp_buff[tmp_len++] = 0;
    tmp_buff[tmp_len++] = packet_type;

    tmp_buff[tmp_len++] = (time_stamp >> 24) & 0xFF;
    tmp_buff[tmp_len++] = (time_stamp >> 16) & 0xFF;
    tmp_buff[tmp_len++] = (time_stamp >> 8) & 0xFF;
    tmp_buff[tmp_len++] = (time_stamp) & 0xFF;

    tmp_buff[tmp_len++] = (block_len >> 8) & 0xFF;
    tmp_buff[tmp_len++] = (block_len) & 0xFF;

    tmp_buff[tmp_len++] = ((*block_pid) >> 24) & 0xFF;
    tmp_buff[tmp_len++] = ((*block_pid) >> 16) & 0xFF;
    tmp_buff[tmp_len++] = ((*block_pid) >> 8) & 0xFF;
    tmp_buff[tmp_len++] = ((*block_pid)) & 0xFF;

    tmp_buff[tmp_len++] = ((*block_pid +*cur_pid) >> 24) & 0xFF;
    tmp_buff[tmp_len++] = ((*block_pid +*cur_pid) >> 16) & 0xFF;
    tmp_buff[tmp_len++] = ((*block_pid +*cur_pid) >> 8) & 0xFF;
    tmp_buff[tmp_len++] = ((*block_pid +*cur_pid)) & 0xFF;

    (*cur_pid) += 1;
    *header_len = tmp_len;
    return 0;
}

int packet_len_to_info(int len, int max_length, int *block_len, int *last_packet_len)
{
    if(!block_len || !last_packet_len)
        return -1;
    if(len <= max_length)
    {
        *block_len = 1;
    }else
    {
        //printf("b1: %d, %d\n", send_len/MAX_DATA_LENGTH, (send_len%MAX_DATA_LENGTH)?1:0);
        *block_len = (int)(len/max_length) + (int)((len%max_length)?1:0);
    }
    *last_packet_len = len - (*block_len-1)*max_length;
    return 0;
}

#define MAX_DATA_LENGTH 999
#define PS_HEADER_LENGTH 20
#define PACKET_ENABLE_CHECKSUM      (0)


int packet_create(char packet_type, int *packet_counter, char *in_data, int in_len, char *out_data, int *out_len)
{
    int block_len, last_packet_len;
    int i, tmp_len = 0;
    int real_len;
    static unsigned int audio_time_stamp = 0;
    static unsigned int video_time_stamp = 0;
    // static unsigned int video_time_stamp = (unsigned int)video_get_timestamp();
    unsigned int *p_time_stamp = NULL;
    *out_len = 0;
    int *start_pid = NULL;
    int *current_pid = NULL;

    /*---------------------------------------------*/
    /* Debug PID */
#if (PACKET_ENABLE_CHECKSUM == 1)
    unsigned short usCsCalc = 0;
    char *pPacket = NULL;
    unsigned char acChecksumErr[2] = {0x00, 0x00};
    int iterator = 0;
    char acDebugPid[1024] = {0x00};
    memset(acDebugPid, 0x00, sizeof(acDebugPid));
#endif
    /*---------------------------------------------*/
    // dzlog_info("packet_type=%d\n", packet_type);
    // Set pid counter
    start_pid = &g_start_pid_of_block;
    current_pid = &g_this_pid_of_block;

    /* process timestamp */
    video_time_stamp = g_packet_video_ts;
    audio_time_stamp = g_packet_audio_ts;

    // Encypt h264 frame
    if ((int)packet_type / 10 == 1) {
        encrypt_frame(in_data, in_len);
        //dzlog_info("GOT VIDEO FRAME let ENCRYPT\n");
        // TODO: process time stamp here
        p_time_stamp = &video_time_stamp;
    } else if ((int)packet_type / 10 == 2) {
        encrypt_audio_frame(in_data, in_len);
        // TODO: process time stamp here
        p_time_stamp = &audio_time_stamp;
        *p_time_stamp += 32; // bitrate = 8000, bit depth = 16 -> time 32 ms for each packet 512 bytes
    }
    #if (FAST_AUDIO_MODE)
    else if((int)packet_type / 10 == eMediaTypeFastAudio) {
        encrypt_audio_frame(in_data, in_len);
        // TODO: process time stamp here
        p_time_stamp = &audio_time_stamp;
        *p_time_stamp += 32; // bitrate = 8000, bit depth = 16 -> time 32 ms for each packet 512 bytes
        // Using another pid counter for fast audio
        start_pid = &g_start_pid_of_block_audio;
        current_pid = &g_this_pid_of_block_audio;
#if (PACKET_ENABLE_CHECKSUM == 1)
        sprintf(acDebugPid, "FA: ");
#endif
    }
    #endif

    /* DEBUG_PID */
#if (PACKET_ENABLE_CHECKSUM == 1)
    sprintf(acDebugPid + strlen(acDebugPid), "DBG pid-start: %d,", *start_pid);
#endif

    /*Put spi header*/
    if(packet_len_to_info(in_len, MAX_DATA_LENGTH, &block_len, &last_packet_len)==0)
    {
        // printf("block_len %d, last_packet_len %d\n", block_len, last_packet_len);
        //dzlog_info("block_len %d, last_packet_len %d\n", block_len, last_packet_len);
        for (i=0; i<block_len; i++)
        {
            real_len = ((i==block_len-1)?last_packet_len:MAX_DATA_LENGTH) + PS_HEADER_LENGTH;
            //printf("len %d\n", real_len);

#if (PACKET_ENABLE_CHECKSUM == 1)
            pPacket = out_data + *out_len;
#endif

            if(packet_spi_header_wrapper(0x03, &real_len, out_data + *out_len, &tmp_len) == 0)
            {
                unsigned int current_len = 0;
                unsigned int spi_packet_len = 0;
                current_len = *out_len;
                *out_len += tmp_len;
                //printf("packet_spi_header_wrapper %d, %d\n", tmp_len, *out_len);
                if(packet_ps_header_wraper(packet_type, block_len, *packet_counter, i, *p_time_stamp, out_data + *out_len, &tmp_len, start_pid, current_pid)==0)
                {
                    

                    *out_len += tmp_len;
                    // printf("packet_ps_header_wraper %d, %d\n", tmp_len, *out_len);
                    // copy real data to buffer
                    if(i==block_len-1)//last packet
                    {
                        memcpy(out_data + *out_len, in_data + i*MAX_DATA_LENGTH, last_packet_len);
                        *out_len +=last_packet_len;
                    }
                    else
                    {
                        memcpy(out_data + *out_len, in_data + i*MAX_DATA_LENGTH, MAX_DATA_LENGTH);
                        *out_len +=MAX_DATA_LENGTH;
                    }

                }
                spi_packet_len = *out_len - current_len;

                // padding 1022
                if (spi_packet_len < MAX_SPI_PACKET_LEN) {
                    int padding_sz = 0;
                    padding_sz = MAX_SPI_PACKET_LEN - spi_packet_len;
                    memset(out_data + *out_len, 0, padding_sz);
                    *out_len += padding_sz;
                    /*zlog_info(zc, "padding_sz = %d\n", padding_sz);*/
                }

#if (PACKET_ENABLE_CHECKSUM == 1)
                /* Add checksum error */
                for(iterator = 0; iterator < (MAX_SPI_PACKET_LEN-2); iterator++)
                {
                    usCsCalc += *(pPacket + iterator);
                }
                *(pPacket + MAX_SPI_PACKET_LEN-2) = (char)((usCsCalc >> 8) & 0xFF);
                *(pPacket + MAX_SPI_PACKET_LEN-1) = (char)(usCsCalc & 0xFF);
                printf("CS: %x, b1022: %x, b1023: %x\n", usCsCalc, *(pPacket + MAX_SPI_PACKET_LEN-2), *(pPacket + MAX_SPI_PACKET_LEN-1));
                usCsCalc = 0;
#endif
            }
        }

	/* timestamp debug */
    #if 0
        sprintf(acTsDebug + strlen(acTsDebug), "t(%d):%u ", packet_type, *p_time_stamp);
        // printf("packet_ps (type %d) 0x%02X%02X%02X%02X, (0x%X-%u)\r\n", \
        //     packet_type, *(out_data + 6), *(out_data + 7), *(out_data + 8),\
        //     *(out_data + 9), *p_time_stamp, *p_time_stamp);
        if(packet_type == 10)
        {
            printf(acTsDebug);
            printf("\r\n");
            memset(acTsDebug, 0x00, 1024);
        }
    #endif

        // Reset index for next packet
        (*start_pid) += block_len;
        (*current_pid) = 0;
        *packet_counter = *packet_counter + 1;

#if (PACKET_ENABLE_CHECKSUM == 1)
        /* DEBUG_PID */
        sprintf(acDebugPid + strlen(acDebugPid), " end: %d, block_len: %d \n", *start_pid, block_len);
        printf("%s", acDebugPid);
#endif
    }
    return 0;
}


#if 0
int pos_send_data(char packet_type, char *in_data, int in_len)
{
    int out_len = 0;
    int tmp_len = 0;
    int ret_val = 0;
    int packet_counter = 0;
    char out_data[100*1024] = {0};

    memset(out_data, 0, sizeof(out_data));
    out_len = 0;
    tmp_len = 0;
    ret_val = packet_create(packet_type, &packet_counter, in_data, in_len, out_data, &out_len);
    if(ret_val == 0)
    {
        while(tmp_len<out_len)
        {
            //printf("Send: %d\n", tmp_len);
            //send_spi_data(out_data+tmp_len, 1024);
            scheduler_send_data((char *)out_data, MAX_SPI_PACKET_LEN, SCHEDULER_PRIORITY_LOW);  
            tmp_len += MAX_SPI_PACKET_LEN;
            //usleep(50);
        }
    }
    return 0;
}
#endif


#if 0
void main()
{
    unsigned char out_data[512] = {0};
    unsigned char test_data[512] = {0};
    int out_len;
    int test_len = 500;
    int i;
    int packet_counter = 0;
    for(i=0; i<test_len; i++)
    {
        test_data[i] = 0x48;
    }
#if 1
    packet_create(24, &packet_counter, test_data, 100, out_data, &out_len);
    printf("out_len %d\n", out_len);
    for(i=0; i<out_len; i++)
    {
        if(i%(5+20+99) == 0)
            printf("\n");
        printf("0x%02x ", out_data[i]);
    }
    printf("\n");
#endif

    for(i=0; i<test_len; i++)
    {
        test_data[i] = 0x48;
    }
    memset(out_data, 0, 512);
    out_len = 0;
    packet_create(24, &packet_counter, test_data, 200, out_data, &out_len);
    printf("out_len %d\n", out_len);
    for(i=0; i<out_len; i++)
    {
        if(i%(5+20+99) == 0)
            printf("\n");
        printf("0x%02x ", out_data[i]);
    }
    printf("\n");
    

    for(i=0; i<test_len; i++)
    {
        test_data[i] = 0x48;
    }
    memset(out_data, 0, 512);
    out_len = 0;
    packet_create(24, &packet_counter, test_data, 100, out_data, &out_len);
    printf("out_len %d\n", out_len);
    for(i=0; i<out_len; i++)
    {
        if(i%(5+20+99) == 0)
            printf("\n");
        printf("0x%02x ", out_data[i]);
    }
    printf("\n");
}
#endif

/* init aes engine */
// int init_aes_engine_decrypt(void) {
//     char aes_key[_AES_KEY_MAX_BYTES_LEN_] = {0};
//     int ret_val = -1;
//     int retry_cnt = 3;

//     dzlog_info("DEBUG-------init_aes_engine\n");
    
//     // GET KEY
//     while (retry_cnt-- > 0 && ret_val != 0) {
//         // send spi command and get key
//         // check if valid key
//         // FIXME: hard code key
//         memcpy(aes_key, key, _AES_KEY_MAX_BYTES_LEN_);
//         /*memset(aes_key, 0, _AES_KEY_MAX_BYTES_LEN_);*/
//         ret_val = 0;
//         dzlog_info("DEBUG-------AES_KEY = %s\n", aes_key);
//     }
//     if (ret_val != 0) {
//         goto exit;
//     }

//     // INIT
//     spi_dec = AESEngineInit(aes_key);
//     if (spi_dec == NULL) {
//         ret_val = -1;
//         goto exit;
//     }

// exit:
//     return ret_val;
// }


// Descrypt audio frame
/* descrypt frame mode ecb with key of GM */
static int descrypt_frame(char *in_packet, int in_length)
{
    char *ptr_packet = NULL;
    int ret_val = 0;
    int num_portion = 0;
    int i = 0;

    // Init AES in the first time
    // NOTE: if there is a case that frame is send before SPI init
    // how about get key failed?????
    if (spi_enc == NULL) {
        // FIXME: need fix this
        // failed, send raw data
        ret_val = -1;
        goto exit;
    }

    num_portion = in_length / ENCRYPT_PORTION;
    // if last portion length is greater or equal ENCRYPT_LEN, let's encrypt it
    num_portion += (in_length % ENCRYPT_PORTION >= ENCRYPT_LEN)?1:0;

    for (i = 0; i < num_portion; i++) {
        ptr_packet = in_packet + (i * ENCRYPT_PORTION);
        // Encrypt
        //Encrypt_AES_NonPadding(spi_enc, ptr_packet, ENCRYPT_LEN);
        Decrypt_AES_NonPadding(spi_enc, ptr_packet, ENCRYPT_LEN);
    }
exit:
    return ret_val;
}

int decrypt_audio_frame(char *i8Data, int i32Len)
{
    int i32Ret = 0;
    int i32NumBlock = 0;
    int i = 0;

    i32NumBlock = (i32Len / _AES_BLK_SIZE_);

    // Init AES in the first time
    // NOTE: if there is a case that frame is send before SPI init
    // how about get key failed?????
    if (spi_enc == NULL) 
    {
        // dzlog_info("spi_enc == NULL\n");
        // FIXME: need fix this
        // failed, send raw data
        i32Ret = -1;
    }

    if(i32Ret == 0)
    {
        for (i = 0; i < i32NumBlock; i++)
        {
            Decrypt_AES_NonPadding(spi_enc, \
                                i8Data + (i * _AES_BLK_SIZE_), _AES_BLK_SIZE_);
        }
    }
    return i32Ret;
}

int packet_descrypt(char *i8Data, int i32LenData)
{
    int i32Ret = 0;
    if(i8Data == NULL || i32LenData <= 0)
    {
        i32Ret = -1;
    }
    else
    {
        decrypt_audio_frame(i8Data, i32LenData);
    }
    return i32Ret;
}

/*---------------------------------------------------------------------------*/
/* For p2p sd streaming, encrypt full a block mode ecb */
/*
*  - Divide the first block into 16-byte chunks. Each chunk has 16 bytes data.
*  - Encrypt all of chunks
*  - If the last chunk is not enough 16 byte. Don't ENCRYPT it. Otherwise
*    encrypt it.
*  ---------------------------------------------------------------------------
*  - For other block, only encrypt the first 16 bytes of block.
*  - For the last block, if it is not enough 16 bytes, don't encrypt it.
*    Otherwise encrypt it.
*/
// int g_start_pid_of_block = 0;
// int g_this_pid_of_block = 0;

int g_iStartFileBlockPid = 0;
int g_iCurrentFileBlockPid = 0;

static int p2p_sd_streaming_encrypt_full_block(char *pPacketData, int iLength)
{
    int iRet = 0;
    char *pChunk = NULL;
    int iNumChunk = 0;
    int i = 0;

    if(pPacketData == NULL || iLength <= 0)
    {
        printf("%s Parameter is NULL or iLength (%d) <= 0\r\n", \
                                                __func__, iLength);
        return -1;   
    }
    // Init AES in the first time
    // NOTE: if there is a case that frame is sent before SPI init
    // how about get key failed?????
    if (spi_enc == NULL)
    {
        printf("EEROR: need to init engine AES\r\n");
        iRet = -1;
    }
    else
    {
        iNumChunk = iLength / ENCRYPT_LEN;
        for (i = 0; i < iNumChunk; i++)
        {
            pChunk = pPacketData + (i * ENCRYPT_LEN);
            // Encrypt
            Encrypt_AES_NonPadding(spi_enc, pChunk, ENCRYPT_LEN);
        }
    }
    return iRet;
}

static int p2p_sd_streaming_encrypt_head_block(char *pPacketData, int iLength)
{
    int iRet = 0;

    if(pPacketData == NULL || iLength <= 0)
    {
        printf("%s Parameter is NULL or iLength (%d) <= 0\r\n", \
                                                __func__, iLength);
        return -1;   
    }
    if(iLength < ENCRYPT_LEN)
    {
        // no need to encrypt
        return 0;
    }

    if (spi_enc == NULL)
    {
        printf("EEROR: need to init engine AES\r\n");
        iRet = -1;
    }
    else
    {
        // Encrypt
        Encrypt_AES_NonPadding(spi_enc, pPacketData, ENCRYPT_LEN);
    }
    return iRet;
}


int p2p_packet_len_to_info(int len, int max_length, int *block_len, int *last_packet_len)
{
    if(!block_len || !last_packet_len)
        return -1;
    if(len <= max_length)
    {
        *block_len = 1;
    }else
    {
        //printf("b1: %d, %d\n", send_len/MAX_DATA_LENGTH, (send_len%MAX_DATA_LENGTH)?1:0);
        *block_len = (int)(len/max_length) + (int)((len%max_length)?1:0);
    }
    *last_packet_len = len - (*block_len-1)*max_length;
    return 0;
}

int p2p_packet_set_size_stream(long lSize)
{
    
}

/*
* Only for p2p_sdcard_streaming
*/
// int packet_create_p2p_sd_streaming(char cPacketType, int *pPacketCnt, 
    // char *pcDataInput, int iLenInput, char *pcOutData, int *pLenOutput)
int packet_create_p2p_sd_streaming(char packet_type, int *packet_counter, 
    char *in_data, int in_len, char *out_data, int *out_len)
{
    int iRet = 0;
    int iFilePid = 0;
    char *pFirstBlockData = NULL; // for first block
    int iLengthFirstBlock = 0; // for first block
    char *pOtherBlockData = NULL; // for other block
    int iLengthOtherBlock = 0;
    int iEncNum = 0;


    int block_len, last_packet_len;
    int i, tmp_len = 0;
    int real_len;
    int *start_pid = NULL;
    int *current_pid = NULL;

    *out_len = 0;
    /*---------------------------------------------*/
    /* Debug PID */
#ifdef P2P_SDCARD_STREAMING_DEBUG
    unsigned short usCsCalc = 0;
    char *pPacket = NULL;
    unsigned char acChecksumErr[2] = {0x00, 0x00};
    int iterator = 0;
    char acDebugPid[1024] = {0x00};
    memset(acDebugPid, 0x00, sizeof(acDebugPid));
#endif
    /*---------------------------------------------*/
    
    // Set pid counter
    g_iStartFileBlockPid = 0;
    start_pid = &g_iStartFileBlockPid;
    current_pid = &g_iCurrentFileBlockPid;

    if(packet_type != 41)
    {
        printf("%s don't support this packet type (%d)\r\n", __func__, packet_type);
        iRet = -1;
    }
    else
    {
        // For encrypt packet. first packet and other packet
        // if(current_pid == 0) // for first block
        // {
            pFirstBlockData = in_data;

            printf("1.Raw(%d): ", *current_pid);
            for(i = 0; i < 16; i++)
            {
                printf("%02X ", *(pFirstBlockData + i));
            }

            p2p_sd_streaming_encrypt_full_block(pFirstBlockData, MAX_DATA_LENGTH);

            printf(" -- Enc: ");
            for(i = 0; i < 16; i++)
            {
                printf("%02X ", *(pFirstBlockData + i));
            }

            if(in_len > MAX_DATA_LENGTH)
            {
                iLengthOtherBlock = in_len - MAX_DATA_LENGTH; // first block has 999 bytes!
            }
            pOtherBlockData = in_data + MAX_DATA_LENGTH;
        // }
        
        iEncNum = iLengthOtherBlock/MAX_DATA_LENGTH;
        if((iLengthOtherBlock % MAX_DATA_LENGTH) >= ENCRYPT_LEN)
        {
            iEncNum++;
        }
        for(i=0; i < iEncNum; i++)
        {
            p2p_sd_streaming_encrypt_head_block(pOtherBlockData + i*MAX_DATA_LENGTH, iLengthOtherBlock - i*MAX_DATA_LENGTH);
        }
    
        /* DEBUG_PID */
    #ifdef P2P_SDCARD_STREAMING_DEBUG
        sprintf(acDebugPid + strlen(acDebugPid), "DBG pid-start: %d,", *start_pid);
    #endif
        /*Put spi header*/
        if(packet_len_to_info(in_len, MAX_DATA_LENGTH, &block_len, &last_packet_len) == 0)
        {
            for (i=0; i<block_len; i++)
            {
                real_len = ((i==block_len-1)?last_packet_len:MAX_DATA_LENGTH) + PS_HEADER_LENGTH;

    #ifdef P2P_SDCARD_STREAMING_DEBUG
                pPacket = out_data + *out_len;
    #endif

                if(packet_spi_header_wrapper(0x03, &real_len, out_data + *out_len, &tmp_len) == 0)
                {
                    unsigned int current_len = 0;
                    unsigned int spi_packet_len = 0;
                    current_len = *out_len;
                    *out_len += tmp_len;
                    // timestamp 0
                    // NOTE: start_pid must be 0
    #ifdef P2P_SDCARD_STREAMING_DEBUG
                    // printf("start_pid(0): %d, current_pid: %d, block: %d\r\n", *start_pid, *current_pid, i);
    #endif
                    if(packet_ps_header_wraper(packet_type, block_len, *packet_counter, i, 0, \
                        out_data + *out_len, &tmp_len, start_pid, current_pid)==0)
                    {
                        *out_len += tmp_len;
                        //printf("packet_ps_header_wraper %d, %d\n", tmp_len, *out_len);
                        // copy real data to buffer
                        if(i==block_len-1)//last packet
                        {
                            memcpy(out_data + *out_len, in_data + i*MAX_DATA_LENGTH, last_packet_len);
                            *out_len +=last_packet_len;
                        }
                        else
                        {
                            memcpy(out_data + *out_len, in_data + i*MAX_DATA_LENGTH, MAX_DATA_LENGTH);
                            *out_len +=MAX_DATA_LENGTH;
                        }

                    }
                    spi_packet_len = *out_len - current_len;

                    // padding 1022
                    if (spi_packet_len < MAX_SPI_PACKET_LEN) {
                        int padding_sz = 0;
                        padding_sz = MAX_SPI_PACKET_LEN - spi_packet_len;
                        memset(out_data + *out_len, 0, padding_sz);
                        *out_len += padding_sz;
                        /*zlog_info(zc, "padding_sz = %d\n", padding_sz);*/
                    }

    // #ifdef P2P_SDCARD_STREAMING_DEBUG
    //                 /* Add checksum error */
    //                 for(iterator = 0; iterator < (MAX_SPI_PACKET_LEN-2); iterator++)
    //                 {
    //                     usCsCalc += *(pPacket + iterator);
    //                 }
    //                 *(pPacket + MAX_SPI_PACKET_LEN-2) = (char)((usCsCalc >> 8) & 0xFF);
    //                 *(pPacket + MAX_SPI_PACKET_LEN-1) = (char)(usCsCalc & 0xFF);
    //                 printf("CS: %x, b1022: %x, b1023: %x\n", usCsCalc, *(pPacket + MAX_SPI_PACKET_LEN-2), *(pPacket + MAX_SPI_PACKET_LEN-1));
    //                 usCsCalc = 0;
    // #endif
                }
            }
            // Reset index for next packet
            (*start_pid) = 0;
            (*current_pid) = 0;
            *packet_counter = *packet_counter + 1;

    #ifdef P2P_SDCARD_STREAMING_DEBUG
            /* DEBUG_PID */
            sprintf(acDebugPid + strlen(acDebugPid), " end: %d, block_len: %d \n", *start_pid, block_len);
            printf("%s", acDebugPid);
    #endif
        }
    } // end of if packet type  != 41
    return iRet;
}

int packet_p2psdstream_get_current_file_id(void)
{
    return g_iCurrentFileBlockPid;
}
void packet_p2psdstream_set_current_file_id(int current_file_id)
{
    g_iCurrentFileBlockPid = current_file_id;
}

/*---------------------------------------------------------------------------*/
/*                              END OF FILE                                  */
/*---------------------------------------------------------------------------*/
