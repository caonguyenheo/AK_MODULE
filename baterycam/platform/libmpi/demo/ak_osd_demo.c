/******************************************************
 * @brief  osd demo
 
 * @Copyright (C), 2016-2017, Anyka Tech. Co., Ltd.
 
 * @Modification  2017-5-17 
*******************************************************/
#include <string.h>
#include "ak_common.h"
#include "ak_vi.h"
#include "ak_osd.h"
#include "command.h"
#include "drv_api.h"
#include "libc_mem.h"
#include "osd_font.h"
#include "kernel.h"

/******************************************************
  *                    Constant         
  ******************************************************/
static char *help[]={
	"osd module demo",
	"usage:osddemo <content> <fontsize> <position> <font_color> <bg_color>\n"
	"      content : osd content,\n"
	"      fontsize: osd font size,defult:24,\n"
	"      position: osd show position,1 <= position <= 5,\n"
	"            1-full(defalut),2-leftup,3-leftdown,4-rightup,5-rightdown\n"
	"      font_color:1 <=font_color <=16, defualt 16(black)\n"
	"      bg_color: 1 <=background_color <=16 default 2(white)\n",
	"      sample : osddemo\n"
	"               osddemo babyroom 24 2\n"
	
};

/*str1 is : 2016-11-10 18:18:18 anyka 星期一 */
static unsigned short osd_str1[] =
{ '2', '0', '1', '6', '-', '1', '1', '-','1', '0',' ',
  '1', '8', ':','1', '8', ':','1', '8', ' ',
  'A','n','y','k','a',0x661f,0x671f ,0x56db,'\0'};

/*str2 is : 16年10月18日*/
static unsigned short osd_str2[] =
{
	'1', '6',0x5e74, '1', '0',0x6708,'1', '8', 0x65e5,'\0',
};

char * osd_pos_str[5] = {"full","leftup","leftdown",
								"rightup","rightdown"};

/******************************************************
  *                    Macro         
  ******************************************************/
#define CH0_WIDTH 1280
#define CH0_HEIGHT 720
#define CH1_WIDTH 640
#define CH1_HEIGHT 480


/* tf card the least free size */
#define MIN_DISK_SIZE_FOR_OSD ((CH0_WIDTH*CH0_HEIGHT*3+CH1_WIDTH * CH1_HEIGHT*3)/1024)
/******************************************************
  *                    Type Definitions         
  ******************************************************/

/******************************************************
  *                    Global Variables         
  ******************************************************/
static void * vi_handle = NULL;





static int font_size = 0;
static int osd_pos_str_index = 0;
static short* user_osd_str = NULL;

/******************************************************
*               Function prototype                           
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/

/**
 *ai_get_disk_free_size - get disk free size, appoint the dir path
 * @path[IN]: disk dir full path
 * return: disk free size in KB, -1 failed
 */

static unsigned long osd_get_disk_free_size(const char *path)
{
	struct statfs disk_statfs;
    unsigned long free_size;
	
	memset(&disk_statfs, 0, sizeof(struct statfs));

	if( statfs( path, &disk_statfs ) == -1 ) 
	{		
		ak_print_error("fs_get_disk_free_size error!\n"); 
		return -1;
	}
	
    free_size = disk_statfs.f_bsize;
    free_size = free_size / 512;
    free_size = free_size * disk_statfs.f_bavail / 2;
   
	return free_size;
}

/** 
 * @brief  init sensor. 
 * @handle[IN]:  the handler of the sensor 
 * return: 0 - success; otherwise -1; 
 */
static int osd_init_video_input(void * handle)
{
	struct video_resolution video_res;
	struct video_channel_attr ch_attr ;

	/* get sensor resolution */
	memset(&video_res, 0, sizeof(struct video_resolution));
	if (ak_vi_get_sensor_resolution(handle, &video_res))
		ak_print_error_ex("ak_mpi_vi_get_sensor_res fail!\n");
	else
		ak_print_normal("ak_mpi_vi_get_sensor_res ok! w:%d, h:%d\n",
			video_res.width, video_res.height);

	/* set video resolution and  crop information */
	memset(&ch_attr, 0, sizeof(struct video_channel_attr));
	ch_attr.res[VIDEO_CHN_MAIN].width = CH0_WIDTH;
	ch_attr.res[VIDEO_CHN_MAIN].height = CH0_HEIGHT;
	ch_attr.res[VIDEO_CHN_SUB].width = CH1_WIDTH;
	ch_attr.res[VIDEO_CHN_SUB].height = CH1_HEIGHT;
	ch_attr.crop.width = video_res.width;
	ch_attr.crop.height = video_res.height;

	if (ak_vi_set_channel_attr(handle, &ch_attr)) {
		ak_print_error("ak_vi_set_channel_attr fail!\n");
		return -1;
	} else {
		ak_print_normal("ak_vi_set_channel_attr ok!\n");
	}

	memset(&ch_attr, 0, sizeof(struct video_channel_attr));
	if (ak_vi_get_channel_attr(handle, &ch_attr)) {
		ak_print_error("ak_vi_get_channel_attr fail!\n");
	}

	return 0;
}

