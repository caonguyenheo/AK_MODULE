#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packet_queue.h"
#include "pqueue.h"
#include "ak_apps_config.h"
#include "spi_transfer.h"


int g_i32CountAllocatePacket = 0;

// enum SPI_PACKET_TYPE {
//     CMD_VERSION     = 0x01,
//     CMD_UDID        = 0x02,
//     DATA_STREAMING  = 0x03,
//     CMD_STATUS      = 0x04,
//     CMD_GET_PACKET  = 0x05,
//     CMD_RESET       = 0x06,
//     CMD_TCP         = 0x07,
//     CMD_UDP         = 0x08,
//     CMD_PING        = 0x09,
//     CMD_GET_RESP    = 0x0A,
//     CMD_REQEST      = 0x0B,
//     CMD_RESPONSE    = 0x0C,
//     MAX_PACKET_TYPE
// };

#define GET_BYTE(x)         (*((uint8_t  *)(x)++))
#define GET_2_BYTES(x)      (*((uint16_t *)(x)++))
#define GET_4_BYTES(x)      (*((uint32_t *)(x)++))

typedef struct node_t
{
	pqueue_pri_t pri;
	int    val;
    packet_header_t *packet;
	size_t pos;
} node_t;

static int
cmp_pri(pqueue_pri_t next, pqueue_pri_t curr)
{
	// return (next < curr);
	return (curr < next);
}



static pqueue_pri_t
get_pri(void *a)
{
	return ((node_t *) a)->pri;
}


static void
set_pri(void *a, pqueue_pri_t pri)
{
	((node_t *) a)->pri = pri;
}


static size_t
get_pos(void *a)
{
	return ((node_t *) a)->pos;
}


static void
set_pos(void *a, size_t pos)
{
	((node_t *) a)->pos = pos;
}

int
packet_queue_test(void)
{
	pqueue_t *pq;
	node_t   *ns;
	node_t   *n;

	ns = malloc(10 * sizeof(node_t));
	pq = pqueue_init(10, cmp_pri, get_pri, set_pri, get_pos, set_pos);
	if (!(ns && pq)) return 1;

	ns[0].pri = 5; ns[0].val = -5; pqueue_insert(pq, &ns[0]);
	ns[1].pri = 4; ns[1].val = -4; pqueue_insert(pq, &ns[1]);
	ns[2].pri = 2; ns[2].val = -2; pqueue_insert(pq, &ns[2]);
	ns[3].pri = 6; ns[3].val = -6; pqueue_insert(pq, &ns[3]);
	ns[4].pri = 1; ns[4].val = -1; pqueue_insert(pq, &ns[4]);

	n = pqueue_peek(pq);
	printf("peek: %lld [%d]\n", n->pri, n->val);

	pqueue_change_priority(pq, 8, &ns[4]);
	pqueue_change_priority(pq, 7, &ns[2]);

   	while ((n = pqueue_pop(pq)))
		printf("pop: %lld [%d]\n", n->pri, n->val);

	pqueue_free(pq);
	free(ns);

	return 0;
}

uint8_t expected_array[] = {0x80,0x80,0x4C,0x8B,0x88,0x17,0xF6,0x4C,0x19,0x26,0x1F,0xD3,0xF3,0xAF,0x5C,0xBC};
int check_packet(uint8_t *data, uint32_t len) {
    uint32_t i = 0;
    uint8_t *ptr_data = data;
    uint32_t PID = 0;
    uint32_t block_PID = 0;
    uint32_t real_idx = 0;
    uint32_t sz_expected_array = sizeof(expected_array);
    if ( ptr_data == NULL) {
        return -1;
    }
    
    PID = (ptr_data[19] & 0xFF) << 24;
    PID |= (ptr_data[20] & 0xFF) << 16;
    PID |= (ptr_data[21] & 0xFF) <<  8;
    PID |= (ptr_data[22] & 0xFF) <<  0;

    block_PID = (ptr_data[15] & 0xFF) << 24;
    block_PID |= (ptr_data[16] & 0xFF) << 16;
    block_PID |= (ptr_data[17] & 0xFF) <<  8;
    block_PID |= (ptr_data[18] & 0xFF) <<  0;

    printf("Block_PID=%u, PID=%u\r\n", block_PID, PID);
    real_idx = (PID - block_PID)*999;

    ptr_data = data + 23; // 3 spi header + 20 p2p header
    for (i = 0; i < len - 23; i++) {
        if (ptr_data[i] != expected_array[real_idx%sz_expected_array]) {
            printf("idx=%u, real_idx=%u, DATA=%x, EXPECTED=%x\r\n", i, real_idx, ptr_data[i], expected_array[real_idx%sz_expected_array]);
            return -2;
        }
        real_idx++;
    }
    return 0;
}
void print_buffer(uint8_t *data, int32_t len) {
    int i = 0;
    for (i = 0; i < len; i++) {
        printf("0X%02x ", data[i]);
    }
}
#define MAX_PQ_SIZE     1024

