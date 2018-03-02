
/**
 * @file  ak_drv_vtimer.h
 * @brief: Implement  operations of how to use vtimer device.
 *
 * Copyright (C) 2016 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author  
 * @date    2011-11-10
 * @version 1.0
 */

#ifndef __AK_VTIMER_H__
#define __AK_VTIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief: Timer Callback type define.
 */
typedef void (*T_fAK_VTIMER_CALLBACK)(long timer_id, unsigned long delay);

/**
 * @brief open timer
 * When the time reach, the vtimer callback function will be called. User must call function
 * @author 
 * @date 2017-11-10
 * @param unsigned long milli_sec: Specifies the time-out value, in millisecond. Caution, this value must can be divided by 5.
 * @param unsigned char loop: loop or not;   1:loop
 * @param T_fVTIMER_CALLBACK callback_func: Timer callback function. If callback_func is
 *                              not NULL, then this callback function will be called when time reach.
 * @return signed long: timer ID, user can stop the timer by this ID
 * @retval -1: failed
 */
long ak_drv_vtimer_open(unsigned long milli_sec, unsigned char loop, T_fAK_VTIMER_CALLBACK callback_func);


/**
 * @brief close timer
 * Function ak_drv_vtimer_open() must be called before call this function
 * @author  
 * @date 2017-11-10
 * @param timer_id :  Timer ID
 * @return void
 */
void ak_drv_vtimer_close(long timer_id);


/**
 * @brief reset timer
 * @author 
 * @date 2017-11-10
 * @param timer_id :  Timer ID
 * @param unsigned long milli_sec: Specifies the time-out value, in millisecond. Caution, this value must can be divided by 5.
 * @return signed int: 
 * @retval -1: failed
 *              0: OK
 */
int  ak_drv_vtimer_reset( long timer_id, unsigned int milli_sec);


/**
 * @brief get timer
 * Function ak_drv_vtimer_open() must be called before call this function
 * @author 
 * @date 2017-11-10
 * @param timer_id :  Timer ID
 * @param type: ~0:  get timer current delay
 * @                   0:   get timer total delay
 * @return unsigned long
 * @retval timer delay
 */
unsigned long  ak_drv_vtimer_get_time(long timer_id, unsigned char type);




/*@}*/
#ifdef __cplusplus
}
#endif
#endif //#ifndef __AK_VTIMER_H__

