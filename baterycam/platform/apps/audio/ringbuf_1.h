/**
  ******************************************************************************
  * File Name          : ringbuf.h
  * Description        : This file contains all config hardware of board 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RINGBUF_H__
#define __RINGBUF_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 
#include "ak_apps_config.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

#if 0
typedef char            int8_t;
typedef unsigned char   uint8_t;
typedef short           int16_t;
typedef unsigned short  uint16_t;
typedef int             int32_t;
typedef unsigned int    uint32_t;
typedef long            int64_t;
typedef unsigned long   uint64_t;
#endif
//typedef long long       int64_t;
//typedef unsigned long long   uint64_t;


typedef struct ring_buffer
{
    uint32_t    cap; // capacity of buffer    
    uint32_t    rdIdx; // read index
    uint32_t    wrIdx; // write index
    uint32_t    cnt;  // available data can read
    uint8_t     *data;  // buffer
} ring_buffer_t;

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
/*******************************************************************************
* Function Name  : Ringbuf_Init
* Description    : Init a ringbufer and buffer with size paramater
* Input          : [uint32_t]size 
* Output         : None
* Return         : point to ringbufer
*******************************************************************************/
ring_buffer_t * Ringbuf_Init(uint32_t size);
/*******************************************************************************
* Function Name  : Ringbuf_IsFull
* Description    : Check ringbuffer full
* Input          : ringbufer 
* Output         : None
* Return         : -1: Error
*                :  0: Not full
*                :  1: Ringbuffer is FULL
*******************************************************************************/
int8_t Ringbuf_IsFull(ring_buffer_t *rb);
/*******************************************************************************
* Function Name  : Ringbuf_IsEmpty
* Description    : Check ringbuffer empty
* Input          : ringbufer 
* Output         : None
* Return         : -1: Error
*                :  0: Not empty
*                :  1: Ringbuffer is Empty
*******************************************************************************/
int8_t Ringbuf_IsEmpty(ring_buffer_t *rb);
/*******************************************************************************
* Function Name  : Ringbuf_GetAvailSpace
* Description    : Get available space in ringbuffer
* Input          : ringbufer (I don't check argument)
* Output         : None
* Return         : size of space
*******************************************************************************/
uint32_t Ringbuf_GetAvailSpace(ring_buffer_t *rb);
/*******************************************************************************
* Function Name  : Ringbuf_GetAvailData
* Description    : Get available data in ringbuffer
* Input          : ringbufer (I don't check argument)
* Output         : None
* Return         : size of data
*******************************************************************************/
uint32_t Ringbuf_GetAvailData(ring_buffer_t *rb);
/*******************************************************************************
* Function Name  : Ringbuf_WriteByte
* Description    : Put a byte data into ringbuffer
* Input          : ringbufer
* Input          : byte data
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_WriteByte(ring_buffer_t *rb, uint8_t bData);
/*******************************************************************************
* Function Name  : Ringbuf_ReadByte
* Description    : Pop a byte data out of ringbuffer
* Input          : ringbufer
* Input          : point to byte data
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_ReadByte(ring_buffer_t *rb, uint8_t *bData);
/*******************************************************************************
* Function Name  : Ringbuf_WriteBuf
* Description    : Put a buffer data into ringbuffer
* Input          : ringbufer
* Input          : point to buffer data
* Input          : length of buffer
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_WriteBuf(ring_buffer_t *rb, uint8_t *pBuf, uint32_t len);
/*******************************************************************************
* Function Name  : Ringbuf_ReadBuf
* Description    : Pop a buffer data out of ringbuffer
* Input          : ringbufer
* Input          : point to buffer data
* Input          : length of buffer
* Output         : None
* Return         : -1: Error 0: success
*******************************************************************************/
int8_t Ringbuf_ReadBuf(ring_buffer_t *rb, uint8_t *pBuf, uint32_t len);
/*******************************************************************************
* Function Name  : Ringbuf_Free
* Description    : Free a ringbuffer
* Input          : ringbufer
* Output         : None
* Return         : None
*******************************************************************************/
void Ringbuf_Free(ring_buffer_t *rb);


#ifdef __cplusplus
}
#endif

#endif /* __RINGBUF_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