/*****************************************
 * @brief  save screen data. yuv format
 * @param handle[in]  vi handler
 * @param pos_id[in]  osd position index
 * @return on success return 0, fail return -1
 *****************************************/
static int get_yuv_file(void * handle, int pos_id)
{
	struct video_input_frame frame;
	FILE *fd = NULL;
	char name[128] = {0};
	struct ak_date systime;
	int i=0, channel = 0;

	memset(&frame, 0, sizeof(struct video_input_frame));
	/* clear frame buf which is old data */
	while((ak_vi_get_frame(handle, &frame) == 0)){
				ak_vi_release_frame(handle, &frame);
		memset(&frame, 0, sizeof(struct video_input_frame));
		if( i++ > 2) break;
	}

	/* i is save photo count.*/
	for ( i = 0; i < 2; i++) {
		memset(&frame, 0, sizeof(struct video_input_frame));
		if (ak_vi_get_frame(handle, &frame)) {
			ak_print_error("ak_video_input_get_frame fail!\n");
			ak_sleep_ms(2*1000);
			continue;
		} else {
			ak_print_normal("ak_video_input_get_frame ok!\n");

			/* save the YUV frame data */
			ak_get_localdate(&systime);
			for(channel = 0; channel < 2; channel++){

				memset(name, 0, 32);	    
				sprintf(name, "a:/CH%1d%s_%d%02d%02d%02d%02d%02d_%1d.yuv",
						channel,
						osd_pos_str[pos_id],
						systime.year, 
						systime.month, systime.day, systime.hour, systime.minute, 
						systime.second,
						i);
				ak_print_normal("filename:%s\n",name);
				fd = fopen(name,"w");
				if (fd != NULL) {
					if (frame.vi_frame[channel].len == fwrite(
								frame.vi_frame[channel].data, 1,
								frame.vi_frame[channel].len, fd)) {
						ak_print_normal_ex(" save YUV file ok!!--i=%d\n",i);
					}
					else {
						ak_print_error_ex(" save YUV file fail!!--i=%d\n",i);
					}
					fclose(fd);
				} else {
					ak_print_error("open YUV file fail!!\n");
				}
			}
			/* the frame has used,release the frame data */
			ak_vi_release_frame(handle, &frame);
			ak_sleep_ms(1000);
		}
	}

	return 0;
}
static int short_str_len(const short* tmp_str)
{

	int len = 0;
	if(tmp_str == NULL)
		return 0;
	while(*tmp_str != '\0')
	{
		
		++len;
		++tmp_str;
	}

	return len;
		
}
static void  draw_osd_str( const int chn, const short * osd_str)
{
	int font_w = font_size, font_h = font_size;

	int osd_str1_count = short_str_len(osd_str);

	int max_w, max_h;

	if (ak_osd_get_max_rect(chn, &max_w, &max_h) < 0) {
		ak_print_error_ex("chn:%d ak_osd_get_max_rect fail!\n", chn);
		return ;
	}
	//left-top,left_down right-top ,right-down
	int main_startx[4] = { 0,0,max_w,max_w};
	int main_starty[4] = { 0,max_h,0,max_h};
	
	/* process 4 position */
	main_starty[1] -= font_h;
	main_starty[3] = main_starty[1];

	ak_print_normal_ex("ch%d,str_count:%d(%d,%d),(%d,%d),(%d,%d),(%d,%d)\n",
			chn,osd_str1_count,
			main_startx[0],main_starty[0],
			main_startx[1],main_starty[1],
			main_startx[2],main_starty[2],
			main_startx[3],main_starty[3]);
	
	int osd_pos_id = osd_pos_str_index;
	int pos_show = 4, is_full = 1;
	int all_osd_font_len = 0;
	int osd_str1_index = 0;
	if ( 0 != osd_pos_id )
	{
		pos_show = osd_pos_id +1;
		is_full = 0;
	}

	/* calculate string length */
	for( osd_str1_index = 0; osd_str1_index < osd_str1_count; osd_str1_index++)
	{
		//ak_print_normal("idx:%d,now_ch:%c,%x\n",osd_str1_index,*(osd_str+osd_str1_index-1),*(osd_str+osd_str1_index-1));
		if(osd_str1_index != 0 
				&& *(osd_str+osd_str1_index-1) < 0xff
				)
				all_osd_font_len += font_w/2;
			else //if(osd_str1_index != 0 )
				all_osd_font_len += font_w;
			
			if(*(osd_str+osd_str1_index) == ' ')
			{
				continue;
			}

			if(osd_str1_index == osd_str1_count - 1)
			{	
				//all_osd_font_len += (*(osd_str+osd_str1_index) < 0xff ? font_w/2 : font_w);

				main_startx[2] -= all_osd_font_len;

				main_startx[3] = main_startx[2];
			}
	}

	// ak_print_normal("osd_len:%d\n",all_osd_font_len);
	/* display osd on 4 position: "leftup","leftdown","rightup","rightdown" */
	for (; osd_pos_id < pos_show; osd_pos_id++) {
		
		/* draw channel 0 osd */
		// ak_print_normal("osd_pos_id:%d,%d\n",osd_pos_id,pos_show);
		int offsetx=main_startx[is_full?osd_pos_id:osd_pos_id-1];
		char *osd_char;
		
		/* one bit change to color index */
		int matrix_size = font_size*font_size/8/*osd_font.byte*/*4;
		
	
		for( osd_str1_index = 0; osd_str1_index < osd_str1_count; osd_str1_index++)
		{
			// ak_print_normal("osd_str1_index:%d\n",osd_str1_index);
			if(osd_str1_index != 0 
					&& *(osd_str+osd_str1_index-1) < 0xff
					)
				offsetx += font_w/2;
			else if(osd_str1_index != 0 )
				offsetx += font_w;
			
			if(*(osd_str+osd_str1_index) == ' ')
			{
				continue;
			}

			osd_char = get_osd_font_data_one(*(osd_str+osd_str1_index));

			// ak_print_normal("offsetx:%d,y:%d\n",offsetx,main_starty[is_full?osd_pos_id:osd_pos_id-1]);
			if( ak_osd_draw_matrix(chn, 
				offsetx, main_starty[is_full?osd_pos_id:osd_pos_id-1], 
				font_w, font_w , osd_char, matrix_size) < 0 )
			{ 
				ak_print_error_ex("ak_osd_draw_matrix fail!\n");
				return;
			}
		}
		ak_print_normal_ex("ch0 ak_osd_draw_matrix ok!\n");

		
		
	}
	
}
static void set_osd_rect(const int channel)
{
	int width = (channel == 0) ? CH0_WIDTH : CH1_WIDTH;
	int height = (channel == 0) ?  CH0_HEIGHT: CH1_HEIGHT;
	int max_w, max_h;

	if (ak_osd_get_max_rect(channel, &max_w, &max_h) < 0) {
		ak_print_error_ex("chn:%d ak_osd_get_max_rect fail!\n", channel);
		return;
	}
	ak_print_normal_ex("chn:%d ak_osd_get_max_rect ok, max_w: %d max_h: %d\n",
			channel, max_w, max_h);

	width = (width > max_w) ? max_w : width;
	height = (height > max_h) ? max_h : height;

	if (ak_osd_set_rect(vi_handle, channel, 0, 0, width, height) < 0) {
		ak_print_error("chn: %d ak_osd_set_rect fail!\n", channel);
	}else{
		ak_print_normal("chn: %d ak_osd_set_rect ok!\n", channel);
	}
}
/*****************************************
 * @brief start to process osd
 * @param [void]  
 * @return on success return 0, fail return -1
 *****************************************/
