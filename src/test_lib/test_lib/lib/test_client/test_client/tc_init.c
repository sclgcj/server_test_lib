#include "tc_std_comm.h"
#include "tc_init.h"
#include "tc_init_module.h"

struct tc_init_module global_init = TC_INIT_MODULE(global_init);

int
tc_init_test()
{
	return global_init.init_test(&global_init);
}

int
tc_local_init_register(
	int (*init)()
)
{
	return global_init.local_init_register(&global_init, init);
}

int
tc_local_uninit_register(
	int (*uninit)()
)
{
	return global_init.local_uninit_register(&global_init, uninit);
}

int
tc_init_register(
	int (*init)()
)
{
	return global_init.other_init_register(&global_init, init);
}

int
tc_uninit_register(
	int (*uninit)()
)
{
	return global_init.other_uninit_register(&global_init, uninit);
}

int
tc_init()
{
	global_init.init_handle(&global_init);
}

int
tc_uninit()
{
	global_init.uninit_handle(&global_init);
}
