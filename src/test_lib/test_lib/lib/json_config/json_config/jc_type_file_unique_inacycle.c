#include "jc_comm_func_private.h"
#include "jc_type_private.h"
#include "jc_type_file_unique_private.h"
#include "jc_type_file_comm_hash_private.h"
#include "jc_type_file_unique_inacycle_private.h"

#define JC_TYPE_FILE_UNIQUE_INACYCLE "unique_in_a_cycle" 

struct jc_type_file_unique_var_data {
	int cur_num;
};

struct jc_type_file_unique_cycle {
	int alloc_num;
	int cur_id;
	struct jc_type_file_comm_hash *comm_hash;
};

static struct jc_type_file_unique_cycle global_cycle;

static int jc_type_file_unique_cycle_init( 
	struct jc_comm *jcc
)
{
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_unique_private *jtfup = NULL;	

	jtp = (typeof(jtp))jcc->module_private;
	jtfp = (typeof(jtfp))jtp->sub_module_private;
	jtfup = (typeof(jtfup))jtfp->private_data;
	global_cycle.alloc_num = jtfup->alloc_num;

	return global_cycle.comm_hash->init(global_cycle.comm_hash, jcc);
}

static int
jc_type_file_unique_cycle_execute(
	struct jc_comm *jcc
)
{
	return global_cycle.comm_hash->execute(global_cycle.comm_hash, jcc);
}

static int
jc_type_file_unique_cycle_copy(
	unsigned int data_num
)
{
	return global_cycle.comm_hash->copy(global_cycle.comm_hash, data_num);
}

static  int
jc_type_file_unique_cycle_comm_init(
	struct jc_type_file_comm_node *fcn
)
{
	return JC_OK;
}

static int
jc_type_file_unique_cycle_comm_copy(
	struct jc_type_file_comm_node *fcn,
	struct jc_type_file_comm_var_node *cvar
)
{
	int line = 0;
	struct jc_type_file_unique_var_data *var_data = NULL;

	var_data = (typeof(var_data))cvar->data;
	var_data->cur_num = 0;
	line = global_cycle.alloc_num * global_cycle.alloc_num, 
	cvar->map_ptr = jc_file_line_ptr_get(line, cvar->map_ptr);
	if (!cvar->map_ptr) {
		fprintf(stderr, "no map ptr at line %d\n", line);
		return JC_ERR;
	}
	cvar->cur_ptr = cvar->map_ptr;

	return JC_OK;
}

static char *
jc_type_file_unique_cycle_comm_execute(
	char separate,
	struct jc_type_file_comm_node *fsn,
	struct jc_type_file_comm_var_node *svar
)
{
	struct jc_type_file_unique_var_data *vdata = NULL;

	vdata = (typeof(vdata))svar->data;
	if (vdata->cur_num >= global_cycle.alloc_num) {
		svar->cur_ptr = svar->map_ptr;
		vdata->cur_num = 0;
	}
	vdata->cur_num++;
	pthread_mutex_lock(&svar->mutex);
	if (svar->last_val)
		free(svar->last_val);
	svar->last_val = jc_file_val_get(fsn->col_num, 0,
				         separate, svar->cur_ptr, 
				         &svar->cur_ptr);
	pthread_mutex_unlock(&svar->mutex);

	return svar->last_val;
}

int
json_type_file_unique_cycle_uninit()
{
	struct jc_type_file_manage_oper oper;
	struct jc_type_file_comm_hash_oper comm_oper;

	memset(&comm_oper, 0, sizeof(comm_oper));
	comm_oper.comm_hash_execute = jc_type_file_unique_cycle_comm_execute;
	comm_oper.comm_hash_init    = jc_type_file_unique_cycle_comm_init;
	comm_oper.comm_hash_copy    = jc_type_file_unique_cycle_comm_copy;
	global_cycle.comm_hash = 
			jc_type_file_comm_create(
					0,
					sizeof(struct jc_type_file_unique_var_data), 
					&comm_oper);
	if (!global_cycle.comm_hash)
		return JC_ERR;
	
	memset(&oper, 0, sizeof(oper));
	oper.manage_init = jc_type_file_unique_cycle_init;
	oper.manage_copy = jc_type_file_unique_cycle_copy;
	oper.manage_execute = jc_type_file_unique_cycle_execute;

	return jc_type_file_unique_module_add(JC_TYPE_FILE_UNIQUE_INACYCLE, &oper);
}

int
json_type_file_unique_cycle_init()
{
	if (global_cycle.comm_hash)
		return jc_type_file_comm_destroy(global_cycle.comm_hash);

	return JC_OK;	
}

