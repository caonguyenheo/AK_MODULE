#ifndef __PARTITION_PORT_H__
#define __PARTITION_PORT_H__

/**
* @brief	initializing partition port and spinorflash port
* @author 
* @date 2016-12-21
* @param none
* @return int: success return 0, fail return -1
* @version 
*/
int partition_port_init();

/** 
 * kernel_partition_open - 
 * @partition_name[IN]: 
 * return: opened partition handle; NULL failed; 
 */
void* kernel_partition_open(const char *partition_name);


/** 
 * kernel_partition_write - 
 * @handle[IN]: opened handle in ak_partition_open
 * @data[IN]: data to write
 * @len[IN]: data len
 * return: 0 success, -1 failed
 */
int kernel_partition_write(const void *handle, const char *data, unsigned int len);


/** 
 * kernel_partition_read - 
 * @handle[IN]: opened handle in ak_partition_open
 * @data[OUT]: data buf for read
 * @len[IN]: data len
 * return: 0 success, -1 failed
 */
int kernel_partition_read(const void *handle, char *data, unsigned int len);


/** 
 * kernel_partition_close - 
 * @handle[IN]: opened handle in ak_partition_open
 * return: 0 success, -1 failed
 */
int kernel_partition_close(const void *handle);

/**
 * kernel_partition_get_dat_size-get partition data size
 * @handle[IN]: opened handle in ak_partition_open
 * return:   success return data_size, fail return 0
 */
unsigned long kernel_partition_get_dat_size(void *handle);

/**
 * kernel_partition_get_size - get partition size (bytes)
 * @handle[IN]: opened handle in ak_partition_open
 * return:    success return partition size, fail return 0
 */
unsigned long kernel_partition_get_size(void *handle);

#endif//#ifndef __PARTITION_PORT_H__