static int test_osd(void)
{

	/** step1: init font lib to memery */
	if (init_osd_font(24) < 0)	{
		ak_print_error("ak_osd_set_font_file fail!\n");
		return -1;
	}
	ak_print_normal("ak_osd_set_font_file ok!\n");

	/** step2: init osd font and channel config */
	if (ak_osd_init(vi_handle) < 0) {
		ak_print_error("ak_osd_init fail!\n");
		return -1;
	}
	ak_print_normal("ak_osd_init ok!\n");

	set_osd_rect(0);
	set_osd_rect(1);
	if(user_osd_str != NULL)
		{
		draw_osd_str(0, user_osd_str);
		draw_osd_str(1, user_osd_str);
		}
	else
		{
		draw_osd_str(0,osd_str1);
		draw_osd_str(1,osd_str2);
		}

	/** step5: save frames to show osd */
	int ret = ak_mount_fs(DEV_MMCBLOCK, 0, "");	
	if (ret < 0)
	{
		ak_print_error("mount sd fail!\n");
		goto finish_free;
	}
	

	char *path = "a:/";
	int free_size = osd_get_disk_free_size(path);
	
	if ( free_size < MIN_DISK_SIZE_FOR_OSD)
	{
		ak_print_error_ex("the card memery isn't enough!(%lu,%d)\n",
			    free_size, MIN_DISK_SIZE_FOR_OSD);
		goto ERR;
	}

	/* get screen data and save it in file. yuv format */
	if (get_yuv_file(vi_handle, osd_pos_str_index) < 0) {
		ak_print_error_ex("get_yuv_file fail!\n");
	}
	
	/* free font buf memery */
	destroy_osd_font();

ERR:
	// ak_unmount_fs(DEV_MMCBLOCK, 0, ""); //comment by ymx
finish_free:
	ak_osd_destroy();

	return 0;
}

