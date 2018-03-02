#ifndef __OSD_FOND_H__
#define __OSD_FOND_H__

/** 
 * @brief - init osd font data. 
 * @size[IN]:  size of font  in font data file 
 * return: AK_SUCCESS - success; otherwise AK_FAILED; 
 */
int init_osd_font(int size);

/** 
 * @brief - set osd font color and backgroud color. 
 * @f_color[IN]:  osd font color
 * @bg_color[IN]:  osd bakground color
 * return: none; 
 */
void set_osd_font_color(int f_color, int bg_color);

/** 
 * @brief - find font data from font buf. 
 * @code[IN]:  the unicode of the font 
 * return: char* - success; otherwise NULL; 
 */
char* get_osd_font_data_one(unsigned short code);

/** 
 * @brief - destroy osd font data. 
 * @param[IN]:  none 
 * return: none 
 */
void destroy_osd_font(void);


#endif



