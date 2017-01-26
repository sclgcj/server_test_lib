#include "json_config_mode_private.h"
#include "json_config_mode_each_iteration_private.h"
#include "tc_hash.h"

#define JC_MODE_EI_VAR_SIZE 26

#define JC_MODE_EACH_ITERATION  "each iteration"

struct json_mode_each_iteration {
	pthread_mutex_t mutex;
	int cur_iteration;
};

static struct json_mode_each_iteration global_ei;

static int
jc_mode_each_iteration_init(
	struct json_config_comm *jcc
)
{
	global_ei.cur_iteration = jcc->iteration;	
	return JC_OK;
}

static int
jc_mode_each_iteration_execute(
	struct json_config_comm *jcc
)
{
	int ret = 0;

	pthread_mutex_lock(&global_ei.mutex);
	if (jcc->iteration == global_ei.cur_iteration) {
		jcc->getvalue = JC_IGNORE_VALUE;
		ret = JC_ERR;
	} else {
		global_ei.cur_iteration = jcc->iteration;
		jcc->getvalue = JC_GET_VALUE;
		ret = JC_OK;
	}
	pthread_mutex_unlock(&global_ei.mutex);

	if (jcc->mode_type)
		free(jcc->mode_type);

	jcc->mode_type = strdup(JC_MODE_EACH_ITERATION);

	return ret;
}

int
json_config_mode_each_iteration_init()
{
	int ret = 0;	
	struct json_mode_oper oper;

	pthread_mutex_init(&global_ei.mutex, NULL);

	memset(&oper, 0, sizeof(oper));
	oper.json_mode_init = jc_mode_each_iteration_init;
	oper.json_mode_execute = jc_mode_each_iteration_execute;

	return  json_mode_module_add(JC_MODE_EACH_ITERATION, &oper);
}

int
json_config_mode_each_iteration_uninit()
{
	return JC_OK;
}
