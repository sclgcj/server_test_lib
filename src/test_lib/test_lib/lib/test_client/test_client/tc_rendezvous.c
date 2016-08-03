#include "tc_rendezvous_private.h"
#include "tc_comm.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_print.h"

/*
 * The implemention of this module is ugly and there is many limitation in using it.
 * We want to modify it later, at present we just use it. (8.3)
 */

struct tc_rendezvous {
	int  count;
	int  total;
	char *name;
	pthread_cond_t  cond;
	pthread_mutex_t mutex;
};

tc_rendezvous_t
tc_rendezvous_create(
	int total_link,
	char *name
)
{
	int len = 0;
	struct tc_rendezvous *rend = NULL;

	rend = (struct tc_rendezvous *)calloc(1, sizeof(*rend));
	if (!rend) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_RENDEVOUS_ERR;
	}
	rend->total = total_link;
	if(name) {
		len = strlen(name);
		rend->name = (char*)calloc(1, len + 1);
		memcpy(rend->name, name, len);
	}
	pthread_cond_init(&rend->cond, NULL);
	pthread_mutex_init(&rend->mutex, NULL);

	return (tc_rendezvous_t)rend;
}

void
tc_rendezvous_destroy(
	tc_rendezvous_t handle
)
{
	struct tc_rendezvous *rend = NULL;	

	if (!handle)
		return;

	rend = (struct tc_rendezvous*)handle;
	if (rend->name)
		TC_FREE(rend);
	
	TC_FREE(rend);
}

int
tc_rendezvous_set(
	tc_rendezvous_t handle
)
{
	struct tc_rendezvous *rend = NULL;

	if (!handle) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	rend = (struct tc_rendezvous *)handle;
	pthread_mutex_lock(&rend->mutex);
	rend->count++;
	if (rend->count >= rend->total) {
		pthread_cond_broadcast(&rend->cond);
		rend->count = 0;
		goto out;
	}
	pthread_cond_wait(&rend->cond, &rend->mutex);

out:
	pthread_mutex_unlock(&rend->mutex);

	return TC_OK;
}
