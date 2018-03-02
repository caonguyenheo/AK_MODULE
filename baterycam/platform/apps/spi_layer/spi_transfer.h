/**
  ******************************************************************************
  * File Name          : spi_transfer.h
  * Description        : This file contains configs of spi transfer
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _SPI_TRANSFER_H_
#define _SPI_TRANSFER_H_
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/

#include <stdbool.h>
#include "arch_spi.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/


/* define for new SDK */
/** 32 bits byte swap */
#define drv_swap_byte_32(x) \
((unsigned int)((((unsigned int)(x) & 0x000000ffUL) << 24) | \
         (((unsigned int)(x) & 0x0000ff00UL) <<  8) | \
         (((unsigned int)(x) & 0x00ff0000UL) >>  8) | \
         (((unsigned int)(x) & 0xff000000UL) >> 24)))
         
//#define SPI_REQUEST_GPIO  GPIO_SPI_CC3200   //59  23

#define SPI_ID      SPI_ID1

#define CLK_2_M     2000000
#define CLK_4_M     4000000
#define CLK_10_M    10000000
#define CLK_20_M    20000000
#define SPI_CLK     CLK_2_M

/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/


/*----------------------------------------------------------------------------*/
/*                        SPI COMMAND                                         */
/*----------------------------------------------------------------------------*/
/*
*   Brief: Init structure buffer for spi command
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-June-2017
*/
void spi_cmd_init(void);
/*
*   Brief: Free structure buffer for spi command
*   Param: None
*   Return: None
*   Author: Tien Trung
*   Date: 22-June-2017
*/
void spi_cmd_deinit(void);
/*
*   Brief: Spi command send response to CC3200
*   Param: [buf] pointer to data wanna send
*           [len] length of data send
*   Return: <0 error
*           >0 length data that push to buffer
*   Author: Tien Trung
*   Date: 22-June-2017
*/
int spi_cmd_send(char *buf, int len);
/*
*   Brief: Spi command receive command packet from CC3200
*   Param: [buf] pointer to data wanna receive
*           [len] length of data receive
*   Return: <0 error
*           >0 length data receive
*   Author: Tien Trung
*   Date: 22-June-2017
*/
int spi_cmd_receive(char *buf, int len);


int spi_fwupgrade_send(char *buf, int len);
int spi_fwupgrade_receive(char *buf, int len);

int spi_uploader_send(char *buf, int len);
int spi_uploader_receive(char *buf, int len);

int spi_p2psdstream_send(char *buf, int len);
int spi_p2psdstream_receive(char *buf, int len);

/*----------------------------------------------------------------------------*/
/*                       End of API spi cmd                                   */
/*----------------------------------------------------------------------------*/

int send_spi_data(char *buf, int len);
int read_spi_data(char *buf, int len);
void *spi_task(void *arg);

void *spi_read_test(void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _SPI_TRANSFER_H_ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/
