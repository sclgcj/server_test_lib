#include "jc_type_private.h"
#include "jc_type_file_private.h"
#include "jc_type_file_unique_private.h"

#define JC_TYPE_FILE_UNIQUE "unique"
#define JC_TYPE_FILE_UNIQUE_ALLOC  1

struct jc_type_file_unique {
	struct jc_type_file_unique_private up;
	struct jc_type_file_manage *fm;
};

static struct jc_type_file_unique global_unique;

int
jc_type_file_unique_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper
)
{
	global_unique.fm->module_add(module, oper, global_unique.fm); 
}

static int
jc_type_file_unique_init(
	struct jc_comm *jcc
)
{
	cJSON *obj = NULL;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;

	jtp = (typeof(jtp))jcc->module_private;
	jtfp = (typeof(jtfp))jtp->sub_module_private;

	obj = cJSON_GetObjectItem(jcc->conf_val, "alloc_num");
	if (!obj) 
		global_unique.up.alloc_num = JC_TYPE_FILE_UNIQUE_ALLOC;	
	else {
		if (obj->valuestring)
			global_unique.up.alloc_num = atoi(obj->valuestring);
		else if (obj->valueint == 0)
			global_unique.up.alloc_num = JC_TYPE_FILE_UNIQUE_ALLOC;
		else
			global_unique.up.alloc_num = obj->valueint;
	}
	jtfp->private_data = (unsigned long)&global_unique.up;
	global_unique.fm->init(global_unique.fm, jcc);
}

static int
jc_type_file_unique_copy(
	unsigned int data_num
)
{
	return global_unique.fm->copy(global_unique.fm, data_num);
}

static int
jc_type_file_unique_execute(
	struct jc_comm *jcc
)
{
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;

	jtp = (typeof(jtp))jcc->module_private;
	jtfp = (typeof(jtfp))jtp->sub_module_private;
	jtfp->private_data = (unsigned long)&global_unique.up;

	return global_unique.fm->execute(global_unique.fm, jcc);
}

int 
json_config_type_file_uniqueuence_uninit()
{
	return jc_type_file_manage_destroy(global_unique.fm);
}

int 
json_config_type_file_uniqueuence_init()
{
	struct jc_type_file_oper oper;

	global_unique.fm = jc_type_file_manage_create();
	oper.file_init = jc_type_file_unique_init;
	oper.file_copy = jc_type_file_unique_copy;
	oper.file_execute = jc_type_file_unique_execute;

	return jc_type_file_module_add(0, JC_TYPE_FILE_UNIQUE, &oper);
}

