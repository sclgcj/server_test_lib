#include "jc_type_private.h"
#include "jc_type_object_private.h"

#define JC_TYPE_OBJECT  "object"

/*
 * object 的主要目的就是帮助遍历所有的
 * json 节点，保留每个节点的配置信息，
 * 方便在实际执行的时候快速获取节点的
 * 值
 */
static cJSON *
jc_type_object_walk(
	struct jc_comm *jcc	
)
{
	cJSON *obj  = NULL; 
	cJSON *child = NULL;
	struct jc_type_private *jtp = NULL;

	if (!jcc->conf_val)
		return NULL;

	jtp = (struct jc_type_private *)jcc->module_private;
	child = jcc->conf_val->child;
	while (child) {
		jcc->out_data = jcc->walk_cb(jcc->id, jcc->depth + 1, jcc->out_data, child);
		child = child->next;
	}

	jcc->retval = (char*)jcc->out_data;
	return (cJSON*)jcc->out_data;
}

static int
jc_type_object_init(
	struct jc_comm *jcc
)
{
	jcc->type = JC_TYPE_OBJECT;
	jc_type_object_walk(jcc);

	//jcc->out_data = (unsigned long)tmp;

	return JC_OK;
}

static int
jc_type_object_execute(
	struct jc_comm *jcc
)
{
	cJSON *obj = NULL;

	jcc->type = JC_TYPE_OBJECT;
	jcc->out_data = (unsigned long)cJSON_CreateObject();
	obj = jc_type_object_walk(jcc);
	if (!obj)
		return JC_ERR;
	jcc->retval = (char *)obj;
	//jcc->out_data = (unsigned long)obj;
	return JC_OK;	
}

int
json_config_type_object_uninit()
{
	return JC_OK;
}

int
json_config_type_object_init()
{
	struct jc_type_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.jc_type_init    = jc_type_object_init;
	oper.jc_type_execute = jc_type_object_execute;

	return jc_type_module_add(JC_TYPE_OBJECT, &oper);
}