static pqueue_t *pq_packet = NULL;
void packet_process_thread(void *arg) {
    // pqueue_t *pq_packet = NULL;
    packet_header_t *ptr_header = NULL;
    int32_t ret_val = -1;

    // RINGBUF
    int len;
    uint8_t local_rx_buf[MAX_SPI_PACKET_LEN] = {0};

    printf("Start packet_process_thread\r\n");

    pq_packet = pqueue_init(MAX_PQ_SIZE, cmp_pri, get_pri, set_pri, get_pos, set_pos);
    if (pq_packet == NULL) {
        return;
    }
    int index = 500;
    // WHILE LOOP
    while (1) 
    {
    #ifdef CFG_USE_SCHEDULER
        if(scheduler_p2p_avail_data() > 0)
        {
            memset(local_rx_buf, 0x00, sizeof(local_rx_buf));
            len = scheduler_p2p_recv_data(local_rx_buf, MAX_SPI_PACKET_LEN);
            if (len <= 0) {
                // FIXME: find better solution
                ak_sleep_ms(10);
                continue;
            }
            ret_val = create_packet(&ptr_header, local_rx_buf, len);

            if (ret_val < 0 || ptr_header == NULL) {
                continue;
            }

            ret_val = push_packet(pq_packet, ptr_header);

            if (ret_val < 0) {
                // FIXME: delete packet
                printf("Error: cannot push packet, error=%d\r\n", ret_val);
                delete_packet(ptr_header);
                continue;
            }
        }
    #else
        len = read_spi_data((char *)local_rx_buf, SPI_TRAN_SIZE);
        if(len > 0)
        {
            printf(".");
            ret_val = create_packet(&ptr_header, local_rx_buf, len);

            if (ret_val < 0 || ptr_header == NULL) {
                continue;
            }

            ret_val = push_packet(pq_packet, ptr_header);

            if (ret_val < 0) {
                // FIXME: delete packet
                printf("Error: cannot push packet, error=%d\r\n", ret_val);
                delete_packet(ptr_header);
                continue;
            }
        }
    #endif
        ak_sleep_ms(20);
    }
    // END
    pqueue_free(pq_packet);
    ak_thread_join(ak_thread_get_tid());
    ak_thread_exit();
}

// uinit test
void main_pop_frame(void *arg) {
    int32_t ret_val = -1;
    uint32_t len = 0;
    uint8_t data_buf[1024*5] = {0};

    printf("main_pop_frame start\r\n");
    for (;;) {
        if (pq_packet != NULL) {
            break;
        }
        ak_sleep_ms(1000);
    }

    // TEST start
    // ak_sleep_ms(5000);
    // packet_header_t *ptr_header = NULL;
    // while (1) {
    //     printf("Start-----\r\n");
    //     ret_val = pop_packet(pq_packet, &ptr_header);
    //     if (ret_val == 0) {
    //         printf("Create pid    alskdjfk   PID=%d\r\n", ptr_header->PID);
    //         delete_packet(ptr_header);
    //     }
    //     printf("------Done\r\n");
    //     // ak_sleep_ms(100);
    // }
    // TEST end

    #if 0
    TickType_t lstTick = 0, curTick = 0;
    lstTick = xTaskGetTickCount();
    #endif
    // uint32_t cntData = 0;

    for (;;) {
        #if 0
        curTick = xTaskGetTickCount();
        if((curTick - lstTick) >= (configTICK_RATE_HZ))
        {
            lstTick = curTick;
            // DkS
            // printf("\n\rTalkback: %dbytes/s", cntData);
            cntData = 0;
        }
        #endif

        len = 0;
        memset(data_buf, 0x00, sizeof(data_buf));
        ret_val = get_audio_frame(data_buf, sizeof(data_buf), &len);
        if (ret_val == 0) {
            // cntData += len;
        }
        // printf("pop frame ret_val=%d, len=%d\r\n", ret_val, len);
        ak_sleep_ms(10);
    }
    ak_thread_join(ak_thread_get_tid());
    ak_thread_exit();
}

