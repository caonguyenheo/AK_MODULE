/**
*cmd_api.c   process cmd read write 
*/


#include "ak_drv_uart1.h"
#include "ak_drv_spi.h"
#include "ak_common.h"
#include "kernel.h"


#include "cmd_api.h"

int cmd_uart_init()
{ 
 	if(ak_drv_uart1_open(9600, 0) < 0)
	{
		ak_print_error("ak_drv_uart2_open fail\n");
        return 1;
	}
	return 0;
}


int cmd_uart_deinit()
{ 
 	ak_drv_uart1_close();
	return 0;
}


/*
*   1bytes         1byte        2bytes
*| preamble | cmd_id  | cmd_len |  cmd_data | 
*
*read 
*/

int read_cmd_preamble(char *buf, unsigned short len)
{
    int ret;

    if(ak_drv_uart1_read(buf, len, UNBLOCK)>0) {
        return buf[0];
    } else {
        ret = ak_drv_uart1_read(buf, len, BLOCKED);
    	if( ret > 0)
    	{
    		/*return first byte value*/
    		return buf[0];
    	}
    	else
    	{
    		ak_print_error("read cmd preamble error %d\n", ret);
    		return AK_ERROR;
    	}
    }

}

/*
*   1bytes         1byte        2bytes
*| preamble | cmd_id  | cmd_len |  cmd_data | 
*/
int read_cmd_len(char *buf, unsigned short len)
{
	unsigned short cmd_len;
	/*read cmd_id and cmd_len*/
	if(ak_drv_uart1_read(buf, len, UNBLOCK) >0)
	{
		cmd_len =  (*(unsigned short*)(buf + 1));
		return cmd_len;
	}
	else
	{
		ak_print_error("read cmd len error\n");
		return AK_ERROR;
	}
}

/*
*   1bytes         1byte        2bytes
*| preamble | cmd_id  | cmd_len |  cmd_data | 
*/

int read_cmd_data(char *buf, unsigned short len)
{
	/*read cmd data*/
	return ak_drv_uart1_read(buf, len, UNBLOCK); 
		
}

/*
*only read one cmd every time
*
*/
int read_one_cmd(char *buf, unsigned short len)
{
	unsigned char preamble;
	unsigned short cmd_len;
	int read_len;
    int get_len;
    do
	{
		preamble = read_cmd_preamble(buf, 1);/*keep read util read the cmd preamble*/
		ak_sleep_ms(20);
	}
	while(preamble != CMD_PREAMBLE);

    //ak_thread_sem_post(&sem_uart_cmd);  
    
	buf += 1;
	cmd_len = read_cmd_len(buf, 3);
	if( cmd_len > 0 )
	{
		if(cmd_len < len)
		{
			buf += 3;
			read_len = read_cmd_data(buf, cmd_len);
			if(read_len != cmd_len)
			{
				ak_print_error("cmd length should be %d, actually read %d\n", cmd_len, read_len);
				return AK_ERROR;
			}
			return AK_OK;
		}
		else
		{
			ak_print_error("cmd data length %d is greater than buf length %d \n", cmd_len, len);
			return AK_ERROR;
		}
	}
	else 
	{
		ak_print_error("read cmd len error\n");
		return AK_ERROR;
	}

		
}


/*
*send one cmd
* len should not greater 50 bytes because cc3200 reveive buf is 50bytes
*/
int send_one_cmd(char *buf, unsigned short len)
{
    //ak_thread_sem_wait(&sem_uart_cmd);  
    ak_print_info("send cmd id %d\n",buf[1]);
	return(ak_drv_uart1_write(buf, len));
}


