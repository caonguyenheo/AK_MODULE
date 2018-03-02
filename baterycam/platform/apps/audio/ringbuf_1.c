/**
  ******************************************************************************
  * File Name          : ringbuf.c
  * Description        : This file contain functions for ringbuf
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/

#include "ringbuf_1.h"
#include <stdlib.h>

#include "ak_apps_config.h"


/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
// #define malloc	Fwl_Malloc
// #define free	Fwl_Free


/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/
/*******************************************************************************
* Function Name  : Ringbuf_Init
* Description    : Init a ringbufer and buffer with size paramater
* Input          : [uint32_t]size 
* Output         : None
* Return         : point to ringbufer
*******************************************************************************/
ring_buffer_t * Ringbuf_Init(uint32_t size)
{
    ring_buffer_t *rb = (ring_buffer_t *)malloc(sizeof(ring_buffer_t));
    if(rb == NULL || size <= 0)
    {
        return NULL;
    }
    else
    {
        // malloc buffer
        rb->data    = (uint8_t *)malloc(size);
        rb->cap     = size;
        rb->rdIdx   = 0;
        rb->wrIdx   = 0;
        rb->cnt     = 0;
    }
    return rb;
}

/*******************************************************************************
* Function Name  : Ringbuf_IsFull
* Description    : Check ringbuffer full
* Input          : ringbufer 
* Output         : None
* Return         : -1: Error
*                :  0: Not full
*                :  1: Ringbuffer is FULL
*******************************************************************************/
int8_t Ringbuf_IsFull(ring_buffer_t *rb)
{
    int8_t ret = 0;
    if(rb == NULL)  return -1;
    ret = (rb->cnt == rb->cap)?1:0;
    return ret;
}
/*******************************************************************************
* Function Name  : Ringbuf_IsEmpty
* Description    : Check ringbuffer empty
* Input          : ringbufer 
* Output         : None
* Return         : -1: Error
*                :  0: Not empty
*                :  1: Ringbuffer is Empty
*******************************************************************************/
int8_t Ringbuf_IsEmpty(ring_buffer_t *rb)
{
    int8_t ret = 0;
    if(rb == NULL)  return -1;
    ret = (rb->cnt == 0)?1:0;
    return ret;
}
/*******************************************************************************
* Function Name  : Ringbuf_GetAvailSpace
* Description    : Get available space in ringbuffer
* Input          : ringbufer (I don't check argument)
* Output         : None
* Return         : size of space
*******************************************************************************/
uint32_t Ringbuf_GetAvailSpace(ring_buffer_t *rb)
{
    return (rb->cap - rb->cnt);
}
/*******************************************************************************
* Function Name  : Ringbuf_GetAvailData
* Description    : Get available data in ringbuffer
* Input          : ringbufer (I don't check argument)
* Output         : None
* Return         : size of data
*******************************************************************************/
uint32_t Ringbuf_GetAvailData(ring_buffer_t *rb)
{
    return (rb->cnt);
}
/*******************************************************************************
* Function Name  : Ringbuf_WriteByte
* Description    : Put a byte data into ringbuffer
* Input          : ringbufer
* Input          : byte data
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_WriteByte(ring_buffer_t *rb, uint8_t bData)
{
    int8_t ret = 0;
    if((rb == NULL))
    {
        // argument null
        ret = -1;
    }
    else
    {
        if(rb->cnt == rb->cap)
        {
            //ringbufer is full
            ret = -1;
        }
        else
        {
            rb->data[rb->wrIdx] = bData;
            rb->wrIdx = (rb->wrIdx + 1) % rb->cap;
            rb->cnt++;
        }
    }
    return ret;
}
/*******************************************************************************
* Function Name  : Ringbuf_ReadByte
* Description    : Pop a byte data out of ringbuffer
* Input          : ringbufer
* Input          : point to byte data
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_ReadByte(ring_buffer_t *rb, uint8_t *bData)
{
    int8_t ret = 0;
    if((rb == NULL) || (bData == NULL))
    {
        // argument null or pointer data null
        ret = -1;
    }
    else
    {
        if(rb->cnt == 0)
        {
            ret = -1; // ringbufer is empty
        }
        else
        {
            *bData = rb->data[rb->rdIdx];
            rb->rdIdx = (rb->rdIdx + 1) % rb->cap;
            rb->cnt--;
        }
    }
    return ret;
}

/*******************************************************************************
* Function Name  : Ringbuf_WriteBuf
* Description    : Put a buffer data into ringbuffer
* Input          : ringbufer
* Input          : point to buffer data
* Input          : length of buffer
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_WriteBuf(ring_buffer_t *rb, uint8_t *pBuf, uint32_t len)
{
    int8_t ret = 0;
    uint32_t i = 0;
    if((rb == NULL) || (pBuf == NULL) || (len == 0))
    {
        // argument null
        ret = -1;
    }
    else
    {
        if((rb->cap - rb->cnt) < len)
        {
            // no space to put data
            ret = -1;
        }
        else
        {
            for(i = 0; i < len; i++)
            {
                rb->data[rb->wrIdx] = *(pBuf + i); 
                rb->wrIdx = (rb->wrIdx + 1) % rb->cap;
                rb->cnt++;
            }
        }
    }
    return ret;
}
/*******************************************************************************
* Function Name  : Ringbuf_ReadBuf
* Description    : Pop a buffer data out of ringbuffer
* Input          : ringbufer
* Input          : point to buffer data
* Input          : length of buffer
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_ReadBuf(ring_buffer_t *rb, uint8_t *pBuf, uint32_t len)
{
    int8_t ret = 0;
    uint32_t i = 0;
    if((rb == NULL) || (pBuf == NULL) || (len == 0))
    {
        // argument null
        ret = -1;
    }
    else
    {
        // check available data can be read
        if(rb->cnt < len)
        {
            // len data need to read which greater than available data
            ret = -1;
        }
        else
        {
            for(i = 0; i < len; i++)
            {
                *(pBuf + i) = rb->data[rb->rdIdx]; 
                rb->rdIdx = (rb->rdIdx + 1) % rb->cap;
                rb->cnt--;
            }
        }
    }
    return ret;
}

/*******************************************************************************
* Function Name  : Ringbuf_Free
* Description    : Free a ringbuffer
* Input          : ringbufer
* Output         : None
* Return         : None
*******************************************************************************/
void Ringbuf_Free(ring_buffer_t *rb)
{
    if(rb)
    {
        free(rb->data);
        free(rb);
    }
}

/*
***********************************************************************
*                           END OF FILES                              *
***********************************************************************
*/