// entry to get frame
int32_t get_audio_frame(uint8_t *frame, uint32_t buffer_len, uint32_t *frame_len) {
    int32_t ret_val = 0;
    media_frame_t *ptr_media_frame = NULL;

    if (pq_packet == NULL) {
        //printf("why here\r\n");
        return -1;
    }

    if (frame == NULL) {
        return -2;
    }

    ptr_media_frame = malloc(sizeof(media_frame_t));
    if (ptr_media_frame == NULL) {
        return -3;
    }
    // printf("get_audio_frame 0\r\n");

    // printf("get_audio_frame pop frame\r\n");
    ret_val = pop_block_frame(pq_packet, ptr_media_frame);
    if (ret_val < 0) {
        //printf("why here 1\r\n");
        goto exit;
    }

    if (buffer_len < ptr_media_frame->frame_size) {
        ret_val = -4;
        goto exit_error;
    }

    // printf("get_audio_frame pop frame 1\r\n");
    *frame_len = ptr_media_frame->frame_size;
    memcpy(frame, ptr_media_frame->frame_data, ptr_media_frame->frame_size);
    // DkS
    // printf("fPID=%d ", ptr_media_frame->PID);
    
exit_error:
    free(ptr_media_frame->frame_data);
exit:
    free(ptr_media_frame);
    return ret_val;
}

typedef struct pid_state {
    uint16_t number_packet;
    uint16_t packet_counter;
    uint32_t block_PID;
    uint32_t PID;
} pid_state_t;

#define INIT_PID_STATE  {0, 0, 0, 0}
#define RTT             100 // ms
#define CHECK_RANGE     10
// Get a media frame
int32_t pop_block_frame(pqueue_t *pq, media_frame_t *frame) {
    static uint32_t min_PID = -1;
    static uint32_t max_PID = -1;
    static uint32_t pre_PID = -1;
    pid_state_t need_packet = INIT_PID_STATE;
    int32_t ret_val = 0;

    uint32_t frame_size = 0;
    uint8_t *frame_data = NULL;
    uint32_t time_stamp = 0;
    eMediaSubType frame_type = 0;

    packet_header_t *ptr_packet = NULL;

    int retry = 10;
    // printf("POP 0\r\n");

    if (pq == NULL || frame == NULL) {
        return -1;
    }

    // Get first frame
    // initialize
    while (1) {
        // printf("POP 1\r\n");
        ptr_packet = NULL;
        //printf("%s: ptr_packet: %p\n\r", __func__, ptr_packet);
        ret_val = pop_packet(pq, &ptr_packet);
        if (ret_val < 0 || ptr_packet == NULL) {
            return -2;
        }
        // Get the start frame
        #if (0)
        if (ptr_packet->block_PID == ptr_packet->PID) {
            break;
        }
        #else
        if (ptr_packet->block_PID == ptr_packet->PID) {
            if (ptr_packet->PID > max_PID || ptr_packet->PID < min_PID) {
                max_PID = ptr_packet->block_PID;
                if (max_PID < CHECK_RANGE) {
                    min_PID = max_PID;
                }
                else {
                    min_PID = max_PID - CHECK_RANGE;
                }
                printf("INFO: packet ID = %u, len = %u, gpkt:%d\r\n", ptr_packet->PID, ptr_packet->block_length, g_i32CountAllocatePacket);

                break;
            }
        }
        #endif
        // Drop frame if this is middle frame
        // printf("WARN: Leakage packet block_PID=%d, PID=%d, block_len=%d\r\n", ptr_packet->block_PID, ptr_packet->PID, ptr_packet->block_length);

        delete_packet(ptr_packet);
        ak_sleep_ms(10);
    }

    // Copy packet data to frame
    need_packet.block_PID = ptr_packet->block_PID;
    need_packet.PID = ptr_packet->PID;
    need_packet.number_packet = ptr_packet->block_length; // unit: packet
    need_packet.packet_counter = 1;
    frame_size = ptr_packet->packet_size;
    frame_data = malloc(1024*need_packet.number_packet);
    memcpy(frame_data, ptr_packet->packet_data, ptr_packet->packet_size);
    time_stamp = ptr_packet->time_stamp;
    frame_type = ptr_packet->packet_sub_type;
    delete_packet(ptr_packet);

    // DkS
    // printf("\n\r[QUE]BLK[%d]n=%d s[%d]=%d ", ptr_packet->block_PID, need_packet.number_packet, need_packet.PID, frame_size);
#if 1
    // Next packet
    need_packet.PID++;
    while (need_packet.packet_counter < need_packet.number_packet) {
        // Peek to get packet
        ptr_packet = NULL;
        ret_val = peek_packet(pq, &ptr_packet);
        if (ret_val < 0 || ptr_packet == NULL) {
            // FIXME: should waiting, if reading speed > writing speed
            if (retry-- > 0) {
                goto wait_here;
            }
            printf("ERROR: can't find next packet ret_val = %d\r\n", ret_val);
            goto exit_error;
        }

        if (ptr_packet->PID > need_packet.PID) {
            // FIXME: wait just a little
            // continue;
            if (retry -- > 0) {
                goto wait_here;
            }
            printf("ERROR: Lost packet\r\n");
            ret_val = -4;
            goto exit_error;
        }
        
        // FIXME: is there any case peek_packet and pop_packet aren't the same
        // check validity
        ret_val = pop_packet(pq, &ptr_packet);
        if (ret_val < 0) {
            goto exit_error;
        }

        if (ptr_packet->PID < need_packet.PID) {
            delete_packet(ptr_packet);
            continue;
        }

        // copy data
        memcpy(frame_data + frame_size, ptr_packet->packet_data, ptr_packet->packet_size);
        frame_size += ptr_packet->packet_size;
        need_packet.packet_counter++;

        // DkS
        // printf("s[%d]=%d ",ptr_packet->PID, ptr_packet->packet_size);
         
        delete_packet(ptr_packet);
       
        // calculate next pid
        need_packet.PID++;
wait_here: 
        ak_sleep_ms(10);
    }
#endif    
    frame->PID = need_packet.block_PID;
    frame->frame_size = frame_size;
    frame->time_stamp = time_stamp;
    frame->packet_sub_type = frame_type;
    frame->frame_data = frame_data;
    // DkS
    // printf(" > size:%d\n\r", (int)frame_size);
    return ret_val;
exit_error:
    free(frame_data);
    return ret_val;
}

