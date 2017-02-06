#include "jc_type_private.h"
#include "jc_type_file_private.h"
#include "jc_type_file_sequence_private.h"

#define JC_TYPE_FILE_SEQUENCE "sequence"

struct jc_type_file_seq {
	struct jc_type_file_manage *fm;
};

static struct jc_type_file_seq global_seq;

int
jc_type_file_seq_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper
)
{
	global_seq.fm->module_add(module, oper, global_seq.fm); }

static int
jc_type_file_seq_init(
	struct jc_comm *jcc
)
{
	global_seq.fm->init(global_seq.fm, jcc);
}

static int
jc_type_file_seq_copy(
	unsigned int data_num
)
{
	return global_seq.fm->copy(global_seq.fm, data_num);
}

static int
jc_type_file_seq_execute(
	struct jc_comm *jcc
)
{
	return global_seq.fm->execute(global_seq.fm, jcc);
}

int 
json_config_type_file_sequence_uninit()
{
	return jc_type_file_manage_destroy(global_seq.fm);
}

int 
json_config_type_file_sequence_init()
{
	struct jc_type_file_oper oper;

	global_seq.fm = jc_type_file_manage_create();
	oper.file_init = jc_type_file_seq_init;
	oper.file_copy = jc_type_file_seq_copy;
	oper.file_execute = jc_type_file_seq_execute;

	return jc_type_file_module_add(0, JC_TYPE_FILE_SEQUENCE, &oper);
}

