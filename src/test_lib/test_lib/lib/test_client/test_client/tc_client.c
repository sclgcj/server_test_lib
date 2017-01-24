#include "tc_std_comm.h"
#include "tc_cmd.h"
#include "tc_err.h"
#include "tc_init_private.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_private.h"
#include "tc_config_read.h"
#include "tc_epoll_private.h"

int 
main(
	int argc,
	char **argv
)
{
	int ret = 0;	
	char buf[1024] = { 0 };

	//init user cmd 
	ret = tc_user_cmd_init();
	if (ret != TC_OK)
		TC_PANIC("tc_cmd_init error: %s\n", TC_CUR_ERRMSG_GET());

	//handle console cmd
	ret = tc_cmd_handle(argc, argv);
	if (ret != TC_OK) 
		TC_PANIC("tc_cmd_handle error: %s\n", TC_CUR_ERRMSG_GET());

	//read config
	ret =tc_config_read_handle();
	if (ret != TC_OK)
		TC_PANIC("tc_config_read_handle error:%s\n", TC_CUR_ERRMSG_GET());

	//init lib module and user module
	ret = tc_init();
	if (ret != TC_OK) {
		PRINT("tc_init -errr: %s\n", TC_CUR_ERRMSG_GET());
	}
	
	//start epoll
	tc_epoll_start();

	tc_uninit();

	return 0;
}
