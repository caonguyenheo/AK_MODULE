#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include "transfer_scheduler.h"
#include "spi_transfer.h"
#include "ak_apps_config.h"

/*
* We have two mode.
* when system power on, it will enter mode P2P to streaming.
* After app stops streaming, DoorBell will enter mode IP to upload file record.
* After uploading finish, DoorBell will be turn off by CC3200! This make it save 
* energy.
*/

static E_SCHEDULER_MODE_TYPE      g_eSchedulerMode = E_SCHEDULER_MODE_P2P;   

/* Porting for AKOS */
#define pthread_mutex_t             ak_mutex_t
#define pthread_mutex_init(a)       ak_thread_mutex_init(a)
#define pthread_mutex_deinit(a)     ak_thread_mutex_destroy(a)

#define pthread_mutex_lock(a)       ak_thread_mutex_lock(a)
#define pthread_mutex_trylock(a)    ak_thread_mutex_lock(a)
#define pthread_mutex_unlock(a)     ak_thread_mutex_unlock(a)


static uint8_t p2p_config_buf[SPI_TRAN_SIZE] = {0};

static pthread_mutex_t data_buff_mutex = NULL;
static pthread_mutex_t scheduler_handler_thread = NULL;

scheduler_send_handle_t send_handler[SCHEDULER_PRIORITY_NUM];// Control the send session
scheduler_recv_handle_t recv_handler[SCHEDULER_DATA_TYPE_NUM];// control the recv session

static RingBuffer *g_pRbP2p = NULL;

F_SEND_METHOD f_send = NULL;
F_RECV_METHOD f_recv = NULL;
int thread_started = 0;

// FIX ME
static int g_iIOSWaitResponse = 0;

void scheduler_set_wait_response(void){
    g_iIOSWaitResponse = 1;
}

void scheduler_set_dont_wait_response(void){
    g_iIOSWaitResponse = 0;
}


void scheduler_lock_data_buf(void){
    pthread_mutex_lock(&data_buff_mutex);
}
void scheduler_unlock_data_buf(void){
    pthread_mutex_unlock(&data_buff_mutex);
}


uint64_t get_time(){
    // struct timeval tp;
    // gettimeofday(&tp, NULL);
    // return (tp.tv_sec * 1000 + tp.tv_usec / 1000);
    return get_tick_count();
}

void *scheduler_thread(void *pvParameter);

int read_ringbuf(uint8_t* data, RingBuffer* ring_buffer, int len){
    if(!ring_buffer){
        printf("%s Ringbuffer null!\n\r", __func__);
        return -1;
    }
    if(RingBuffer_empty(ring_buffer)){
        return -1;
    }
    RingBuffer_read( ring_buffer, data, len);
    //ak_print_normal("readRB\n");
    return len;
}


// API get set mode
E_SCHEDULER_MODE_TYPE scheduler_get_mode(void){
    return g_eSchedulerMode;
}

void scheduler_set_mode(E_SCHEDULER_MODE_TYPE mode){
    if((mode != g_eSchedulerMode) && (mode > 0) && (mode < E_SCHEDULER_MODE_NUM)){
        g_eSchedulerMode = mode;
    }
}


void scheduler_p2p_cb(char *pData, int iLength)
{
    if(!g_pRbP2p){
        g_pRbP2p = RingBuffer_create(4*1024); // should be small, real-time
        if(!g_pRbP2p){
            return;
        }
    }

    if(!pData || (iLength <= 0)){
        return;
    }
    
    //if(RingBuffer_available_space(g_pRbP2p) >= iLength){
        RingBuffer_write(g_pRbP2p, pData, iLength);
    //}
    return;
}


