/**
  ******************************************************************************
  * File Name          : audio_agc.c
  * Description        :
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 PREMIELINK PTE LTD
  *
  ******************************************************************************
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include "audio_agc.h"
#include "ak_apps_config.h"
#include <math.h>

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

#define S16_REF_POINT 32768


static char *agc_help[]={
	"agc on/off",
	"usage:\n"
};

int g_agc_enable = 1;


/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/

static mic_gain_info_t mic_adaptivity_table[MIC_GAIN_ADAPTIVITY_STATE_NUM] = 
{
    {MIC_AGAIN_DEFAULT, MIC_DGAIN_DEFAULT, 350, 30000, 0, 0},	// state active
    {0, 0, 0, 400,                                   0, 0}	// step 1 after 2s silent, mic 2
    //{0, 0, 200, 500,                                  0, 0},	// step 2 after 4s silent, mic 1
    //{0, 0, 0, 200,                                    0, 0}		// turn off mic (mic 0)
};

static int mic_gain_adap_state = MIC_GAIN_ADAPTIVITY_STATE1;
static int agc_debug = 0;

int set_agc_debug(int val)
{
    agc_debug = val;
}
int audio_gain_adap_set_info(int index, int mic_again, int mic_dgain, int min_threshold, int max_threshold)
{
    if(index>=MIC_GAIN_ADAPTIVITY_STATE_NUM || index<0)
        return -1;
    mic_adaptivity_table[index].again = mic_again;
    mic_adaptivity_table[index].dgain = mic_dgain;
    mic_adaptivity_table[index].theshold_gain_min = min_threshold;
    mic_adaptivity_table[index].theshold_gain_max = max_threshold;
    mic_adaptivity_table[index].period_in_condition = 0;
    mic_adaptivity_table[index].timestamp_last_condition = 0;
    printf("audio_gain_adap SET STATE %d: %d %d %d %d\n", index, mic_again, mic_dgain, min_threshold, max_threshold);
    return 0;
}
void audio_gain_adap_show_info()
{
    int index;
	
    printf("-----------------------------------------\n");
    printf("       AGAIN     DGAIN   MIN     MAX\n");
	
    for (index=0; index < MIC_GAIN_ADAPTIVITY_STATE_NUM; index++)
    {
        printf("STATE %d: %d        %d      %d      %d\n", index, mic_adaptivity_table[index].again, mic_adaptivity_table[index].dgain, 
                  mic_adaptivity_table[index].theshold_gain_min, mic_adaptivity_table[index].theshold_gain_max);
    }
    printf("-----------------------------------------\n");
}
int check_in_range(int val, int min, int max)
{
    return (val<min)?OUT_RANGE_LOWER:((val>max)?OUT_RANGE_HIGHER:IN_RANGE);
}
void audio_gain_adap_reset_state(int state)
{
    if(state<0 || state>MIC_GAIN_ADAPTIVITY_STATE_NUM)
        return;
    mic_adaptivity_table[state].period_in_condition = 0;
    mic_adaptivity_table[state].timestamp_last_condition = 0;
}
void audio_gain_adap_switch_state(int state)
{
    if(state<0 || state>MIC_GAIN_ADAPTIVITY_STATE_NUM)
        return;

    // audio_set_mic_again(mic_adaptivity_table[state].again);
    // audio_set_mic_dgain(mic_adaptivity_table[state].dgain);
    
    audio_set_volume_onflight_with_value(mic_adaptivity_table[state].again);
	//audio_set_volume_onflight_with_value(mic_adaptivity_table[state].dgain);
	
    mic_adaptivity_table[state].period_in_condition = 0;
    mic_adaptivity_table[state].timestamp_last_condition = 0;
}
void audio_gain_adaptivity(int mic_gain)
{
    int theshold_gain_min = mic_adaptivity_table[mic_gain_adap_state].theshold_gain_min;
    int theshold_gain_max = mic_adaptivity_table[mic_gain_adap_state].theshold_gain_max;
    unsigned long *period_in_condition = &(mic_adaptivity_table[mic_gain_adap_state].period_in_condition);
    unsigned long *timestamp_last_condition = &(mic_adaptivity_table[mic_gain_adap_state].timestamp_last_condition);
    int in_range_ret = check_in_range(mic_gain, theshold_gain_min, theshold_gain_max);
    if(agc_debug)
	{
        printf("OLD STATE: %d, period_in_condition %d, mic_gain %d, (%d-%d), range: %d\n", \
        	mic_gain_adap_state, *period_in_condition, mic_gain, theshold_gain_min, theshold_gain_max, in_range_ret);
    }

	switch(in_range_ret)
    {
        case IN_RANGE:
            audio_gain_adap_reset_state(mic_gain_adap_state);
            break;
			
        case OUT_RANGE_HIGHER:
            if(mic_gain_adap_state!=MIC_GAIN_ADAPTIVITY_STATE1)
            {
                audio_gain_adap_reset_state(mic_gain_adap_state);
                mic_gain_adap_state = MIC_GAIN_ADAPTIVITY_STATE1;
                audio_gain_adap_switch_state(mic_gain_adap_state);
				printf("OUT_RANGE_HIGHER switch back to STATE1 %d (%d)\n", mic_gain, mic_adaptivity_table[mic_gain_adap_state].again);
            }
            break;

		/* Need to debug this case. system crash at here */
        case OUT_RANGE_LOWER:
            if(*timestamp_last_condition==0)
            {
                // *timestamp_last_condition = xTaskGetTickCount();
                *timestamp_last_condition = get_tick_count();
            }
			else
            {
                // *period_in_condition += xTaskGetTickCount() - *timestamp_last_condition;
                *period_in_condition += get_tick_count() - *timestamp_last_condition;
                if(*period_in_condition>=AGC_ALGO_TIME)
                {
                    //printf("OLD STATE: %d, period_in_condition %d, mic_gain %d\n", mic_gain_adap_state, *period_in_condition, mic_gain);
                    audio_gain_adap_reset_state(mic_gain_adap_state);
                    mic_gain_adap_state = (mic_gain_adap_state+1<MIC_GAIN_ADAPTIVITY_STATE_NUM)?mic_gain_adap_state+1:mic_gain_adap_state;

                    //printf("NEW STATE: %d\n", mic_gain_adap_state);
                    audio_gain_adap_switch_state(mic_gain_adap_state);
					printf("NEW STATE %d (%d)\n", mic_gain_adap_state, mic_adaptivity_table[mic_gain_adap_state].again);

                }
				else
                {
                    // *timestamp_last_condition = xTaskGetTickCount();
                    *timestamp_last_condition = get_tick_count();
                }
            }
            break;
        
        default:
            break;
    }

}


