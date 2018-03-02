/**@file tcard_dev_drv.h
 * @brief provide  operations of how to control sd.
 *
 * This file describe sd  driver.
 * Copyright (C) 2017 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author  
 * @date    2017-10-27
 * @version 1.0
 */


#ifndef _TCARD_DEV_DRV_H
#define _TCARD_DEV_DRV_H


/**
* @brief initial mmc sd or comob card
* @author  
* @date 2017-10-27
* @return void *
* @retval NON-NULL  set initial successful,card type is  mmc sd or comob
* @retval NULL set initial fail,card type is not mmc sd or comob card
*/
void * sd_dev_drv_init(void);


/**
 * @brief power off sd controller
 * @author 
* @date 2017-10-27
 * @param handle[in] card handle,a pointer of void
 * @return void
 */
void sd_dev_drv_release(void * handle);

/**
 * @brief read data from sd card 
 * @author 
 * @date 2017-10-27 
 * @param handle[in] card handle,a pointer of void
 * @param block_src[in] source block to read
 * @param databuf[out] data buffer to read
 * @param block_count[in] size of blocks to be readed
 * @return bool
 * @retval  true: read successfully
 * @retval  false: read failed
 */
bool sd_dev_read_block(void* handle,unsigned long block_src, unsigned char *databuf, unsigned long block_count);

/**
 * @brief write data to sd card 
 * @author
 * @date 2017-10-27 
 * @param handle[in] card handle,a pointer of void
 * @param block_dest[in] destation block to write
 * @param databuf[in] data buffer to write
 * @param block_count[in] size of blocks to be written
 * @return bool
 * @retval  true:write successfully
 * @retval  false: write failed
 */
bool sd_dev_write_block(void* handle,unsigned long block_dest, const unsigned char *databuf, unsigned long block_count);

/**
 * @brief get sd card information
 * @author 
 * @date 2017-10-27 
 * @param handle[in] card handle,a pointer of void
 * @param total_block[out] current sd's total block number
 * @param block_size[out] current sd's block size
 * @a block = 512 bytes
 * @return void
 */
void sd_dev_get_info(void* handle, unsigned long *total_block, unsigned long *block_size);


#endif //#ifndef _TCARD_DEV_DRV_H


