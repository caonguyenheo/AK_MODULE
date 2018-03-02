
#ifndef __DRV_MHD_OSL_H__
#define __DRV_MHD_OSL_H__

#include "akos_api.h"

typedef unsigned int	uint32_t;
typedef unsigned short	uint16_t;
typedef unsigned char	uint8_t;

typedef long        	osl_threadid_t;
typedef long        	osl_mutex_t;
typedef long        	osl_sem_t;

struct thread_struct {
    T_hTask 		handle;
    osl_threadid_t  id;
    void 			*stack_addr;
    osl_sem_t 		sem;
};

typedef uint32_t osl_thread_arg_t;
typedef void(*thread_handle_t)( osl_thread_arg_t );

typedef struct
{
    osl_threadid_t				id;
    osl_thread_arg_t  		arg;
	thread_handle_t 		entry_function;
} osl_thread_type_t;

typedef struct
{
    uint32_t              message_num;
    uint32_t              message_size;
    uint8_t*              buffer;
    uint32_t              push_pos;
    uint32_t              pop_pos;
    osl_sem_t   		  push_sem;
    osl_sem_t   		  pop_sem;
    uint32_t              occupancy;
	long				  queueid;
} osl_queue_type_t;

typedef void (*timer_handler_t)(void *arg);

typedef struct
{
	T_hTimer		handle;
    timer_handler_t function;
    void*           arg;
} osl_timer_type_t;

typedef enum
{
    SDIO_READ,
    SDIO_WRITE
} sdio_transfer_direction_t;

#define NEVER_TIMEOUT ((uint32_t)0xffffffffUL)
#define OK			0
#define ERR			-1
#define TIMEOUT		2

#define MAX_TASK_NUM    (128)


#define  SDIO_CCCR_SPEED	0x13
#define  SDIO_SPEED_SHS		0x01	/* Supports High-Speed mode */
#define  SDIO_SPEED_EHS		0x02	/* Enable High-Speed mode */



#endif
