#ifndef __ISP_MODULES_INTERFACE_C__
#define __ISP_MODULES_INTERFACE_C__

#include "anyka_types.h"

typedef enum isp_module_id{
	ISP_BB = 0,   			//��ƽ��
	ISP_LSC,				//��ͷУ��
	ISP_RAW_LUT,			//raw gamma
	ISP_NR,					//NR
	ISP_3DNR,				//3DNR

	ISP_GB,					//��ƽ��
	ISP_DEMO,				//DEMOSAIC
	ISP_GAMMA,				//GAMMA
	ISP_CCM,				//��ɫУ��
	ISP_FCS,				//FCS

	ISP_WDR,				//WDR
	ISP_SHARP,				//SHARP
	ISP_SATURATION,			//���Ͷ�
	ISP_CONSTRAST,			//�Աȶ�

	ISP_RGB2YUV,			//rgb to yuv
	ISP_YUVEFFECT,			//YUVЧ��
	ISP_DPC,				//����У��
	ISP_WEIGHT,				//Ȩ��ϵ��
	ISP_AF,					//AF

	ISP_WB,					//WB	
	ISP_EXP,				//Expsoure
	ISP_MISC,				//����
	ISP_Y_GAMMA,			//y gamma
	ISP_HUE,				//hue

	ISP_3DSTAT,				//3D����ͳ��
	ISP_AESTAT,				//AEͳ��
	ISP_AFSTAT,				//AFͳ��
	ISP_AWBSTAT,			//AWBͳ��

	ISP_SENSOR,				//sensor����

	/*
		sub modules defined
	*/
	ISP_BB_SUB_BLC,
	ISP_LSC_SUB_LSC,
	ISP_RAW_LUT_SUB_LUT,
	ISP_NR_SUB_NR1,
	ISP_NR_SUB_NR2,
	ISP_3DNR_SUB_3DNR,
	ISP_GB_SUB_GB,
	ISP_DEMO_SUB_DEMO,
	ISP_GAMMA_SUB_GAMMA,
	ISP_CCM_SUB_CCM,
	ISP_FCS_SUB_FCS,
	ISP_WDR_SUB_WDR,
	ISP_WDR_SUB_WDR_EX,
	ISP_SHARP_SUB_SHARP,
	ISP_SHARP_SUB_SHARP_EX,
	ISP_SATURATION_SUB_SATURATION,
	ISP_CONSTRAST_SUB_CONSTRAST,
	ISP_YUVEFFECT_SUB_YUVEFFECT,
	ISP_RGB2YUV_SUB_RGB2YUV,
	ISP_DPC_SUB_DPC,
	ISP_DPC_SUB_SDPC,
	ISP_WEIGHT_SUB_WEIGHT,
	ISP_AF_SUB_AF,
	ISP_WB_SUB_WB,
	ISP_WB_SUB_MWB,
	ISP_WB_SUB_AWB,
	ISP_WB_SUB_AWB_EX,
	ISP_EXP_SUB_RAW_HIST,
	ISP_EXP_SUB_RGB_HIST,
	ISP_EXP_SUB_YUV_HIST,
	ISP_EXP_SUB_EXP_TYPE,
	ISP_EXP_SUB_FRAME_RATE,
	ISP_EXP_SUB_AE,
	ISP_MISC_SUB_MISC,
	ISP_Y_GAMMA_SUB_Y_GAMMA,
	ISP_HUE_SUB_HUE,

	ISP_AESTAT_SUB_AE,
	ISP_AESTAT_SUB_RAW_HIST,
	ISP_AESTAT_SUB_RGB_HIST,
	ISP_AESTAT_SUB_YUV_HIST,
	
	/********** add new items for new requirement ************/
	ISP_FPS_CTRL,			//frame rate control
	ISP_AESTAT_SUB_RGB_STAT,
	
	ISP_MODULE_ID_NUM
} T_MODULE_ID;

/**
 * @brief set isp module configuration
 * @author ye_guohong   
 * @date 2016-10-19
 * @param[in] module_id isp module id
 * @param[in] buf memory for this module
 * @return bool
 * @retval true if set successfully
 * @retval false if set unsuccessfully
 */
bool ispdrv_set_module(unsigned long module_id, unsigned char *buf);

/**
 * @brief get isp module configuration
 * @author ye_guohong   
 * @date 2016-10-19
 * @param[in] module_id isp module id
 * @param[in] buf memory for this module
 * @return bool
 * @retval true if get successfully
 * @retval false if get unsuccessfully
 */
bool ispdrv_get_module(unsigned long module_id, unsigned char *buf);

/**
 * @brief pause isp
 * @author ye_guohong   
 * @date 2016-10-19
 * @param[]
 * @return bool
 * @retval true  successfully
 * @retval false  unsuccessfully
 */
bool ispdrv_isp_pause(void);

/**
 * @brief resume isp
 * @author ye_guohong   
 * @date 2016-10-19
 * @param[]
 * @return bool
 * @retval true  successfully
 * @retval false  unsuccessfully
 */
bool ispdrv_isp_resume(void);

#endif