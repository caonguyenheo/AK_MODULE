
//#undef NDEBUG_RB
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "dbg_rb.h"
#include "ringbuffer.h"
#include "ak_apps_config.h"


static void Ringbuffer_debug(RingBuffer *buffer, char *str)
{
    #if 0
    if(buffer && str){
        log_info("[%s]RB: 0x%p l:%d s:%d e:%d\n", str, buffer, 
            buffer->length, buffer->start, buffer->end);
    }
    #endif
}

int RingBuffer_available_data(RingBuffer *buffer)
{
    Ringbuffer_debug(buffer, "AVAIL");
    return ((buffer->end % buffer->length) - buffer->start);
}

int RingBuffer_available_space(RingBuffer *buffer)
{
    return (buffer->length - buffer->end - 1);
}

RingBuffer *RingBuffer_create(int length)
{
    // RingBuffer *buffer = calloc(1, sizeof(RingBuffer));
    RingBuffer *buffer = (RingBuffer *)malloc(sizeof(RingBuffer));
    check(buffer != NULL, "Failed to create buffer.")
    buffer->length  = length + 1;
    buffer->start = 0;
    buffer->end = 0;
    // buffer->buffer = calloc(buffer->length, 1);
    buffer->buffer = (char *)malloc(buffer->length);

    // printf("--->>>length: %d\n", buffer->length);
    return buffer;
error:
    return NULL;
}

void RingBuffer_destroy(RingBuffer *buffer)
{
    if(buffer) {
        free(buffer->buffer);
        free(buffer);
    }
}

int RingBuffer_write(RingBuffer *buffer, char *data, int length)
{
    int space = 0;
    int avail_data = 0;
    
    if(RingBuffer_available_data(buffer) == 0) {
        Ringbuffer_debug(buffer, "RESET");
        buffer->start = buffer->end = 0;
    }
    //log_info("length: %d, space:%d\r\n", length, RingBuffer_available_space(buffer));
    #if 0
    check(length <= RingBuffer_available_space(buffer),
            "Not enough space: %d request, %d available",
            RingBuffer_available_data(buffer), length);
    #else
    // Trung debug negative number of ringbuffer
    //printf("\nWRITE(0x%x): len:%d, start:%d, end:%d\n", buffer, buffer->length, buffer->start, buffer->end);
    space = (int)RingBuffer_available_space(buffer);
    if(space < 0){
        printf("\nWRITE(0x%x): len:%d, start:%d, end:%d, space: %d\n", buffer, buffer->length, buffer->start, buffer->end, space);
        return -1;
    }
    else{
        if(space < length){
            avail_data = (int)RingBuffer_available_data(buffer);
            // log_err("Not enough space (0x%x), length: %d, space:%d\r\n", buffer, length, space);
            // printf("%s NO SPACE: length: %d, space:%d\r\n", __func__, length, space);
            printf("@");
            goto error;
        }
    }
    #endif
    
    void *result = memcpy(RingBuffer_ends_at(buffer), data, length);
    check(result != NULL, "Failed to write data into buffer.");

    RingBuffer_commit_write(buffer, length);

    return length;
error:
    return -1;
}

int RingBuffer_read(RingBuffer *buffer, char *target, int amount)
{
    int avail_data = 0;
    int adt_len = 0, aspace_len = 0;
    #if 0
    check_debug(amount <= RingBuffer_available_data(buffer),
            "Not enough in the buffer: has %d, needs %d",
            RingBuffer_available_data(buffer), amount);
    #else
    //printf("\nREAD(0x%x): len:%d, start:%d, end:%d\n", buffer, buffer->length, buffer->start, buffer->end);
    avail_data = RingBuffer_available_data(buffer);
    if(!avail_data){
        return 0;
    }
    else if(avail_data < 0){
        aspace_len = RingBuffer_available_space(buffer);
        adt_len = buffer->length - buffer->end - 1;
        printf("\nREAD(0x%x): len:%d, start:%d, end:%d, avail_data:%d\n", buffer, buffer->length, buffer->start, buffer->end, avail_data);
        return -1;
    }
    else{
        if(avail_data < amount){
            log_info("amount: %d, data:%d\r\n", amount, avail_data);
            goto error;
        }
    }
    #endif
    
    void *result = memcpy(target, RingBuffer_starts_at(buffer), amount);
    check(result != NULL, "Failed to write buffer into data.");

    RingBuffer_commit_read(buffer, amount);

    if(buffer->end == buffer->start) {
        buffer->start = buffer->end = 0;
    }

    return amount;
error:
    return -1;
}


int RingbBuffer_flush(RingBuffer *buffer)
{
    buffer->start = buffer->end = 0;
    return 0;
}
// Trung: we don't use this function.
#if 0
bstring RingBuffer_gets(RingBuffer *buffer, int amount)
{
    check(amount > 0, "Need more than 0 for gets, you gave: %d ", amount);
    check_debug(amount <= RingBuffer_available_data(buffer),
            "Not enough in the buffer.");

    bstring result = blk2bstr(RingBuffer_starts_at(buffer), amount);
    check(result != NULL, "Failed to create gets result.");
    check(blength(result) == amount, "Wrong result length.");

    RingBuffer_commit_read(buffer, amount);
    assert(RingBuffer_available_data(buffer) >= 0 && "Error in read commit.");

    return result;
error:
    return NULL;
}
#endif
