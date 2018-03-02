/**
  ******************************************************************************
  * File Name          : led_cds.c
  * Description        : control led
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  *
  ******************************************************************************
  */

/*---------------------------------------------------------------------------*/
/*                           INCLUDED FILES                                  */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ak_apps_config.h"
#include "ringbuffer.h"
#include "spi_transfer.h"
#include "list.h"
#include "spi_cmd.h"
#include "doorbell_config.h"

#include "main_ctrl.h"
#include "ak_common.h"
#include <stdint.h>
#include <stdlib.h>

// #include "platform_devices.h"
#include "drv_gpio.h"
#include "led_cds.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/

static LED_STATE_t g_eStateLed = LED_STATE_OFF;  // 0:off 1: on
/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/
#define LIGHT_PWM
#define PWM_FREQ 30000
int pwm_value=10;
void led_init(void)
{
#ifdef LIGHT_PWM
    gpio_share_pin_set(ePIN_AS_PWM2, 0);
#else
    // g_led_gpio_info.nb = SYS_LED_PIN_CONTROL;
    // g_led_gpio_info.low = 1;
    // g_led_gpio_info.out = 1;
    
    if(gpio_set_pin_as_gpio(SYS_LED_PIN_CONTROL) == 1)
    {
        ak_print_normal("Pin 47 is as gpio pin!\r\n");
    }
    else
    {
        ak_print_normal("Pin 47 is not as gpio pin!\r\n");
    }

    gpio_set_pin_dir(SYS_LED_PIN_CONTROL, GPIO_DIR_OUTPUT);
    gpio_set_pin_level(SYS_LED_PIN_CONTROL, GPIO_LEVEL_LOW);  // pin 47 level 0
    
#endif
    // gpio_set_pin_level(SYS_LED_PIN_CONTROL, GPIO_LEVEL_HIGH);  // pin 47 level 0
    g_eStateLed = LED_STATE_OFF; // this pin is low, led off
    
}

void led_turn_on(void)
{
    ak_print_normal("Turn on LED\r\n");
#ifdef LIGHT_PWM
    pwm_set_duty_cycle(2,  PWM_FREQ,  pwm_value);  //2 is for gpio47.
#else
    gpio_set_pin_level(SYS_LED_PIN_CONTROL, GPIO_LEVEL_HIGH);
#endif
    g_eStateLed = LED_STATE_ON; // this pin is high, led on
}
void led_turn_off(void)
{
    ak_print_normal("Turn off LED\r\n");
#ifdef LIGHT_PWM
    pwm_set_duty_cycle(2,  PWM_FREQ,  0);  //2 is for gpio47.
#else
    gpio_set_pin_level(SYS_LED_PIN_CONTROL, GPIO_LEVEL_LOW);
#endif
    g_eStateLed = LED_STATE_OFF; // this pin is low, led off
}

void led_update(void)
{
if (g_eStateLed == LED_STATE_ON){
    ak_print_normal("Turn on LED\r\n");
#ifdef LIGHT_PWM
    pwm_set_duty_cycle(2,  PWM_FREQ,  pwm_value);  //2 is for gpio47.
#else
    gpio_set_pin_level(SYS_LED_PIN_CONTROL, GPIO_LEVEL_HIGH);
#endif
    }
}

LED_STATE_t led_get_state(void)
{
    return g_eStateLed;
}
/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/