// Peek first element
int32_t peek_packet(pqueue_t *pq, packet_header_t **packet) {
    node_t *p_node = NULL;
    
    if (pq == NULL) {
        return -1;
    }

    p_node = pqueue_peek(pq);
    if (p_node == NULL) {
        return -2;
    }

    *packet = p_node->packet;
    return 0;
}


// Get a packet from queue
int32_t pop_packet(pqueue_t *pq, packet_header_t **packet) {
    node_t *p_node = NULL;

    //printf("%s: packet: %p\n\r", __func__, packet);
    
    if (pq == NULL) {
        return -1;
    }

    p_node = pqueue_pop(pq);
    if (p_node == NULL) {
        return -2;
    }

    *packet = p_node->packet;
    free(p_node);
    g_i32CountAllocatePacket--;
    // printf("pop_packet PID=%d, p=%p\r\n",(*packet)->PID, (*packet));
    return 0;
}

// Push a packet into queue
// Create a node wrapper, pri is PID of packet
int32_t push_packet(pqueue_t *pq, packet_header_t *packet) {
    node_t *p_node = NULL;
    int ret_val = 0;
    if (pq == NULL || packet == NULL) {
        return -1;
    }

    p_node = malloc(sizeof(node_t));
    if (p_node == NULL) {
        return -2;
    }

    p_node->pri = packet->PID;
    p_node->packet = packet;
    // printf("PID=%d____pri=%d\r\n", packet->PID, p_node->pri);
    ret_val = pqueue_insert(pq, p_node);
    if (ret_val != 0) {
        free(p_node);
        ret_val = -1;
    }
    g_i32CountAllocatePacket++;
    return ret_val;
}

// generate a packet from raw data
// data format
/*
Command (1 bytes, 1024 padding)            Response(1024 bytes padding)   Description
"0x01"                                     "0x81<version>"                Get version
"0x02"                                     "0x82<udid>"                   Get UDID
"0x03<Len2><data>"                                                        Send a media packet, example 0x03+1019<0..1018>
"0x04"                                     "0x84<0:factory reset>"        Get status
                                                <1:not-ready>
                                                <2:ready>
                                                <3:streaming>
"0x05"                                     "0x85<len2><data>              Get a media packet, example 0x05+1019<0..1018>
"0x06 0xF7 0xF8 0xF9"                                                     IOT reset
"0x07 --- 0x0A"															  IP over SPI
"0x0B<packet_seq><packet_index><end_flag><command_seq><command_id><command_body>"		 Send request (master -> slave)
"0x0C<packet_seq><packet_index><end_flag><command_seq><command_id><command_body>"		 Response (slave -> master)
*/

