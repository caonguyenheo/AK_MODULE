#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include "buffer.h"
#include "dbg.h"

Buffer *
buffer_alloc(int initial_size)
{
    Buffer *buf = malloc(sizeof(Buffer));
    char   *tmp = calloc(1, initial_size * sizeof(char));

    jump_to_error_if(buf == NULL || tmp == NULL);

    buf->contents   = tmp;
    buf->bytes_used = 0;
    buf->total_size = initial_size;

    return buf;
error:
    if (buf) { buffer_free(buf); }
    if (tmp) { free(tmp); }

    return NULL;
}

int
buffer_strlen(Buffer *buf)
{
    return buf->bytes_used;
}

void
buffer_free(Buffer *buf)
{
    free(buf->contents);
    free(buf);
}

int
buffer_has_space(Buffer *buf, int desired_length)
{
    int bytes_remaining = buf->total_size - buf->bytes_used;

    debug("Requesting %d bytes, %d available", desired_length, bytes_remaining);

    return desired_length <= bytes_remaining;
}

int
buffer_grow(Buffer *buf, int minimum_size)
{
    int factor = buf->total_size;

    if (factor < minimum_size) {
        factor = minimum_size;
    }

    int new_size = factor * 2;

    debug("Growing buffer from %d to %d bytes", buf->total_size, new_size);

    char *tmp = realloc(buf->contents, new_size * sizeof(char));
    jump_to_error_if(tmp == NULL);

    buf->contents   = tmp;
    buf->total_size = new_size;

    return 0;
error:
    return -1;
}

void
buffer_cat(Buffer *buf, char *append, int length)
{
    int i               = 0;
    int bytes_copied    = 0;
    int buffer_position = 0;

    for (i = 0; i < length; i++) {
        if (append[i] == '\0') { break; }

        buffer_position = buf->bytes_used + i;
        *(buf->contents + buffer_position) = append[i];

        bytes_copied++;
    }

    buf->bytes_used += bytes_copied;
    *(buf->contents + buf->bytes_used) = '\0';
}

int
buffer_append(Buffer *buf, char *append, int length)
{
    int status         = 0;
    int desired_length = length + 1; // Space for NUL byte

    // printf("DEBUG: %s entered %d!\n", __func__, __LINE__);
    if (!buffer_has_space(buf, desired_length)) {
        printf("INFO: buffer_has_space false, desired_length: %d\n", desired_length);
        status = buffer_grow(buf, desired_length);
        jump_to_error_unless(status == 0)
    }

    buffer_cat(buf, append, length);
    // printf("DEBUG: %s exited %d!\n", __func__, __LINE__);
    return 0;
error:
    // printf("DEBUG: %s entered ERROR %d!\n", __func__, __LINE__);
    return -1;
}

int
buffer_appendf(Buffer *buf, const char *format, ...)
{
    char *tmp = NULL;
    int bytes_written, status;

    va_list argp;

    int len_tmp = sizeof(char) * 1024;
    tmp = (char *)malloc(len_tmp);
    if(tmp == NULL)
    {
        printf("%s cannnot malloc memory for tmp!\r\n", __func__);
        return -1;
    }

    va_start(argp, format);
/*
    printf("DEBUG: %s entered %d, tmp %p, &tmp %p!\n", __func__, __LINE__, tmp, &tmp);
    bytes_written = vasprintf(&tmp, format, argp);
    printf("DEBUG: %s line %d, tmp %p, &tmp %p!\n", __func__, __LINE__, tmp, &tmp);
    // ak_sleep_ms(1000);
    jump_to_error_if(bytes_written < 0);
*/
    // printf("bytes_written: %d tmp: %s\r\n", bytes_written, tmp);
    bytes_written = vsnprintf(tmp, len_tmp, format, argp);
    // printf("bytes_written: %d tmp: %s\r\n", bytes_written, tmp);
    jump_to_error_if(bytes_written < 0);
    // while(1)
    // {
    //     ak_sleep_ms(100);
    //     printf("i");
    // }
    // perror(tmp);

    va_end(argp);

    status = buffer_append(buf, tmp, bytes_written);
    // printf("INFO: buffer_append->status: %d\n", status);
    jump_to_error_unless(status == 0);

    free(tmp);
    // printf("DEBUG: %s exited!\n", __func__);
    return 0;
error:
    if (tmp != NULL) { free(tmp); }
    printf("DEBUG: %s entered ERROR!\n", __func__);
    return -1;
}

int
buffer_nappendf(Buffer *buf, size_t length, const char *format, ...)
{
    int status        = 0,
        printf_length = length + 1;

    char *tmp  = calloc(1, printf_length * sizeof(char));

    jump_to_error_if(tmp == NULL);

    va_list argp;
    va_start(argp, format);

    status = vsnprintf(tmp, printf_length, format, argp);
    jump_to_error_if(status < 0);

    va_end(argp);

    status = buffer_append(buf, tmp, length);
    jump_to_error_unless(status == 0);

    free(tmp);

    return 0;
error:
    if (tmp != NULL) { free(tmp); }
    return -1;

}

char *
buffer_to_s(Buffer *buf)
{
    char *result = calloc(1, buf->bytes_used + 1);
    strncpy(result, buf->contents, buffer_strlen(buf));

    return result;
}