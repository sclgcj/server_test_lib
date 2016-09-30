#include "hc_comm.h"
#include "hc_init.h"

struct tc_init_module global_init = TC_INIT_MODULE(global_init);

int
hc_local_init_register(
	int (*init)()
)
{
	return global_init.local_init_register(&global_init, init);
}

int
hc_local_uninit_register(
	int (*uninit)()
)
{
	return global_init.local_uninit_register(&global_init, uninit);
}

int
hc_init_register(
	int (*init)()
)
{
	return global_init.other_init_register(&global_init, init);
}

int
hc_uninit_register(
	int (*uninit)()
)
{
	return global_init.other_uninit_register(&global_init, uninit);
}

int
hc_init()
{
	global_init.init_handle(&global_init);
}

int
hc_uninit()
{
	global_init.uninit_handle(&global_init);
}
