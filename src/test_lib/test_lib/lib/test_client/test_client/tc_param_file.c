#include "tc_param_file.h"
#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_init_private.h"
#include "tc_print.h"

static int
tc_param_file_setup()
{
}

int
tc_param_file_uninit()
{
}

int
tc_param_file_init()
{
	int ret = 0;	
	
	ret = tc_local_init_register(tc_param_file_setup);
	if (ret != TC_OK) 
		TC_PANIC("tc_local_init_register: register tc_param_file_setup error");

	return tc_local_uninit_register(tc_param_file_uninit);
}

TC_MOD_INIT(tc_param_file_init);