int audio_agc_calculate_gain(int16_t *addr, int len)
{
        int sum = 0, i;
        int counter = 0;
        if(len%2 !=0)
                return -1;
        for(i=0; i<len/2; i++)
        {
            if(addr[i]>0)
            {
                sum += addr[i];
                counter++;
                //if(counter%64==0)
                //    printf("sample value %d\n", addr[i]);
            }
        }
        //printf("Counter %d- len %d\n", counter, len);
        return (sum/counter);
}




double audio_agc_calculate_gain_db(int16_t *addr, int len)
{
        int i = 0;
		double s_sample_dB = 0, averagedb = 0;
		short s_sample_ = 0;

		if(len <= 0)
		{
			return -1;
		}
		
        for(i = 0; i < len; i++)
        {
        	s_sample_ = *(addr + i);
			s_sample_dB  = 20.0f*log10((double) abs(s_sample_)/S16_REF_POINT);
			averagedb += s_sample_dB;
        }
        printf("averagedb %lf\n", averagedb);
        return (averagedb/len);
}


void * agc_test_on_fly(int argc, char **args)
{
	// toggle pin IRQ_ANA
	printf("%s start! argc: %d\r\n", __func__, argc);
	
    // check parameter and turn on /off pin IRQ_ANA
    if(strcmp("on",args[0])== 0)
	{
		g_agc_enable = 1;
		ak_print_error_ex("SET AGC on g_agc_enable:%d\r\n", g_agc_enable);
	}
	else if(strcmp("off",args[0])== 0)
	{
		g_agc_enable = 0;
		ak_print_error_ex("SET AGC off g_agc_enable:%d\r\n", g_agc_enable);
	}
	else
	{
		ak_print_error_ex("Command is invalid\r\n");
		ak_print_error_ex("Args[0]: %s\r\n", args[0]);
	}
	return NULL;
}

static int cmd_agc_reg(void)
{
    cmd_register("tagc", agc_test_on_fly, agc_help);
    return 0;
}

cmd_module_init(cmd_agc_reg)

/*
********************************************************************************
*                            END OF FILE                                       *
********************************************************************************
*/

