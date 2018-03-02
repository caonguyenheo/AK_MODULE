#ifndef _AK_SCHEDULER_H_
#define _AK_SCHEDULER_H_

// #ifndef TRANSFER_SCHEDULER_
#include "ringbuffer.h"
// #define TRANSFER_SCHEDULER_

#define MAX_BUFFER_LENGTH       (256*1024)
#define DATA_SIZE               (1024)

#define SCH_TYPE_CMD_VERSION            1
#define SCH_TYPE_CMD_UDID               2
#define SCH_TYPE_DATA_STREAMING         3
#define SCH_TYPE_CMD_STATUS             4
#define SCH_TYPE_CMD_GET_PACKET         5
#define SCH_TYPE_CMD_RESET              6
#define SCH_TYPE_CMD_TCP                7
#define SCH_TYPE_CMD_UDP                8
#define SCH_TYPE_CMD_PING               9
#define SCH_TYPE_CMD_GET_RESP           10
#define SCH_TYPE_SPI_CMD_REQUEST        11
#define SCH_TYPE_SPI_CMD_RESPONSE       12

typedef enum scheduler_mode{
    E_SCHEDULER_MODE_P2P = 0,
    E_SCHEDULER_MODE_IP,
    E_SCHEDULER_MODE_NUM
}E_SCHEDULER_MODE_TYPE;


typedef void (*F_SCHEDULER_CALLBACK)(char *data_buff, int data_length);

typedef int (*F_SEND_METHOD)(char *data_buff, int data_length);
typedef int (*F_RECV_METHOD)(char *data_buff, int data_length);

typedef enum scheduler_priority
{
    SCHEDULER_PRIORITY_HIGH = 0,
    SCHEDULER_PRIORITY_LOW,
    SCHEDULER_PRIORITY_SPI_CMD,     // SPI CMD
    SCHEDULER_PRIORITY_NUM
} E_SCHEDULE_PRIORITY;

typedef enum scheduler_data_type
{
    SCHEDULER_DATA_TYPE_P2P = 0,
    SCHEDULER_DATA_TYPE_IP,
    SCHEDULER_DATA_TYPE_CONTROL,

    SCHEDULER_DATA_TYPE_NUM
    
} E_SCHEDULE_DATA_TYPE;
typedef struct scheduler_recv_handle_t_
{
    F_SCHEDULER_CALLBACK transfer_recv_handler;
} scheduler_recv_handle_t;

typedef struct scheduler_send_handle_t_
{
    RingBuffer *transfer_send_buffer;
    int is_init;
} scheduler_send_handle_t;

void scheduler_lock_data_buf(void);
void scheduler_unlock_data_buf(void);

int scheduler_init(E_SCHEDULE_DATA_TYPE data_type, F_SCHEDULER_CALLBACK f_callback, F_SEND_METHOD send_method, F_RECV_METHOD recv_method);
int scheduler_send_data(char *data_buff, int data_length, E_SCHEDULE_PRIORITY data_pri);
int scheduler_p2p_avail_data(void);
int scheduler_p2p_recv_data(char *pBuff, int iLength);
void scheduler_start(void);

#endif //_AK_SCHEDULER_H_