/*****************************************
 * @brief match sensor
 * @param [void]  
 * @return on success return 0, fail return -1
 *****************************************/
static int match_sensor(void)
{
	char *main_addr ="ISPCFG";
	if (ak_vi_match_sensor(main_addr)) {
		ak_print_error(" match sensor first_cfg fail!\n");
		return -1;
	} else {
		ak_print_normal(" ak_vi_match_sensor ok!\n");
	}

	return 0;
}

static short* change_to_short_str(char * ch)
{

	int str_len = strlen(ch);
	ak_print_normal("ymx:len:%d\n",str_len);
	int i = 0;
	short* st_str = malloc(sizeof(short)  * (str_len + 1));

	for(; i<str_len; i++)
		*(st_str+i) = *(ch+i);

	*(st_str+i) = '\0';	
	return st_str;
}

/*****************************************
 * @brief init params
 * @param argc[in]  the count of command param
 * @param args[in]  the command param
 * @return void
 *****************************************/
static int init_params(int argc, char **args)
{
	int i = 0;
	font_size = 24;
	if (argc > 5)
	{
		return -1;
	}

	int background_color = 0x1 ;/* background is white*/
	int font_color = 0xf;/* font is black*/
	osd_pos_str_index = 0;
	for (i = 0; i < argc; i++) {
		char* p_end_ptr = NULL;


		switch (i) {
			case 0:
				//ak_print_normal("ymx:case0:%s\n",args[i]);
				user_osd_str = change_to_short_str(args[i]);
				//user_osd_str = args[i];
				break;
			case 1:
				font_size = strtol(args[i], &p_end_ptr, 10 );
 				if ( font_size != 24 || *p_end_ptr != '\0' ) 
					goto param_invalid;
				break;
			case 2:

				osd_pos_str_index = strtol(args[i], &p_end_ptr, 10 );
				//osd_pos_str_index= atoi(args[i]);
				if ( osd_pos_str_index <1 || osd_pos_str_index > 5 
					|| *p_end_ptr != '\0' ) 
					goto param_invalid;
				osd_pos_str_index -= 1;
				break;
			case 3:
				font_color= strtol(args[i], &p_end_ptr, 10 );
				//font_color= atoi(args[i]);
				if (font_color <1 || font_color > 16 
					|| *p_end_ptr != '\0'
					) 
					goto param_invalid;

				font_color -= 1;
				break;
			case 4:
				background_color= strtol(args[i], &p_end_ptr, 10 );
				if (background_color <1 || background_color > 16
					|| *p_end_ptr != '\0'
					) 
					goto param_invalid;

				background_color -= 1;
				break;

			default:
				ak_print_error_ex("i err:%d\n", i);
		}
	}

	set_osd_font_color(font_color, background_color);
	return 0;
param_invalid:
	ak_print_error_ex("illegal parameter:%s\n", args[i]);
	return -1;
}

/*****************************************
 * @brief start function for command
 * @param argc[in]  the count of command param
 * @param args[in]  the command param
 * @return void
 *****************************************/
void cmd_osd_test(int argc, char **argv)
{
	
	ak_print_notice("*** osd demo begin compile:%s %s %s ***\n",
			__DATE__, __TIME__, ak_osd_get_version());

	/* init params*/
	if(init_params(argc,argv))
	{
		ak_print_error("%s",help[1]);
		goto ERR;
	}

	/** step1: match sensor*/
	if (match_sensor()) {
		ak_print_error("match sensor fail!\n");
		goto ERR;
	}

	/** step2: open sensor*/
	vi_handle = ak_vi_open(VIDEO_DEV0);
	if (NULL == vi_handle) {
		ak_print_error("ak_vi_open fail!\n");
		goto ERR;
	} else {
		ak_print_normal("ak_vi_open ok!\n");
	}

	/** step3: init sensor*/
	if(osd_init_video_input(vi_handle) == 0){

		ak_vi_capture_on(vi_handle);
		/** step4: process osd*/
		test_osd();

		ak_vi_capture_off(vi_handle);
	}

	ak_vi_close(vi_handle);

	ak_print_notice("********** osd demo finish ************\n");

	ERR:
		return;
		
}

/*****************************************
 * @brief register osddemo command
 * @param [void]  
 * @return 0
 *****************************************/
static int cmd_osd_reg(void)
{
    cmd_register("osddemo", cmd_osd_test, help);
    return 0;
}

cmd_module_init(cmd_osd_reg)
