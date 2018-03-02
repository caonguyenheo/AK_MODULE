/*
* os level interface
*/
#include "drv_mhd_osl.h"

#include "drv_api.h"
#include "platform_devices.h"
#include "dev_drv.h"
#include "drv_gpio.h"

static struct thread_struct thread_list[MAX_TASK_NUM]={0};
static unsigned long m_id = 100;

#define TIMER_NUM_MAX	6
static osl_timer_type_t* TimeId_Handle[TIMER_NUM_MAX] = { 0 };

int osl_init_semaphore( osl_sem_t* semaphore )
{
#if 0
	osl_sem_t  handle = AK_Create_Semaphore(0, AK_PRIORITY);
	if (handle != AK_INVALID_SUSPEND)
	{
		*semaphore = handle;
		return OK; 
	}
	else
	{
		return ERR;
	}
#else
	return ak_thread_sem_init(semaphore, 0);
#endif
}

int osl_get_semaphore( osl_sem_t* semaphore, uint32_t timeout_ms, uint8_t will_set_in_isr )
{
	int ret;
	uint32_t wait_ms;

	if ( timeout_ms == NEVER_TIMEOUT )
    {
    	//ret = AK_Obtain_Semaphore(*semaphore , AK_SUSPEND);
		wait_ms = AK_SUSPEND;
	}
	else if ( timeout_ms == 0 )
	{
		//ret = AK_Obtain_Semaphore(*semaphore , AK_NO_SUSPEND);
		wait_ms = AK_NO_SUSPEND;
	}
	else
	{
#if 0
		//AK_Obtain_Semaphore is using tick to count, 1 tick = 5ms
		if (timeout_ms > 5)
			timeout_ms /= 5;
#else
		if (timeout_ms < 10)
			timeout_ms = 10;
		timeout_ms = (timeout_ms * 200) / 1000;
#endif
		//ret = AK_Obtain_Semaphore(*semaphore , timeout_ms);
		wait_ms = timeout_ms;
	}

	ret = AK_Obtain_Semaphore(*semaphore , timeout_ms);
	if (AK_SUCCESS == ret)
		return OK;
	else if (AK_TIMEOUT == ret)
		return TIMEOUT;
	else
		return ERR;
}

int osl_set_semaphore( osl_sem_t* semaphore, uint8_t called_from_ISR )
{
#if 0
	int ret;

	ret = AK_Release_Semaphore(*semaphore);
	if (AK_SUCCESS == ret)
		return 0;
	else
		return 1;
#else
	return ak_thread_sem_post(semaphore);
#endif
}

int osl_deinit_semaphore( osl_sem_t* semaphore )
{
#if 0
	int ret = AK_Delete_Semaphore(*semaphore);
	if (AK_SUCCESS == ret)
		return 0;
	else
		return 1;
#else
	return ak_thread_sem_destroy(semaphore);
#endif
}

int osl_init_mutex( osl_mutex_t* mutex )
{
	return ak_thread_mutex_init(mutex);
}

int osl_lock_mutex( osl_mutex_t* mutex )
{
	return ak_thread_mutex_lock(mutex);
}

int osl_unlock_mutex( osl_mutex_t* mutex )
{
	return ak_thread_mutex_unlock(mutex);
}

int osl_deinit_mutex( osl_mutex_t* mutex )
{
	return ak_thread_mutex_destroy(mutex);
}

uint32_t osl_get_time( void )
{
	return get_tick_count();
}

int osl_set_time( uint32_t time )
{
	//TODO:
	return OK;
}

int osl_delay_milliseconds( uint32_t num_ms )
{
	mini_delay(num_ms);
	return 0;
}

int osl_init_queue( osl_queue_type_t* queue, void* buffer, uint32_t buffer_size, uint32_t message_size )
{
	int result;
    uint32_t     message_num = buffer_size / message_size;

    queue->message_num  = message_num;
    queue->message_size = message_size;
    queue->buffer       = buffer;

    queue->push_pos = 0;
    queue->pop_pos  = 0;
	
    result = osl_init_semaphore( &queue->push_sem );
    if ( result != OK )
    {
        return result;
    }

    result = osl_init_semaphore( &queue->pop_sem );
    if ( result != OK )
    {
        if ( osl_deinit_semaphore( &queue->push_sem ) != OK )
        {
			printf("semaphore deinit failed\n");
        }
        return result;
    }
	
	queue->queueid = AK_Create_Queue((void *)buffer,
						buffer_size,
						AK_FIXED_SIZE,
						message_size,
						AK_FIFO);

	if (AK_IS_INVALIDHANDLE(queue->queueid))
	{
		printf("create queue err:%d", queue->queueid);
		return ERR;
	}
	
	return OK;
}

