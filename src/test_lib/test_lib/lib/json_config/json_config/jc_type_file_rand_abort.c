#include "jc_comm_func_private.h"
#include "jc_type_file_rand_private.h"
#include "jc_type_file_comm_hash_private.h"
#include "jc_type_file_rand_abort_private.h"

#define JC_TYPE_FILE_RAND_ABORT "rand_in_a_abort" 

struct jc_type_file_rand_abort {
	struct jc_type_file_comm_hash *comm_hash;
};

static struct jc_type_file_rand_abort global_abort;

static int jc_type_file_rand_abort_init( 
	struct jc_comm *jcc
)
{
	return global_abort.comm_hash->init(global_abort.comm_hash, jcc);
}

static int
jc_type_file_rand_abort_execute(
	struct jc_comm *jcc
)
{
	return global_abort.comm_hash->execute(global_abort.comm_hash, jcc);
}

static int
jc_type_file_rand_abort_copy(
	unsigned int data_num
)
{
	return global_abort.comm_hash->copy(global_abort.comm_hash, data_num);
}

static  int
jc_type_file_rand_abort_comm_init(
	struct jc_type_file_comm_node *fcn
)
{
	srand(time(NULL));
	return JC_OK;
}

static int
jc_type_file_rand_abort_comm_copy(
	struct jc_type_file_comm_node *fcn,
	struct jc_type_file_comm_var_node *cvar
)
{
	return JC_OK;
}

static char *
jc_type_file_rand_abort_comm_execute(
	char separate,
	struct jc_type_file_comm_node *fsn,
	struct jc_type_file_comm_var_node *svar
)
{
	if (svar->last_val)
		free(svar->last_val);
	pthread_mutex_lock(&svar->mutex);
	svar->last_val = jc_file_val_get(fsn->col_num, 
					 random() % svar->line_num, 
				         separate, svar->cur_ptr, 
				         &svar->cur_ptr);
	pthread_mutex_unlock(&svar->mutex);

	return svar->last_val;
}

static int
jc_type_file_rand_abort_comm_destroy(
	struct jc_type_file_comm_node *fcn
)
{
}

static int
jc_type_file_rand_abort_comm_var_destroy(
	struct jc_type_file_comm_var_node *cvar
)
{
}

int
json_type_file_rand_abort_uninit()
{
	struct jc_type_file_manage_oper oper;
	struct jc_type_file_comm_hash_oper comm_oper;

	memset(&comm_oper, 0, sizeof(comm_oper));
	comm_oper.comm_hash_execute = jc_type_file_rand_abort_comm_execute;
	comm_oper.comm_hash_init    = jc_type_file_rand_abort_comm_init;
	comm_oper.comm_hash_copy    = jc_type_file_rand_abort_comm_copy;
	comm_oper.comm_node_destroy = jc_type_file_rand_abort_comm_destroy;
	comm_oper.comm_var_node_destroy = 
				      jc_type_file_rand_abort_comm_var_destroy;
	global_abort.comm_hash = 
			jc_type_file_comm_create(0, 0, &comm_oper);
	if (!global_abort.comm_hash)
		return JC_ERR;
	
	memset(&oper, 0, sizeof(oper));
	oper.manage_init = jc_type_file_rand_abort_init;
	oper.manage_copy = jc_type_file_rand_abort_copy;
	oper.manage_execute = jc_type_file_rand_abort_execute;

	return jc_type_file_rand_module_add(JC_TYPE_FILE_RAND_ABORT, &oper);
}

int
json_type_file_rand_abort_init()
{
	if (global_abort.comm_hash)
		return jc_type_file_comm_destroy(global_abort.comm_hash);

	return JC_OK;	
}