int scheduler_init(E_SCHEDULE_DATA_TYPE data_type, F_SCHEDULER_CALLBACK f_callback, F_SEND_METHOD send_method, F_RECV_METHOD recv_method)
{
    int i;
    ak_pthread_t scheduler_id = NULL;

    if(pthread_mutex_init(&data_buff_mutex) != 0){
        printf("AK: init data_buff_mutex failed!\n");
        return -1;
    }
    if(pthread_mutex_init(&scheduler_handler_thread) != 0){
        printf("AK: init scheduler_handler_thread failed!\n");
        return -1;
    }
    
    //init recv handler
    recv_handler[data_type].transfer_recv_handler = f_callback;

    //Trung: hardcode for IPOVERSPI
    // TODO: need to implement IOS
    //recv_handler[SCHEDULER_DATA_TYPE_IP].transfer_recv_handler = ios_handle_spi_command_handler;
    printf("%s HARDCODE recv_handler transfer_recv_handler NULL\n", __func__);
    recv_handler[SCHEDULER_DATA_TYPE_IP].transfer_recv_handler = NULL;
    
    //init send handler
    for(i=0; i<SCHEDULER_PRIORITY_NUM; i++){
        if(send_handler[i].is_init==0){
            // FIX ME: decrease latency audio, we set size of ringbuffer small.
            send_handler[i].transfer_send_buffer = RingBuffer_create(MAX_BUFFER_LENGTH);
            send_handler[i].is_init = 1;
            if(send_handler[i].transfer_send_buffer == NULL)
            {
                ak_print_error("Scheduler init ringbuffer [%d] FAILED!\n", i);
            }
        }
    }
    // register real transfer method
    f_send = send_method;
    f_recv = recv_method;

#if 0
    // The first, we must init spi task
    if(spi_get_status_task() == 0){
        xTaskCreate(spi_task, "Spi_task", 10000, NULL, TASK_SPI_PRIORITY, NULL);
    }
#endif

    #ifdef CFG_USE_SCHEDULER
    if(thread_started==0)
    {
        ak_thread_create(&scheduler_id, scheduler_thread, NULL, 32*1024, CFG_SCHEDULER_TASK_PRIO);
        thread_started = 1;
        printf("scheduler_id: %d\n", scheduler_id);
    }
    #endif
    
    return 0;
}
int scheduler_send_data(char *data_buff, int data_length, E_SCHEDULE_PRIORITY data_pri)
{
    int index = 0;
    int length_rb = 0;
    int ret_write = 0;
    
    if(!data_buff){
        ak_print_error("data_buff NULL!\n");
        return -1;
    }

    if(data_pri == SCHEDULER_PRIORITY_HIGH){
        index = 0;
    }
    else if(data_pri == SCHEDULER_PRIORITY_LOW){
        index = 1;
    }
    else if(data_pri == SCHEDULER_PRIORITY_SPI_CMD){
        index = 2;
    }
    else{
        index = 3;
    }

    length_rb = RingBuffer_available_space(send_handler[index].transfer_send_buffer);
    if(length_rb >= data_length ){
        //pthread_mutex_lock(&data_buff_mutex);
        ret_write = RingBuffer_write(send_handler[index].transfer_send_buffer, data_buff, data_length);
        //thread_mutex_unlock(&data_buff_mutex);
        //ak_print_normal("+");
    }
    else{
        // ak_print_error("%s length_rb (%d) < data_length (%d)\n", __func__, length_rb, data_length);
        ak_print_error("@");
        return -1;
    }
    return 0;
}

int scheduler_p2p_avail_data(void)
{
    int iRet = 0;
    if(g_pRbP2p == NULL){
        return -1;
    }
    iRet = RingBuffer_available_data(g_pRbP2p);
    if(iRet > 0){
        return iRet;
    }
    else{
        return 0;
    }
}

int scheduler_p2p_recv_data(char *pBuff, int iLength)
{
    int iRet = 0;
    if(!pBuff || !iLength){
        return -1;
    }

    // if(RingBuffer_available_data(g_pRbP2p) >= iLength){
    //     iRet = RingBuffer_read(g_pRbP2p, pBuff, iLength);
    // }
    iRet = RingBuffer_read(g_pRbP2p, pBuff, iLength);
    return iRet;
}



int scheduler_update_priority(E_SCHEDULE_PRIORITY *current_pri)
{
    switch(*current_pri){
        case SCHEDULER_PRIORITY_HIGH:
            *current_pri = SCHEDULER_PRIORITY_LOW;
            break;
        case SCHEDULER_PRIORITY_LOW:
            *current_pri = SCHEDULER_PRIORITY_HIGH;
            break;
        default:
            *current_pri = SCHEDULER_PRIORITY_HIGH;
            break;  
    }
    return 0;
}