int osl_push_pop_queue( osl_queue_type_t* queue, void* message, uint32_t timeout_ms, uint8_t push  )
{
	int result;
	int status;
	
    if ( push == 1 )
    {
		int status = AK_Send_To_Queue(queue->queueid, message, 
					queue->message_size, timeout_ms /*AK_NO_SUSPEND*/);
		if (status != AK_SUCCESS)
		{
			if (status == AK_QUEUE_FULL)
				return TIMEOUT;
				
			printf("msgsnd: err %d", status);
				return ERR;
		}
		return  OK;
    }
    else
    {
		unsigned long actual_size;

		status = (int)AK_Receive_From_Queue(queue->queueid, message, queue->message_size, &actual_size, timeout_ms);
		if (status != AK_SUCCESS)
		{
			if (status == AK_TIMEOUT)
				return TIMEOUT;

			printf("msgrcv: err %d", status);
			return  ERR;
		}
		 return OK;
    }
}

int osl_deinit_queue( osl_queue_type_t* queue )
{
	int status = AK_Delete_Queue(queue->queueid);
	if(status != AK_SUCCESS)
	{
		printf("%s: del err %d", __func__, status);
		return  ERR;
	}

    return OK;
}

int osl_create_thread( osl_thread_type_t* thread, thread_handle_t entry_function, const char* name, void* stack, uint32_t stack_size, uint32_t priority, osl_thread_arg_t arg )
{
	//because ak_thread_create(...) not support name, so it still use older version.
    T_hTask handle;
	char thread_name[16];
	int i;
	int priority_map;
	osl_threadid_t thread_id;
	int result  = ERR;

    thread_id = ((unsigned long)m_id << 12) >> 12;
	sprintf(thread_name, "%s", name);
    m_id++;
    
    //check id exist or not
    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        if (thread_list[i].id == thread_id)
        {
			printf("same id\r\n");
            return result;
        }       
    }
    
    //find a place for task handle	
    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        if (thread_list[i].stack_addr == NULL)
            break;
    }
    if (i == MAX_TASK_NUM)
    {
		printf("err: MAX_TASK_NUM\r\n");
        return result;
    }

    //init task sem
    if (osl_init_semaphore(&thread_list[i].sem) != 0)
    {
		printf("err: sem\r\n");
        return result;
    }
   
    //malloc stack space
    thread_list[i].stack_addr = (void*)malloc(stack_size);
    if (thread_list[i].stack_addr == NULL)
    {
        osl_deinit_semaphore(&thread_list[i].sem);
		printf("err: malloc\r\n");
        return result;
    }

    priority_map = priority;

    //create task
	handle = AK_Create_Task(entry_function, thread_name, (unsigned long)arg, (void*)arg, 
	            thread_list[i].stack_addr, (unsigned long)stack_size, priority_map, 
	            1, AK_PREEMPT, AK_NO_START);
	if (AK_IS_INVALIDHANDLE(handle))
	{
	    free(thread_list[i].stack_addr);
		thread_list[i].stack_addr = NULL;
        osl_deinit_semaphore(&thread_list[i].sem);
		printf("err: AK_Create_Task\r\n");
	    return  result;
	}

    //record handle and then resume task
    thread_list[i].handle = handle;
    thread_list[i].id     = thread_id;

	thread->entry_function = entry_function;
    thread->arg            = arg;
	thread->id			   = thread_id;

	AK_Resume_Task(handle);
	result = OK;

	return result;
}

int osl_finish_thread( osl_thread_type_t* thread )
{
	T_hTask handle;
    int i;
#if 0
	handle = AK_GetCurrent_Task();

     //find by id
    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        if (thread_list[i].handle == handle)
            break;
    }
#else
	for (i=0; i<MAX_TASK_NUM; i++)
	{
		if (thread_list[i].id == thread->id)
			break;
	}
#endif
	
    if (i == MAX_TASK_NUM)
    {
		printf("%s: can't found task!\n", __FUNCTION__);
        return ERR;
    }

    //send exit signal
    osl_set_semaphore(&thread_list[i].sem, 0);
	
	return OK;
}

int osl_delete_terminated_thread( osl_thread_type_t* thread )
{
	int ret = OK;
	int i;
	
	 //find by id
	for (i=0; i<MAX_TASK_NUM; i++)
	{
		if (thread_list[i].id == thread->id)
			break;
	}
	if (i == MAX_TASK_NUM)
	{
		printf("%s: can't found task!\n", __FUNCTION__);
		return ERR;
	}
	//release resouce
	AK_Terminate_Task(thread_list[i].handle);
	if (AK_SUCCESS != (ret=AK_Delete_Task(thread_list[i].handle)))
	{
		printf("%s: can't delete task!\n", __FUNCTION__);
		ret = ERR;
	}
	free(thread_list[i].stack_addr);
	thread_list[i].stack_addr = NULL;
	osl_deinit_semaphore(&thread_list[i].sem);
	thread_list[i].id = 0;
	
    return ret;
}

int osl_join_thread( osl_thread_type_t* thread )
{
	int ret = OK;
	int i;

	 //find by id
	for (i=0; i<MAX_TASK_NUM; i++)
	{
		if (thread_list[i].id == thread->id)
			break;
	}
	if (i == MAX_TASK_NUM)
	{
		printf("%s: can't found task!\n", __FUNCTION__);
		return ERR;
	}

	//wait thread quit
	osl_get_semaphore(&thread_list[i].sem, NEVER_TIMEOUT, 0);
	
	return ret;
}

