#include "command.h"
#include "ak_drv_key.h"
#include "ak_common.h"
#include "kernel.h"

volatile static bool key_break = false;
static void cmd_keypad_signal(int signo)
{
    key_break = true;
}

static char* help[]={
	"keypaddemo module",
	"usage:keypaddemo [type] [time]\n"
	"      type: 1:unblock, 2:wait_time(ms), 3:block\n"
	"      time: time_out(s)\n"
	"      note: in unblock mode setting a time,just in order to test,\n"
	"            if you chose wait_time mode,during the time,if detect key\n"
	"            been pressed,it will return immediately,if not,it will wait\n"
	"            timeout!\n"
	"            if you entry block type,it will not leave until you reset!\n"
	"      example:keypaddemo 2 3 \n"
	
};

void cmd_keypad_test(int argc, char **args)
{

	char * ts = args[0];
	if((argc < 1) || (argc > 2) || (*ts < '1') || (*ts > '3') || (atoi(args[1]) <= 0))
	{
		ak_print_error_ex("Input command error:\n");		
		ak_print_normal("%s\n",help[1]);
		return ;
	}
	long tmp = 0;
	void * key = NULL;
	struct key_event event = {0};	
	int ms = atoi(args[1])*1000;

	key = ak_drv_key_open();
	if(NULL == key)
	{	
		ak_print_error("key open fail!\n\n\n\n");
		return ;
	}
	
    key_break = false;
    cmd_signal(CMD_SIGTERM, cmd_keypad_signal);
	switch (*ts)
	{
		/*
			������ģʽ���������ʱ����Ϊ�˱��ڲ��ԣ�
			ʵ��Ӧ�õ��У�������û�а������£�ɨ��
			һ�鶼�������˳�������ȴ�
		*/
		case '1':
			while(ms && (!key_break))
			{
				if (ak_drv_key_get_event(key,&event,0) == 0)
				{	
					if(event.stat == 0)
					ak_print_normal("key_id = %d  PRESS\n",event.code);
					else if(event.stat == 2)
					ak_print_normal("key_id = %d  RELEASE\n",event.code);
				}
				else
				{	
					if(500 < ms)
					{
						mini_delay(500);
						ms = ms - 500;
						
					}
					else
					{
						mini_delay(ms);
						ms = 0;				
						ak_print_normal("timeout!\n");
					}					
				}						
			}
			break;
		/*
			�ȴ�ģʽ�����ڵȴ���ʱ���ڣ��а������£���ȡ����
			���Ϸ����˳�������ڵȴ���ʱ���ڶ����а������£�
			����ڵȴ���ʱ����ɺ��˳�
			
		*/	
		case '2':
			while(1)
			{
				if (ak_drv_key_get_event(key,&event,ms) == 0)
				{	
					if(event.stat == 0)
					ak_print_normal("key_id = %d  PRESS\n",event.code);
					else if(event.stat == 2)
					ak_print_normal("key_id = %d  RELEASE\n",event.code);
					break;
				}
				else if (ak_drv_key_get_event(key,&event,ms) == -1)
				{				
					ak_print_normal("timeout!\n");
					break;
				}						
			}
			break;
		/*
			����ģʽ��������û�а������£�������һֱѭ����
			�����Զ��˳���Ҫ��λ�����˳�		
			
		*/		
		case '3':
			while((!key_break))
			{
				if (ak_drv_key_get_event(key,&event,-1) == 0)
				{	
					if(event.stat == 0)
					ak_print_normal("key_id = %d  PRESS\n",event.code);
					else if(event.stat == 2)
					ak_print_normal("key_id = %d  RELEASE\n",event.code);
				}	
			}
			break;
			
		default:
			break;		

	}
		
	ak_drv_key_close(key);

}


static int cmd_gpio_reg(void)
{
	cmd_register("keypaddemo", cmd_keypad_test, help);
    return 0;
}

cmd_module_init(cmd_gpio_reg)

