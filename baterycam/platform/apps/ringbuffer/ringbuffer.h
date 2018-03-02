#ifndef _lcthw_RingBuffer_h
#define _lcthw_RingBuffer_h

//#include "bstrlib.h"

typedef struct {
    char *buffer;
    int length;
    int start;
    int end;
} RingBuffer;

RingBuffer *RingBuffer_create(int length);

void RingBuffer_destroy(RingBuffer *buffer);

int RingBuffer_read(RingBuffer *buffer, char *target, int amount);

int RingBuffer_write(RingBuffer *buffer, char *data, int length);

int RingBuffer_empty(RingBuffer *buffer);

int RingBuffer_full(RingBuffer *buffer);

int RingBuffer_available_data(RingBuffer *buffer);

int RingBuffer_available_space(RingBuffer *buffer);

int RingbBuffer_flush(RingBuffer *buffer);

// Trung: We don't use this function
//bstring RingBuffer_gets(RingBuffer *buffer, int amount);

//#define RingBuffer_available_data(B) (((B)->end + 1) % (B)->length - (B)->start - 1)
//#define RingBuffer_available_data(B) (((B)->end) % (B)->length - (B)->start)


//#define RingBuffer_available_space(B) ((B)->length - (B)->end - 1)

#define RingBuffer_full(B) ((RingBuffer_available_data((B)) - (B)->length) == 0)

#define RingBuffer_empty(B) (RingBuffer_available_data((B)) == 0)

//#define RingBuffer_puts(B, D) RingBuffer_write((B), bdata((D)), blength((D)))

//#define RingBuffer_get_all(B) RingBuffer_gets((B), RingBuffer_available_data((B)))

#define RingBuffer_starts_at(B) ((B)->buffer + (B)->start)

#define RingBuffer_ends_at(B) ((B)->buffer + (B)->end)

#define RingBuffer_commit_read(B, A) ((B)->start = ((B)->start + (A)) % (B)->length)

#define RingBuffer_commit_write(B, A) ((B)->end = ((B)->end + (A)) % (B)->length)

#endif
