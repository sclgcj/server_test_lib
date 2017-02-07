#include "jc_type_private.h"
#include "jc_type_file_private.h"
#include "jc_comm_func_private.h"
#include "jc_number_hash_private.h"
#include "jc_type_file_comm_hash_private.h"

#define JC_TYPE_FILE_COMM_HASH_SIZE 26

static int
jc_type_file_comm_init( 
	struct jc_type_file_comm_hash *ch,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_comm_node *fsn = NULL;

	jtp = (typeof(jtp))jcc->module_private;
	jtfp = (typeof(jtfp))jtp->sub_module_private;

	fsn = (typeof(fsn))calloc(1, sizeof(*fsn) + ch->comm_node_size);
	if (!fsn) {
		fprintf(stderr, "calloc %d bytes error: %s\n",
				sizeof(*fsn) + ch->comm_node_size, 
				strerror(errno));
		exit(0);
	}
	if (jtp->node_name)
		fsn->var_name = strdup(jtp->node_name);
	fsn->lib = strdup(jtfp->comm->path);
	fsn->col_num = jtfp->comm->col_num;
	fsn->map_ptr = jc_file_mmap(fsn->lib, &fsn->file_size);
	fsn->comm_hash = (void*)ch;
	if (ch->oper.comm_hash_init) {
		ret = ch->oper.comm_hash_init(fsn);
		if (ret != JC_OK) {
			if (fsn->var_name)
				free(fsn->var_name);
			if (fsn->lib)
				free(fsn->lib);
			free(fsn);
			return ret;
		}
	}

	return jc_letter_add(jtp->node_name, 
			     (unsigned long)fsn, 
			     ch->var_hash);
}

static int
jc_type_file_comm_execute(
	struct jc_type_file_comm_hash *ch,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_comm_node *fsn = NULL;
	struct jc_type_file_comm_vuser_node *svn = NULL;
	struct jc_type_file_comm_var_node *svar = NULL;
	struct jc_letter_param jl_param;
	
	svn = (typeof(svn))jc_number_get(jcc->id, 0, ch->vuser_hash);
	
	jtp = (typeof(jtp))jcc->module_private;
	jtfp = (typeof(jtfp))jtp->sub_module_private;
	memset(&jl_param, 0, sizeof(jl_param));
	jl_param.name = jtp->node_name;
	jl_param.user_data = 0;
	fsn = (typeof(fsn))jc_letter_get(&jl_param, ch->var_hash);
	if (!fsn) {
		fprintf(stderr, "no fsn named %s\n", fsn->var_name);
		return JC_ERR;
	}
	jl_param.name = fsn->var_name;
	jl_param.user_data = 0;
	svar = (typeof(svar))jc_letter_get(&jl_param, svn->var_spec_hash);

	if (ch->oper.comm_hash_execute) 
		jcc->retval = ch->oper.comm_hash_execute(jtfp->comm->separate,
							 fsn,
							 svar);
	if (!jcc->retval)
		return JC_ERR;
	return JC_OK;
}

static int
jc_type_file_comm_vuser_hash_destroy(
	unsigned long user_data
)
{
	struct jc_type_file_comm_vuser_node *svn = NULL;

	svn = (typeof(svn))user_data;
	jc_letter_destroy(svn->var_spec_hash);
	free(svn);

	return JC_OK;
}

static int
jc_type_file_comm_vuser_walk(
	unsigned long user_data,
	unsigned long walk_data
)
{
	int ret = 0;
	struct jc_type_file_comm_hash *ch = NULL;
	struct jc_type_file_comm_node *fsn = NULL;
	struct jc_type_file_comm_vuser_node *vuser = NULL;
	struct jc_type_file_comm_var_node *svar = NULL;

	fsn = (typeof(fsn))user_data;
	vuser = (typeof(vuser))walk_data;
	ch = (typeof(ch))vuser->comm_hash;
	svar = (typeof(*svar)*)calloc(1, sizeof(*svar) + ch->var_node_size);
	if (!svar) {
		fprintf(stderr, "calloc %d bytes error : %s\n", 
				sizeof(*svar), strerror(errno));
		exit(0);
	}
	svar->line_num = jc_file_line_num_get(fsn->lib);
	svar->map_ptr = fsn->map_ptr;
	svar->cur_ptr = svar->map_ptr;
	svar->var_name = fsn->var_name;
	svar->var_hash = vuser->comm_hash;
	pthread_mutex_init(&svar->mutex, NULL);
	if (ch->oper.comm_hash_copy) {
		ret = ch->oper.comm_hash_copy(fsn, svar);
		if (ret != JC_OK) {
			free(svar);
			return JC_ERR;
		}
	}
	return jc_letter_add(fsn->var_name, (unsigned long)svar, 
			     vuser->var_spec_hash);
}

