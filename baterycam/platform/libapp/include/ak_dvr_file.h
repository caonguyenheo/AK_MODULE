#ifndef _AK_DVR_FILE_H_
#define _AK_DVR_FILE_H_

#include "list.h"
#include "ak_common.h"
#include "ak_dvr_common.h"

struct dvr_file_entry {
	time_t calendar_time;		//calendar time, seconds from 1970-1-1 00:00:00
	unsigned long size;			//file size
	unsigned long total_time;	//record file total time
	char *path;					//including path and file full name
	struct list_head list;		//list head
};

/**  
 * ak_dvr_file_get_first - get the first record file entry according start time
 * @start_time[IN]: appointed start time
 * @type[IN]: record file type
 * @entry[OUT]: record file entry
 * return: 0 success, otherwise -1
 * notes: 1. we'll record the start node after get successfully,
 *		and then you can call ak_dvr_file_get_next
 *		2. you must free entry path memory outside
 */
int ak_dvr_file_get_first(time_t start_time, enum dvr_file_type type, 
						struct dvr_file_entry *entry);

/**  
 * ak_dvr_file_get_next - get next record file entry's full path
 * @type[IN]: record file type
 * @entry[OUT]: record file entry
 * return: 0 success, otherwise -1
 * notes: 1. make sure call ak_dvr_file_get_first firstly.
 *		2. you must free entry path memory outside
 */
int ak_dvr_file_get_next(enum dvr_file_type type, struct dvr_file_entry *entry);

/** 
 * ak_dvr_file_stop_list - stop searching history record file
 * @void
 * return: none
 * notes: the fetch record file list will be kept
 *		if you want to destroy the list, please call ak_dvr_record_exit
 */
void ak_dvr_file_stop_list(void);

/** 
 * ak_dvr_file_fetch_list - wakeup get history record file thread and fetch list
 * @void
 * return: 0 success, -1 failed
 * notes: call again after SD card reinsert
 */
int ak_dvr_file_fetch_list(void);

#endif
