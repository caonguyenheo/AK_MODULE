/**
  ******************************************************************************
  * File Name          : ak_queue.c
  * Description        : This file contain functions for queue
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
#include "akos_api.h"
#include "AKerror.h"

#include "ak_queue.h"

/*---------------------------------------------------------------------------*/
/*                            DEFINITIONS                                    */
/*---------------------------------------------------------------------------*/
struct _msg_queue_mnt
{
    msgq_t msgq;
    void **q_start;
};

#define MSG_QUEUE_MAX                 20
#define assert(x)   \
do{ \
    if(!x){ \
        printf("%s:%d assert " #x "failed\r\n", __FILE__, __LINE__); \
        return -1; \
    } \
}while(0)
struct _msg_queue_mnt _queue_mnt[MSG_QUEUE_MAX];

/*---------------------------------------------------------------------------*/
/*                   FUNCTIONS/PROCEDURES                                    */
/*---------------------------------------------------------------------------*/
int ak_mq_get(msgq_t *pMsgQ, int queue_size)
{
    int ret = 0;
    int i = 0;
    unsigned int *pAddrStart = NULL;
    T_hQueue ak_ret_queue;

    // find in pool msg and allocate it
    for(i = 0; i < MSG_QUEUE_MAX; i++)
    {
        if(_queue_mnt[i].msgq == 0)
        {
            break;
        }
    }
    if(i >= MSG_QUEUE_MAX)
    {
        printf("%s no more message queue!\n", __func__);
        return -1;
    }
    pAddrStart = (unsigned int *)malloc(queue_size*sizeof(unsigned int *));
    if(pAddrStart == NULL)
    {
        printf("%s No memory to queue size: %d.\n", __func__, queue_size);
        return -2;
    }
    ak_ret_queue = AK_Create_Queue((void *)pAddrStart,
                            queue_size * sizeof(unsigned int *),
                            AK_FIXED_SIZE,
                            sizeof(unsigned int *),
                            AK_FIFO);
    if(AK_IS_INVALIDHANDLE(ak_ret_queue))
    {
        printf("%s Create queue error: %d!\n", __func__, ak_ret_queue);
        free(pAddrStart);
        return -3;
    }
    // fill info to management queue
    _queue_mnt[i].msgq = (msgq_t)ak_ret_queue;
    *pMsgQ = ak_ret_queue;
    _queue_mnt[i].q_start = (void **)pAddrStart;
    return ret;
}

int ak_mq_send(msgq_t msgq_id, void *msgbuf)
{
    int ret = 0;
    int status = AK_Send_To_Queue(msgq_id, &msgbuf,
                sizeof(unsigned int *), AK_SUSPEND/*AK_NO_SUSPEND*/);
	if(status != AK_SUCCESS)
    {
        if(status == AK_QUEUE_FULL){
			return 1;
        }
		printf("msgsnd: err %d", status);
		return -1;
	}
    return ret;
}

int ak_mq_recv(msgq_t msgq_id, void **msgbuf /*, unsigned int timeout*/)
{
	int status;
	unsigned int actual_size;

	assert(msgq_id);
	// timeout = sys_ms2ticks(timeout);
	// timeout hard code 19 ms
	status = AK_Receive_From_Queue(msgq_id, msgbuf, sizeof(unsigned int*), &actual_size, 10);
    if(status != AK_SUCCESS)
    {
        if(status == AK_TIMEOUT)
            return 1;
    
    printf("msgrcv: err %d", status);
    return  - 1;
    }
	return 0;
}

int ak_mq_free(msgq_t msgq_id)
{
	int status, i;

	for (i = 0; i < MSG_QUEUE_MAX; i++)
	{
		if (_queue_mnt[i].msgq == msgq_id)
			break;
	}
	if (i >= MSG_QUEUE_MAX)
	{
		printf("msgfree: err no match msgq_id %x", msgq_id);
		return  - 1;
	}

	if (!_queue_mnt[i].q_start)
	{
		assert(0);
	}

	free(_queue_mnt[i].q_start);
	_queue_mnt[i].q_start = 0;
	_queue_mnt[i].msgq = 0;
	
	status = AK_Delete_Queue(msgq_id);
    if(status != AK_SUCCESS)
    {
        printf("del: err %d", status);
    return  - 1;
    }
	return 0;
}

void ak_mq_init(void)
{
	int i;
	for (i = 0; i < MSG_QUEUE_MAX; i++)
	{
		_queue_mnt[i].msgq = 0;
		_queue_mnt[i].q_start = 0;
	}
}
/*---------------------------------------------------------------------------*/
/*                           END OF FILES                                    */
/*---------------------------------------------------------------------------*/

