#include "jc_mode_private.h"
#include "jc_mode_once_private.h"
#include <pthread.h>

#define JC_MODE_ONCE "once"

struct jc_mode_once {
	pthread_mutex_t mutex;
	int once;
};

static struct jc_mode_once global_once;

static int 
jc_mode_once_init(
	struct jc_comm *jcc
)
{
	return JC_OK;	
}

static int
jc_mode_once_execute(
	struct jc_comm *jcc
)
{
	pthread_mutex_lock(&global_once.mutex);
	if (global_once.once == 0) {
		jcc->getvalue = JC_GET_VALUE;
		global_once.once = 1;
	} else 
		jcc->getvalue = JC_IGNORE_VALUE;
	pthread_mutex_unlock(&global_once.mutex);
	return JC_OK;
}

int
json_config_mode_once_uninit()
{
	return JC_OK;	
}

int
json_config_mode_once_init()
{
	struct json_mode_oper oper;

	global_once.once = 0;
	pthread_mutex_init(&global_once.mutex, NULL);

	memset(&oper, 0, sizeof(oper));
	oper.json_mode_init = jc_mode_once_init;
	oper.json_mode_execute = jc_mode_once_execute;

	return json_mode_module_add(JC_MODE_ONCE, &oper);
}

