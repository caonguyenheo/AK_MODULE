/******************************************************
 * @brief  version  demo
 
 * @Copyright (C), 2016-2017, Anyka Tech. Co., Ltd.
 
 * @Modification  2017-8-17 
*******************************************************/
#include "command.h"
#include "ak_common.h"
#include "ak_ini.h"
#include "kernel.h"


/******************************************************
  *                    Constant         
  ******************************************************/
static char *help_ver[]={
	"version demo.",	
"     usage: version,  show all software lib version\n"
};

/******************************************************
  *                    Macro         
  ******************************************************/

/******************************************************
  *                    Type Definitions         
  ******************************************************/

/******************************************************
  *                    Global Variables         
  ******************************************************/

/******************************************************
*               Function prototype                           
******************************************************/


/******************************************************
*               Function Declarations
******************************************************/

/*****************************************
 * @brief start function for command
 * @param argc[in]  the count of command param
 * @param args[in]  the command param
 * @return void
 *****************************************/

extern char *ak_version();

static void cmd_ver_demo(int argc, char **args)
{
	ak_version();
}

/*****************************************
 * @brief register inidemo command
 * @param [void]  
 * @return 0
 *****************************************/
static int cmd_ver_demo_reg(void)
{
    cmd_register("version", cmd_ver_demo, help_ver);
    return 0;
}

cmd_module_init(cmd_ver_demo_reg)

