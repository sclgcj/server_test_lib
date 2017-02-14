#include "jc_private.h"
#include "jc_iteration_ctrl_private.h"
#include "jc_iteration_private.h"

#define JC_ITERATION_DEFAULT	1
#define JC_ITERATION		"iteration"	
#define JC_ITERATION_LEVEL	1

struct jc_iteration {
	int total_iteration;
	int (*iteration_get)(unsigned long user_data);
};

static struct jc_iteration global_iteration;

static int
jc_iteration_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	/*
	 * 获取当前迭代数, 这里需要外部程序提供一个可获取
	 * 迭代数的方法。
	 */
	jcc->iteration = jc_iteration_ctrl_get();
	if (obj->valuestring)
		global_iteration.total_iteration = atoi(obj->valuestring);
	else if (obj->valueint)
		global_iteration.total_iteration = obj->valueint;
	else 
		global_iteration.total_iteration = (int)obj->valuedouble;

	return JC_OK;
}

static int
jc_iteration_init_default(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	return jc_iteration_init(node_name, obj, user_data, jcc);
}

static int
jc_iteration_execute(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	jcc->iteration = jc_iteration_ctrl_get();
	if (jcc->iteration > global_iteration.total_iteration)	
		jcc->end = 1;

	return JC_ERR;
}

static int
jc_iteration_exe_destult(
	char *node_name, 
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	return jc_iteration_execute(node_name, user_data, jcc);
}

static int
jc_iteration_func_add(
	int level,
	int (*func)(unsigned long user_data)
)
{
	global_iteration.iteration_get = func;

	return JC_OK;
}

int
json_config_iteration_init()
{
	struct jc_oper oper;

	memset(&global_iteration, 0, sizeof(global_iteration));
	oper.jc_init		= jc_iteration_init;
	oper.jc_func_add	= jc_iteration_func_add;
	oper.jc_execute		= jc_iteration_execute;
	oper.jc_init_default	= jc_iteration_init_default;
	oper.jc_execute_default = jc_iteration_exe_destult;

	return jc_module_add(JC_ITERATION, JC_ITERATION_LEVEL, &oper);
}

int
json_config_iteration_uninit()
{
	return JC_OK;
}