#define SPI_HEADER_LEN          3
static uint32_t g_pid_previous = 0;

int32_t create_packet(packet_header_t **ptr_packet, uint8_t *data, int32_t data_len) {
    packet_header_t *packet = NULL;
    uint8_t packet_type = 0;
    uint8_t *payload = NULL;
    uint16_t payload_len = 0;
    uint16_t spi_payload_len = 0;
    uint8_t *p_data = data;
    int32_t ret = 0;
    
    if (p_data == NULL) {
        return -1;
    }
    if (data_len > 1024) {
        return -2;
    }

    // We are allocate 1 memory zone in here, but free in other place.
    // TODO: Need source code manage memory to avoid stack over flow
    packet = malloc(sizeof(packet_header_t));
    if (packet == NULL) {
        return -3;
    } 

    packet_type = *(p_data++);
    switch (packet_type) {
        case DATA_STREAMING:
            spi_payload_len          = (GET_BYTE(p_data) & 0xFF) <<  8;
            spi_payload_len         |= (GET_BYTE(p_data) & 0xFF) <<  0;
            // GET_4_BYTES(p_data);
            // FIXME: check "VLVL"
            GET_BYTE(p_data);
            GET_BYTE(p_data);
            GET_BYTE(p_data);
            GET_BYTE(p_data);
            GET_BYTE(p_data);
            // Parse data field
            packet->packet_sub_type  = GET_BYTE(p_data);
            packet->time_stamp       = (GET_BYTE(p_data) & 0xFF) << 24;
            packet->time_stamp      |= (GET_BYTE(p_data) & 0xFF) << 16;
            packet->time_stamp      |= (GET_BYTE(p_data) & 0xFF) <<  8;
            packet->time_stamp      |= (GET_BYTE(p_data) & 0xFF) <<  0;
            packet->block_length     = (GET_BYTE(p_data) & 0xFF) << 8;
            packet->block_length    |= (GET_BYTE(p_data) & 0xFF) << 0;
            packet->block_PID        =(GET_BYTE(p_data) & 0xFF) << 24;
            packet->block_PID       |=(GET_BYTE(p_data) & 0xFF) << 16;
            packet->block_PID       |=(GET_BYTE(p_data) & 0xFF) <<  8;
            packet->block_PID       |=(GET_BYTE(p_data) & 0xFF) <<  0;
            packet->PID              =(GET_BYTE(p_data) & 0xFF) << 24;
            packet->PID             |=(GET_BYTE(p_data) & 0xFF) << 16;
            packet->PID             |=(GET_BYTE(p_data) & 0xFF) <<  8;
            packet->PID             |=(GET_BYTE(p_data) & 0xFF) <<  0;

            payload_len = spi_payload_len - 20;
            packet->packet_size     = payload_len;
            // printf("payload_len=%d, SPI_PAYLOAY_LEN=%d, packet_type=%x, time=%x, block_len=%x, block_pid=%x, pid=%x, "
            //                         , payload_len
            //                         , spi_payload_len
            //                         , packet->packet_sub_type
            //                         , packet->time_stamp
            //                         , packet->block_length
            //                         , packet->block_PID
            //                         , packet->PID
            //                         );
            if(g_pid_previous == 0)
            {
                g_pid_previous = packet->PID;
            }
            else
            {
                if((g_pid_previous + 1) != packet->PID)
                {
                    ak_print_error_ex("--->PID is NG (lack of %u) recv %u\r\n", g_pid_previous + 1, packet->PID);
                }
                g_pid_previous = packet->PID;
            }
            // printf("pid: %u\r\n", packet->PID);

            payload = malloc(payload_len);
            memcpy(payload, p_data, payload_len);
            packet->packet_data     = payload;
            
        break;

        default:
            free(packet);
            return -1;
    }
    if (packet != NULL) {
        *ptr_packet = packet;
    }
    return 0;
}

int32_t delete_packet(packet_header_t *packet) {
    // printf("DEBUG: delete_packet p=%p\r\n", packet);
    if (packet == NULL) {
        return -1;
    }

    if (packet->packet_data != NULL) { 
        free(packet->packet_data);
    }

    free(packet);
    // printf("DEBUG: delete_packet end\r\n");

    return 0;
}

