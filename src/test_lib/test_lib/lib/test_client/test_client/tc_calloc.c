#include "tc_calloc_private.h"
#include "tc_comm.h"


static pthread_mutex_t global_calloc_mutex = PTHREAD_MUTEX_INITIALIZER;

void *
tc_calloc(
	int num,
	int memb_size
)
{
	void *data = NULL;

	pthread_mutex_lock(&global_calloc_mutex);
	data = calloc(num, memb_size);
	pthread_mutex_unlock(&global_calloc_mutex);

	return data;
}
