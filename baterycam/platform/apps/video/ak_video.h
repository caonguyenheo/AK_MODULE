#ifndef _AK_VIDEO_H_
#define _AK_VIDEO_H_

typedef struct doorbell_snapshot_data{
    unsigned int length;
    char *data;
}DOORBELL_SNAPSHOT_DATA_t;


// void ak_video_encode_start(void);
void *ak_video_encode_task(void *arg);

void *db300_ak_record_sd_task(void *arg);

#ifdef SYS_ENCODE_JPEG
void *ak_video_take_snapshot_task(void *arg);

void *ak_video_take_snapshot_task_2(void *arg);

#endif
int video_finish_snapshot(void);

unsigned long video_get_timestamp(void);
unsigned int snapshot_get_size_data(void);
int snapshot_get_file_name(char *pName);
int clip_get_file_name(char *pName);


int video_get_system_info_groupname(void);

void thirdparty_record_done(int duration, char* filename, int filesize, int errorcode);

int check_status_record(void);

//Thuan add - 18/10/2017
extern char g_acDirName_Clip[256];

#endif /* _GM_VIDEO_H_ */

