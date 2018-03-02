/**
  ******************************************************************************
  * File Name          : audio_agc.h
  * Description        : This file contain API prototype and definitions
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 PREMIELINK PTE LTD
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PREMIELINK_AUDIO_AGC_H__
#define __PREMIELINK_AUDIO_AGC_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 


/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
#define OUT_RANGE_LOWER     0
#define IN_RANGE            1
#define OUT_RANGE_HIGHER    2
#define AGC_ALGO_TIME 		(2*1000)
// #define AGC_ALGO_TIME 		(700)		// ms


#define MIC_AGAIN_DEFAULT		6
#define MIC_DGAIN_DEFAULT		6

enum{
    MIC_GAIN_ADAPTIVITY_STATE1 = 0,
    MIC_GAIN_ADAPTIVITY_STATE2,
    //MIC_GAIN_ADAPTIVITY_STATE3,
    //MIC_GAIN_ADAPTIVITY_STATE4,
    MIC_GAIN_ADAPTIVITY_STATE_NUM
};

typedef struct mic_gain_info
{
    int again;
    int dgain;

    int theshold_gain_min;
    int theshold_gain_max;

    int period_in_condition;
    //int timestamp_last_condition;
    unsigned long timestamp_last_condition;
} mic_gain_info_t;


/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/

int audio_agc_calculate_gain(int16_t *addr, int len);
void audio_gain_adaptivity(int mic_gain);
double audio_agc_calculate_gain_db(int16_t *addr, int len);


#ifdef __cplusplus
}
#endif

#endif /* __PREMIELINK_AUDIO_AGC_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/