int osl_cancel_thread(osl_thread_type_t* thread)
{
	signed long ret=0;
    int i;
	
    //find by id
    for (i=0; i<MAX_TASK_NUM; i++)
    {
        if (thread_list[i].id == thread->id)
            break;
    }
    if (i == MAX_TASK_NUM)
    {
		printf("%s: can't found task!\n", __FUNCTION__);
        return ERR;
    }

    //terminate task
	if (AK_SUCCESS != AK_Terminate_Task(thread_list[i].handle))
		ret = ERR;

	if (AK_SUCCESS != AK_Delete_Task(thread_list[i].handle))
		ret = ERR;
		
    //release resouce
    free(thread_list[i].stack_addr);
	thread_list[i].stack_addr = NULL;
    host_rtos_deinit_semaphore(&thread_list[i].sem);
    thread_list[i].id = 0;

    return OK;
}

void timer_callback( signed long timer_id, unsigned long delay )
{
    //Because it need to pass something to callback function, but vtimer doesn't support it.
	osl_timer_type_t* timer = (osl_timer_type_t*)TimeId_Handle[timer_id];
	if ( timer == NULL )
		return;

    if ( timer->function )
    {
        timer->function( timer->arg );
    }
}

int osl_create_timer( osl_timer_type_t* timer, uint32_t time_ms, timer_handler_t function, void* arg )
{
    timer->function = function;
    timer->arg      = arg;

	timer->handle = vtimer_start(time_ms, 1, timer_callback);
	vtimer_pause(timer->handle);

	if (timer->handle < 0)
		return ERR;
	else
	{
		TimeId_Handle[timer->handle] = timer;
		return OK;
	}
}

int osl_start_timer( osl_timer_type_t* timer )
{
	vtimer_continue(timer->handle);
	return OK;
}

int osl_stop_timer( osl_timer_type_t* timer )
{	
	unsigned long total_delay = vtimer_get_time(timer->handle);
	vtimer_reset(timer->handle, total_delay);
	vtimer_pause(timer->handle);
	return OK;
}

int osl_delete_timer( osl_timer_type_t* timer )
{
	vtimer_stop(timer->handle);
	TimeId_Handle[timer->handle] = 0;
	return OK;
}

int osl_is_timer_running( osl_timer_type_t* timer )
{
	//TODO: How to implement?
	return OK;
}

int osl_sdio_init( uint8_t *bus_width )
{
	uint8_t width = USE_FOUR_BUS; //USE_ONE_BUS;
	uint8_t cif = 3;
	int fd;
	T_WIFI_INFO  *wifi =  NULL; //(T_WIFI_INFO *)wifi_dev.dev_data;


    fd = dev_open(DEV_WIFI);
    if(fd < 0)
    {
        printk("osl_sdio_init open wifi faile\r\n");
        while(1);
    }
    dev_read(fd,  &wifi, sizeof(unsigned int *));
	
	//printk("=====%d %d %d\r\n",wifi->sdio_share_pin,wifi->bus_mode,wifi->clock);

	switch (wifi->interface)
	{
		case 1:
			cif = INTERFACE_SDIO;
			break;
			
		case 0:
			cif = INTERFACE_SDMMC4;
			break;

		default:
			break;
	}
	
	if(1 == wifi->bus_mode)
	{
		width = USE_ONE_BUS;
	}

	if(4 == wifi->bus_mode)
	{
		width = USE_FOUR_BUS;
	}	

	//sdio_set_clock_api(wifi->clock);
	
	int ret = sdio_initial(cif, width , 512);//(3, width , 512)
	
	sdio_set_clock(wifi->clock, get_asic_freq(), 1); //SD_POWER_SAVE_ENABLE

	if (ret == 0)
	{
		printf(("sdio initial faild\n"));
		return ERR;
	}
	
	if (bus_width != NULL)
	{
		if (width == USE_ONE_BUS)
			*bus_width = 1;
		else if (width == USE_FOUR_BUS)
			*bus_width = 4;
		else
			*bus_width = 0;
	}
	dev_close(fd);
	return OK;
}

int osl_sdio_enable_high_speed( void )
{
	int ret;
	unsigned char speed;

	sdio_read_byte(0, SDIO_CCCR_SPEED, &speed);
	speed |= SDIO_SPEED_EHS;
	sdio_write_byte(0, SDIO_CCCR_SPEED, speed);

	return OK;
}

int osl_sdio_cmd52(sdio_transfer_direction_t direction, uint8_t func, uint32_t addr, uint8_t data, uint8_t *response)
{
	int ret;
	if (direction == SDIO_WRITE)
		ret = sdio_write_byte(func, addr, data);
	else
		ret = sdio_read_byte(func, addr, response);
	
	return ret;
}

int osl_sdio_cmd53(sdio_transfer_direction_t direction, uint8_t func, uint32_t addr, uint16_t data_size, uint8_t *data)
{
	int ret;
	if (direction == SDIO_WRITE)
		ret =  sdio_write_multi(func, addr, data_size, 1, data);
	else
		ret =  sdio_read_multi(func, addr, data_size, 1, data);

	return ret;
}
