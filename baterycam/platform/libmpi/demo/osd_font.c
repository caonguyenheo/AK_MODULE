#include <string.h>
#include "ak_common.h"
#include "ak_vpss.h"
#include "ak_partition.h"
#include "kernel.h"

#define UNICODE_START_CODE  0X4E00
#define UNICODE_END_CODE    0X9FA5

#define FONT_BIN "FONT"

#define OSD_UNICODE_CODES_NUM		100	
#define FONT_MAX           			OSD_UNICODE_CODES_NUM

struct font_data_node {
	unsigned short unicode;
	unsigned char  *data;		
};

struct font_data_info {
	int num;    			/* font number */ 
	int fd;    				/* fd for font file */ 
	int byte;  				/* number byte of each font use */
	unsigned int size; 		/* font size in pixel */   
	struct font_data_node buf[FONT_MAX];    
};

static int init_flag = 0;

/*the buf reserve all font data of basic char from partition */ 
static struct font_data_info osd_font;
static char font_data_file[VPSS_OSD_CHANNELS_MAX][100];

/* 标准的显示汉字，包括"星期一二三四五六天年月日" */
static unsigned short basic_char[] = {
	'-', ':', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
	'o','p','q','r','s','t','u','v','w','x','y','z',
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N',
	'O','P','Q','R','S','T','U','V','W','X','Y','Z',
	0x5e74, 0x6708, 0x65e5, 0x661f, 0x671f, 0x5929, 0x4e00, 0x4e8c, 
	0x4e09, 0x56db, 0x4e94, 0x516d
};

/* find all basic_char in partition */
static int find_word_count = 0;

/*  the osd font background color*/
static int background_color = 0;

/* the osd font color*/
static int font_color = 0;

#define mencius

/** 
 * @brief - find font data from font buf. 
 * @code[IN]:  the unicode of the font 
 * return: char* - success; otherwise NULL; 
 */
char* get_osd_font_data_one(unsigned short code)
{
	int i = 0;
	int len = osd_font.num;
	
	for(i = 0 ; i < len; i++)
	{
		if(osd_font.buf[i].unicode == code)
			return osd_font.buf[i].data;
	}
	ak_print_error("can't find font:%x\n",code);
	return NULL;
}

/** 
 * @brief - free the buf for all font data of basic char from 
 *			partition. 
 * return: 0 - success; otherwise -1; 
 */
static void free_basic_font_data(void)
{
	int len = osd_font.num;	

	if(1 == init_flag) {
		init_flag = 0;
	
		for(int i=0; i<len; ++i) {
			if(osd_font.buf[i].data) {
				free(osd_font.buf[i].data);
				osd_font.buf[i].data = NULL;
			}
		}

		memset(&osd_font, 0x00, sizeof(struct font_data_info));
	}
}

/** 
 * @brief  malloc memery for all font data of basic char from partition. 
 * @size[IN]: the font size[0,1]  
 * return: 0 - success; otherwise -1; 
 */
static int init_basic_font_data(int size)
{
	int i = 0;
	int ret = AK_FAILED;
	int len = sizeof(basic_char) / sizeof(basic_char[0]);	

	if(1 == init_flag) {
		return AK_SUCCESS;
	}

	init_flag = 1;
	memset(&osd_font, 0x00, sizeof(struct font_data_info));

	osd_font.byte = size * size / 8;
	osd_font.size = size;

	ak_print_normal_ex("font size=%d\n",osd_font.size);
	
	/* init font data */
	osd_font.num = len;  
	for(i=0; i<len; ++i) {
		osd_font.buf[i].unicode = basic_char[i];

		/* osd_font.byte*4:use ak_osd_draw_matrix(), font buf convert to matrix */
		osd_font.buf[i].data= (unsigned char *)calloc(1,  osd_font.byte * 4);

		if (!osd_font.buf[i].data) {
			break;
		}
	}
	
	if (i >= len) {
		ret = AK_SUCCESS;
	} else {
		
		ak_print_error_ex("calloc font data error.\n");
		free_basic_font_data();
	}
	return ret;
}

/** 
 * @brief - change a font data to matrix.
 *	 
 * @data[OUT]: the font matrix buf  
 * @p_font[IN]: the font data from partition  
 * @font_byte[IN]: the font size in partition  (unit:byte) 
 *
 */