//void scheduler_thread(void *pvParameter)
void *scheduler_thread(void *pvParameter)
{
    int rt, retry = 5, retry_send = 0, i;
    // char START_RECV_PACKET[DATA_SIZE] = {IOS_TYPE_GET_RESP, 0, IOS_CONTROL_OPEN};
    // char STOP_RECV_PACKET[DATA_SIZE] =  {IOS_TYPE_GET_RESP, 0, IOS_CONTROL_CLOSE};
    uint64_t start_time = get_time();
    int total_byte_transfer = 0;
    char send_data[DATA_SIZE] = {0};
    char recv_data[DATA_SIZE] = {0};
    int no_sending_data = 0;
    E_SCHEDULE_PRIORITY choosing_priority = SCHEDULER_PRIORITY_HIGH;

    int iTaskIdle = 0;
    int iTaskIdleSend = 0;
    int32_t recv_buf = 0;

    int iSendDummy = 0;

    char debug_video[256] = {0x00};
    int ret_read = 0;
    int length_rb = 0;
    
    uint64_t time_current = get_time();
    
    printf("Start the scheduler_thread\n");
    
    while(1){
        //vTaskDelay(10);
        if(get_time()-start_time>=1000){
            total_byte_transfer = 0;
            start_time = get_time();
        }
        retry = 200;
        retry_send = 10;
        no_sending_data = 0;

        if(scheduler_get_mode() == E_SCHEDULER_MODE_IP){
            printf("MODE IP!\n\r");
            if((get_time()-start_time)>=100){
                iSendDummy = 1; // Need to send dummy
                start_time = get_time();
            }
        }
        pthread_mutex_lock(&data_buff_mutex);
        length_rb = RingBuffer_available_data(send_handler[choosing_priority].transfer_send_buffer);
        ret_read = read_ringbuf(send_data, send_handler[choosing_priority].transfer_send_buffer, DATA_SIZE);
        if(ret_read > 0)
        {
            if(choosing_priority == SCHEDULER_PRIORITY_LOW)
            {
                memset(debug_video, 0x00, sizeof(debug_video));
                sprintf(debug_video, "Video:");
                for(i = 0; i < 13; i ++)
                {
                    sprintf(debug_video + strlen(debug_video), "%02X ", send_data[i]);
                }
                //printf("%s", debug_video);
            }
            
            if(f_send)
            {
                rt = f_send(send_data, DATA_SIZE);
                if(rt>0)
                {
                    total_byte_transfer += DATA_SIZE;
                    //printf("##");
                }
            }
            else
            {
                printf("f_send NULL!\r\n");
            }
            no_sending_data = 0;
            //printf("##");
        }
        else{
            if(no_sending_data++ >=2){
                retry = 0;
            }
            iTaskIdleSend++;
            iTaskIdle++;
        }
        //scheduler_update_priority(&choosing_priority);
        pthread_mutex_unlock(&data_buff_mutex);

        // RECEIVE
        if(f_recv){
            rt = f_recv(recv_data, DATA_SIZE);
            //printf("f_recv: rt %d\n", rt);
            //while(rt > 0){
            #if 0    
                if(rt < 0){ // error
                    printf("[SCHE] Error while recv data!\n\r");
                    iTaskIdle++;    
                }
                else if(rt == 0){ // need to send dump to get data from spi
                    if(iTaskIdleSend){
                        f_send(START_RECV_PACKET, DATA_SIZE);
                        iTaskIdleSend = 0;
                        iTaskIdle--;
                        printf("DUMP SPI\n");
                    }
                    
                }
            #else
                if(rt <= 0){
                    /*
                    if(iTaskIdleSend){
                        f_send(START_RECV_PACKET, DATA_SIZE);
                        iTaskIdleSend = 0;
                        iTaskIdle--;
                    }
                    */
                    if(iSendDummy){
                        // f_send(START_RECV_PACKET, DATA_SIZE);
                        iSendDummy = 0;
                    }
                }
            #endif
                else{ // need to process data
                    switch(recv_data[0]){
                        case SCH_TYPE_CMD_TCP:
                            //printf(">>>Response TCP\n\r");
                        case SCH_TYPE_CMD_UDP:
                        case SCH_TYPE_CMD_PING:
                            //printf("scheduler_thread: f_recv: %x\n", recv_data[0]);
                            recv_handler[SCHEDULER_DATA_TYPE_IP].transfer_recv_handler(recv_data, DATA_SIZE);                   
                            retry = 0;
                            break;
                        case SCH_TYPE_DATA_STREAMING: // for P2P-talkback
                            printf("T");
                            recv_handler[SCHEDULER_DATA_TYPE_P2P].transfer_recv_handler(recv_data, DATA_SIZE);  
                            break;
                        case SCH_TYPE_SPI_CMD_RESPONSE:
                            
                            memcpy(p2p_config_buf, recv_data, SPI_TRAN_SIZE);
                            recv_buf = (int32_t)p2p_config_buf;
                            // TODO: push to p2p setup queue
                            // if(xQueueSendToBack(queue_for_p2p_config, &recv_buf, NULL) != pdPASS){
                            //     printf("p2p config queue is full!!!\n");
                            // }
                            break;
                        default:
                            //retry = 0;
                            //printf("scheduler_thread: wrong packet %x\n", recv_data[0]);
                            break;
                    }
                }
            //    rt = f_recv(recv_data, DATA_SIZE);
            //}// while(rt > 0)   
        }
        scheduler_update_priority(&choosing_priority);
        if(iTaskIdle){
            //ak_sleep_ms(5);
            iTaskIdle = 0;
        }
        ak_sleep_ms(3);  
    }
}

void scheduler_start(void)
{
    scheduler_init(SCHEDULER_DATA_TYPE_P2P, scheduler_p2p_cb, send_spi_data, read_spi_data);
}


