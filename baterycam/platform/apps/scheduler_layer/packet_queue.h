#ifndef __PACKET_QUEUE_H__
#define __PACKET_QUEUE_H__
#include <stdint.h>
#include "pqueue.h"

typedef enum eMediaErrorCode
    {
    eMediaErrorObsolete             = -16,
    eMediaErrorOutOfRange,
    eMediaErrorTimeout,
    eMediaErrorNotSent,
    eMediaErrorWaitingForResponse,
    eMediaErrorTerminated           = -2,
    eMediaErrorFailed               = -1,
    eMediaErrorOK                   = 0,
    }eMediaErrorCode;

// typedef enum eMediaType
//     {
//     eMediaTypeNull = 0,
//     eMediaTypeVideo,
//     eMediaTypeAudio,
//     eMediaTypeCommand,
//     eMediaTypeFile,                 //No more supported
//     eMediaTypeACK,
//     eMediaTypeTalkback,
//     eMediaTypeUserDefine,
//     }eMediaType;

// typedef enum eMediaSubType
//     {
//     eMediaSubTypeNULL               = eMediaTypeNull * 10,
//     eMediaSubTypeVideoIFrame        = eMediaTypeVideo * 10,
//     eMediaSubTypeVideoBFrame,
//     eMediaSubTypeVideoPFrame,
//     eMediaSubTypeAudioAlaw          = eMediaTypeAudio * 10,
//     eMediaSubTypeAudioPCM,
//     eMediaSubTypeAudioADPCM,
//     eMediaSubTypeAudioG722,
//     eMediaSubTypeCommandRequest     = eMediaTypeCommand * 10,
//     eMediaSubTypeCommandResponse,
//     eMediaSubTypeCommandOpenStream,
//     eMediaSubTypeCommandStopStream,
//     eMediaSubTypeCommandAccessStream,
//     eMediaSubTypeCommandCloseStream,
//     eMediaSubTypeMediaACK           = eMediaTypeACK * 10,
//     eMediaSubTypeTalkback           = eMediaTypeTalkback * 10,
//     eMediaSubTypeUserDefine         = eMediaTypeUserDefine * 10,
//     }eMediaSubType;

typedef enum eMediaType
    {
    eMediaTypeNull = 0,
    eMediaTypeVideo,
    eMediaTypeAudio,
    eMediaTypeCommand,
    eMediaTypeFile,                 //No more supported
    eMediaTypeACK,
    eMediaTypeTalkback,
    eMediaTypeUserDefine,
    eMediaTypeFastAudio,
    } eMediaType;

typedef enum eMediaSubType
    {
    eMediaSubTypeNULL               = eMediaTypeNull * 10,
    eMediaSubTypeVideoIFrame        = eMediaTypeVideo * 10,
    eMediaSubTypeVideoBFrame,
    eMediaSubTypeVideoPFrame,
    eMediaSubTypeVideoJPEG,
    eMediaSubTypeAudioAlaw          = eMediaTypeAudio * 10,
    eMediaSubTypeAudioPCM,
    eMediaSubTypeAudioADPCM,
    eMediaSubTypeAudioG722,
    eMediaSubTypeAudioAAC,
    eMediaSubTypeCommandRequest     = eMediaTypeCommand * 10,
    eMediaSubTypeCommandResponse,
    eMediaSubTypeCommandOpenStream,
    eMediaSubTypeCommandStopStream,
    eMediaSubTypeCommandAccessStream,
    eMediaSubTypeCommandCloseStream,
    eMediaSubTypeMediaACK           = eMediaTypeACK * 10,
    eMediaSubTypeTalkback           = eMediaTypeTalkback * 10,
    eMediaSubTypeTalkback_G722_16khz,
    eMediaSubTypeUserDefine         = eMediaTypeUserDefine * 10,
    eMediaSubTypeFastAudioAlaw      = eMediaTypeFastAudio * 10,
    eMediaSubTypeFastAudioPCM,
    eMediaSubTypeFastAudioADPCM,
    eMediaSubTypeFastAudioG722,
    eMediaSubTypeFastAudioAAC,
    eMediaSubTypeFastAudioG722_16khz,
    } eMediaSubType;
    
// Input frame local storage structure
typedef struct packet_header
    {
    uint32_t        PID;                //used at received
    uint32_t        packet_size;          //real packet size
    uint32_t        time_stamp;         //time stamp of this frame
    uint32_t        recv_time_stamp;    //time stamp when received this frame
    int32_t         block_PID;          //start PID of frame
    uint32_t        block_length;       //PID length of frame
    eMediaSubType   packet_sub_type;       //type of frame
    void           *packet_data;          //frame data pointer
    }packet_header_t;

typedef struct media_frame {
    uint32_t        PID;
    uint32_t        frame_size;
    uint32_t        time_stamp;         //time stamp of this frame
    eMediaSubType   packet_sub_type;       //type of frame
    void           *frame_data;
} media_frame_t;
    
int32_t pop_block_frame(pqueue_t *pq, media_frame_t *frame);
int32_t pop_packet(pqueue_t *pq, packet_header_t **packet);
int32_t peek_packet(pqueue_t *pq, packet_header_t **packet);
int32_t push_packet(pqueue_t *pq, packet_header_t *packet);
int32_t create_packet(packet_header_t **packet, uint8_t *data, int32_t data_len);
int32_t delete_packet(packet_header_t *packet);

int32_t get_audio_frame(uint8_t *frame, uint32_t buffer_len, uint32_t *frame_len);

int packet_queue_test(void);
void packet_process_thread(void *arg);
void main_pop_frame(void *arg);
#endif
