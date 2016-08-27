#include "tc_comm.h"
#include "tc_init.h"
#include "tc_err.h"
#include "tc_init_private.h"

#include <signal.h>

struct tc_signal_data {
	int signal;
	pthread_mutex_t mutex;
};

static struct tc_signal_data global_signal_data;

void
tc_signal_handle(
	int signo
)
{
	int ret = 0;

	if (tc_init_test() != TC_OK)
		return;

	pthread_mutex_lock(&global_signal_data.mutex);
	ret = global_signal_data.signal;
	global_signal_data.signal = 1;
	pthread_mutex_unlock(&global_signal_data.mutex);
	if (ret != 0)
		return ;

	tc_uninit();
}

static int
tc_signal_uninit()
{
	pthread_mutex_lock(&global_signal_data.mutex);
	global_signal_data.signal = 1;
	pthread_mutex_unlock(&global_signal_data.mutex);
}

int 
tc_signal_init()
{
	signal(SIGINT, tc_signal_handle);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, tc_signal_handle);

	return tc_uninit_register(tc_signal_uninit);
}

TC_MOD_INIT(tc_signal_init);