static void change_font_to_matrix(unsigned char* data,
		unsigned char *p_font, int font_byte)
{
	int i = 0, j=0;

	/* def_color_tables's index */
	char color_index[2]={background_color,font_color};
	int dot_bit_value = 0;
	int matrix_data_out = 0;
	char dot_value_char;
	
	for(i=0; i<font_byte; i++)
	{
		/* clean matrix_data_out */
		matrix_data_out = 0;
		
		dot_value_char = *(p_font +i);
		/* chang 1 byte to 4 bytes use new color info*/
		for(j=0; j < 8; j++)
		{
			dot_bit_value = (dot_value_char >> j) & 0x1 ;
			if(j == 0)
				matrix_data_out = matrix_data_out | color_index[dot_bit_value];
			else
				matrix_data_out = (matrix_data_out << 4) | color_index[dot_bit_value];
		}

		memcpy((data+(i*4)),&matrix_data_out, sizeof(matrix_data_out));

	}

}

/** 
 * @brief - get all font data of basic char from file 
 *			and reserve in buf. 
 * @code[IN]: the unicode of the font  
 * @p_font[OUT]:  the font data in font buf
 */
static void get_basic_font_data(unsigned short code, 
		unsigned char * p_font)
{
	int i = 0;
	int len = osd_font.num;
	find_word_count = len;
	
	for( i=0; i<len; i++) {

		if(basic_char[i] == code)
		{
			change_font_to_matrix(osd_font.buf[i].data,p_font, osd_font.byte);

			find_word_count--;
			break;
		}

	}

}

/** 
 * @brief - find font data from font partition in basic_char
 * 				to reserve buf
 * return: 0 - success; otherwise -1; 
 */
static int set_osd_font(void)
{

	/** step1: Open font partition */
	void* font_fd = (void *)ak_partition_open(FONT_BIN);
	if(font_fd == NULL) {
		ak_print_error("we can't open the font partition:%s\n",FONT_BIN);
		return AK_FAILED;
	}

	int font_len = osd_font.byte;

	/* font_len + unicode len */
	int osd_font_len = font_len+sizeof(unsigned short);
	
	/*
	  * read 128 words once ,
	  * read_count_once%font_len == 0
	  * read_count_once%flash_page_count == 0 need to know page size, now page_size is 256
	  */
	int words_read_count = 128;
	int read_count_once = osd_font_len*words_read_count;
	unsigned char * p_read_part_once = calloc(1, read_count_once);
	
	unsigned short  *p_word_tmp = NULL;

	int ret  =  AK_SUCCESS, i = 0;
start_loop:
	/** step2: Read font partition */
	ret = ak_partition_read(font_fd, p_read_part_once, read_count_once);
	if (ret == -1)
	{

		ak_print_error_ex("the end of the font file.\r\n");
		if (find_word_count > 0)
		{
			ak_print_error_ex("has words not in the font file,plean support a new font file.\r\n");
			ret =  AK_FAILED;
		}
		goto end_loop;
	}

	for(i=0; i<words_read_count ; i++)
	{
		p_word_tmp = (unsigned short *)(p_read_part_once + (i * osd_font_len));

		/* 0x0 and 0xffff is font partition over */
		if(0x0 == *p_word_tmp || 0xFFFF == *p_word_tmp)
			goto end_loop;

		/*
		  *compare unicode with basic font 
		  *p_word_tmp+1 is offset 2 bytes,skip unicode
		  */
		get_basic_font_data((*p_word_tmp), (unsigned char*)(p_word_tmp+1));

		if(find_word_count == 0)
			goto end_loop;
	}

	if(i == words_read_count)
		goto start_loop;

end_loop:	
	free(p_read_part_once);

	/** step3:  Close partition*/
	if(font_fd)
	{
		ak_partition_close(font_fd);
		font_fd = NULL;
	}

	return ret;
}

void set_osd_font_color(int f_color, int bg_color)
{
	font_color = f_color;
	background_color = bg_color;
}

/** 
 * @brief - init osd font data. 
 * @size[IN]:  size of font  in font data file 
 * return: AK_SUCCESS - success; otherwise AK_FAILED; 
 */
int init_osd_font(int size)
{ 

	unsigned long start_time, end_time;
	struct ak_timeval start_tv, end_tv;

	int ret = AK_SUCCESS;
	find_word_count = 0;
	
	/* init all basic font buf */
	ret = init_basic_font_data(size);
	if(ret == AK_FAILED)
		return ret;

	ak_get_ostime(&start_tv);

	/* reserve font data to basic font buf from partition */
	ret = set_osd_font();
	if( ret == AK_FAILED )
		ak_print_error_ex("get font from partition failed.");

	ak_get_ostime(&end_tv);
	
	ak_print_notice_ex("load fond time %ld us.", end_tv.usec- start_tv.usec);
	return ret;
}

/** 
 * @brief - destroy osd font data. 
 * @param:  none 
 * return: none 
 */
void destroy_osd_font(void)
{
	free_basic_font_data();
}
