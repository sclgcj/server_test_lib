#include "json_config_comm_func_private.h"
#include "json_config_type_file_sequence_private.h"
#include "json_config_type_file_comm_hash_private.h"
#include "json_config_type_file_sequence_continue_with_last_value_private.h"

#define JC_TYPE_FILE_SEQ_CWLV  "sequence_continue_with_last_value"

struct jc_type_file_seq_cwlv {
	struct jc_type_file_comm_hash *comm_hash;
};

static struct jc_type_file_seq_cwlv global_cwlv;

static int
jc_type_file_seq_cwlv_init(
	struct json_config_comm *jcc
)
{
	return global_cwlv.comm_hash->init(global_cwlv.comm_hash, jcc);
}

static int
jc_type_file_seq_cwlv_execute(
	struct json_config_comm *jcc
)
{
	return global_cwlv.comm_hash->execute(global_cwlv.comm_hash, jcc);
}

static int
jc_type_file_seq_cwlv_copy(
	unsigned int data_num
)
{
	return global_cwlv.comm_hash->copy(global_cwlv.comm_hash, data_num);
}

static int 
jc_type_file_seq_cwlv_comm_init(
	struct jc_type_file_comm_node *fsn
)
{
	return JC_OK;	
}

static char*
jc_type_file_seq_cwlv_comm_execute(
	char separate,
	struct jc_type_file_comm_node *fsn,
	struct jc_type_file_comm_var_node *svar
)
{	
	if (svar->last_val)
		free(svar->last_val);
	pthread_mutex_lock(&svar->mutex);
	svar->last_val = jc_file_val_get(fsn->col_num, 0, 
				      separate, svar->cur_ptr, 
				      &svar->cur_ptr);
	pthread_mutex_unlock(&svar->mutex);
	if (!svar->last_val) 
		fprintf(stderr, "no more data in line %d, col %d", 
				fsn->col_num, svar->line_num);

	return svar->last_val;
}

static int
jc_type_file_seq_cwlv_comm_copy(
	struct jc_type_file_comm_node *fsn,
	struct jc_type_file_comm_var_node *svar
)
{
	return JC_OK;
}

static int
jc_type_file_seq_cwlv_comm_destroy(
	struct jc_type_file_comm_node *fsn
)
{
	return JC_OK;
}

static int
jc_type_file_seq_cwlv_comm_var_destroy(
	struct jc_type_file_comm_var_node *svar
)
{
	return JC_OK;
}

int
json_config_type_file_seq_cwlv_init()
{
	struct jc_type_file_manage_oper oper;
	struct jc_type_file_comm_hash_oper comm_oper;

	memset(&comm_oper, 0, sizeof(comm_oper));
	comm_oper.comm_hash_copy = jc_type_file_seq_cwlv_comm_copy;
	comm_oper.comm_hash_init = jc_type_file_seq_cwlv_comm_init;
	comm_oper.comm_hash_execute = jc_type_file_seq_cwlv_comm_execute;
	comm_oper.comm_node_destroy = jc_type_file_seq_cwlv_comm_destroy;
	comm_oper.comm_var_node_destroy = jc_type_file_seq_cwlv_comm_var_destroy;
	global_cwlv.comm_hash = json_config_type_file_comm_create(
							0, 
							0,
							&comm_oper);
	if (!global_cwlv.comm_hash)
		return JC_ERR;

	memset(&oper, 0, sizeof(oper));
	oper.manage_execute = jc_type_file_seq_cwlv_execute;
	oper.manage_init = jc_type_file_seq_cwlv_init;
	oper.manage_copy = jc_type_file_seq_cwlv_copy;
	return jc_type_file_seq_module_add(JC_TYPE_FILE_SEQ_CWLV,&oper);
}

int
json_config_type_file_seq_cwlv_uninit()
{
	if (global_cwlv.comm_hash)
		json_config_type_file_comm_destroy(global_cwlv.comm_hash);

	return JC_OK;
}