static int
jc_type_file_comm_node_destroy(
	unsigned long user_data
)
{
	struct jc_type_file_comm_hash *fch = NULL;
	struct jc_type_file_comm_node *fcn = NULL;

	fcn = (typeof(fcn))user_data;

	if (fcn->var_name)
		free(fcn->var_name);
	if (fcn->lib)
		free(fcn->lib);
	fch = (typeof(fch))fcn->comm_hash;
	if (fch->oper.comm_node_destroy) 
		fch->oper.comm_node_destroy(fcn);
	if (fcn->map_ptr)
		jc_file_munmap(fcn->file_size, fcn->map_ptr);

	free(fch);

	return JC_OK;
}

static int
jc_type_file_comm_var_spec_hash_destroy(
	unsigned long user_data
)
{
	struct jc_type_file_comm_hash *ch = NULL;
	struct jc_type_file_comm_node *fsn = NULL;
	struct jc_type_file_comm_var_node *svn = NULL;

	svn = (typeof(svn))user_data;
	if (svn->last_val)
		free(svn->last_val);
	ch = (typeof(*ch)*)svn->var_hash;
	if (ch->oper.comm_var_node_destroy) 
		ch->oper.comm_var_node_destroy(svn);

	free(svn);

	return JC_OK;
}

static struct jc_type_file_comm_vuser_node *
jc_type_file_comm_vuser_create(
	int id,
	struct jc_type_file_comm_hash *ch
)
{
	int ret = 0;
	struct jc_type_file_comm_vuser_node *svn = NULL;

	svn = (typeof(*svn)*)calloc(1, sizeof(*svn));
	if (!svn) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*svn), strerror(errno));
		exit(0);
	}
	svn->comm_hash = (void*)ch;
	svn->var_spec_hash = jc_letter_create(NULL, NULL,
				jc_type_file_comm_var_spec_hash_destroy);
	ret = jc_letter_traversal((unsigned long)svn, 
			    ch->var_hash, 
			    jc_type_file_comm_vuser_walk);
	if (ret != JC_OK) {
		free(svn);
		svn = NULL;
	}
	return svn;
}

static int
jc_type_file_comm_copy(
	struct jc_type_file_comm_hash *ch,
	unsigned int data_num
)
{
	int i = 0;
	int ret = 0;
	struct jc_type_file_comm_vuser_node *svn = NULL;

	ch->vuser_hash = jc_number_create(data_num, NULL, 
					       jc_type_file_comm_vuser_hash_destroy);
	if (ch->vuser_hash == TC_HASH_ERR)
		return JC_ERR;

	ch->vuser_num = data_num;
	for (; i < data_num; i++) {
		svn = jc_type_file_comm_vuser_create(i, ch);
		if (!svn) {
			jc_number_destroy(ch->vuser_hash);
			return JC_ERR;
		}
		ret = jc_number_add(i, (unsigned long)svn, ch->vuser_hash);
		if (ret != JC_OK) {
			jc_number_destroy(ch->vuser_hash);
			free(svn);
			return JC_ERR;
		}
	}

	return JC_OK;
}

int
jc_type_file_comm_destroy(
	struct jc_type_file_comm_hash *ch
)
{
	/*
	 * 顺序很重要，不要弄反了
	 */
	if (ch->vuser_hash) {
		tc_hash_destroy(ch->vuser_hash);
		free(ch->vuser_hash);
	}
	if (ch->var_hash) {
		tc_hash_destroy(ch->var_hash);
		free(ch->var_hash);
	}

	return JC_OK;
}

struct jc_type_file_comm_hash *
jc_type_file_comm_create(
	int var_node_size,
	int comm_node_size,
	struct jc_type_file_comm_hash_oper *oper
)
{
	struct jc_type_file_comm_hash *fch = NULL;

	fch = (typeof(*fch)*)calloc(1, sizeof(*fch));
	if (!fch) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*fch), strerror(errno));
		exit(0);
	}

	fch->var_hash =  jc_letter_create(NULL, NULL, jc_type_file_comm_node_destroy);
	fch->init = jc_type_file_comm_init;
	fch->copy = jc_type_file_comm_copy;
	fch->execute = jc_type_file_comm_execute;
	fch->var_node_size = var_node_size;
	fch->comm_node_size = comm_node_size;
	if (oper)
		memcpy(&fch->oper, oper, sizeof(*oper));

	return fch;
}

