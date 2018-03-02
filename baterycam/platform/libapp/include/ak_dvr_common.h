#ifndef _AK_DVR_COMMON_H_
#define _AK_DVR_COMMON_H_

#define DVR_FILE_PREFIX_MAX_LEN  20		//DVR file prefix max len in bytes
#define DVR_FILE_PATH_MAX_LEN    255	//DVR file path max len in bytes

/* dvr file type */
enum dvr_file_type {
	DVR_FILE_TYPE_AVI = 0x00,
	DVR_FILE_TYPE_MP4,
	DVR_FILE_TYPE_NUM
};

/* dvr exception */
enum dvr_exception {
    DVR_EXCEPT_NONE = 0x00,	    	//no exception
    DVR_EXCEPT_SD_REMOVED = 0x01,	//SD card removed
    DVR_EXCEPT_SD_UMOUNT = 0x02,	//SD card unmount
	DVR_EXCEPT_SD_NO_SPACE = 0x04,	//SD card space not enough
	DVR_EXCEPT_SD_RO = 0x08,		//SD card read only
	DVR_EXCEPT_NO_VIDEO = 0x10,	    //can't capture video data
	DVR_EXCEPT_NO_AUDIO = 0x20,	    //can't capture audio data
	DVR_EXCEPT_MUX_SYSERR = 0x40, 	//mux system error
};

#endif
