#include "jc_type_private.h"
#include "jc_type_file_private.h"
#include "jc_type_file_rand_private.h"

#define JC_TYPE_FILE_RAND "rand"

struct jc_type_file_rand {
	struct jc_type_file_manage *fm;
};

static struct jc_type_file_rand global_rand;

int
jc_type_file_rand_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper
)
{
	global_rand.fm->module_add(module, oper, global_rand.fm); 
}

static int
jc_type_file_rand_init(
	struct jc_comm *jcc
)
{
	global_rand.fm->init(global_rand.fm, jcc);
}

static int
jc_type_file_rand_copy(
	unsigned int data_num
)
{
	return global_rand.fm->copy(global_rand.fm, data_num);
}

static int
jc_type_file_rand_execute(
	struct jc_comm *jcc
)
{
	return global_rand.fm->execute(global_rand.fm, jcc);
}

int 
json_config_type_file_randuence_uninit()
{
	return jc_type_file_manage_destroy(global_rand.fm);
}

int 
json_config_type_file_randuence_init()
{
	struct jc_type_file_oper oper;

	global_rand.fm = jc_type_file_manage_create();
	oper.file_init = jc_type_file_rand_init;
	oper.file_copy = jc_type_file_rand_copy;
	oper.file_execute = jc_type_file_rand_execute;

	return jc_type_file_module_add(0, JC_TYPE_FILE_RAND, &oper);
}

