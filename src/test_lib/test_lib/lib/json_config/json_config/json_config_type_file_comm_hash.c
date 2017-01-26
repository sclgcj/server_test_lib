#include "json_config_type_private.h"
#include "json_config_type_file_private.h"
#include "json_config_comm_func_private.h"
#include "json_config_type_file_comm_hash_private.h"

#define JC_TYPE_FILE_COMM_HASH_SIZE 26

static int
jc_type_file_comm_init(
	struct jc_type_file_comm_hash *ch,
	struct json_config_comm *jcc
)
{
	int ret = 0;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_comm_node *fsn = NULL;

	jtp = (typeof(*jtp)*)jcc->module_private;
	jtfp = (typeof(*jtfp)*)jtp->sub_module_private;

	fsn = (typeof(*fsn)*)calloc(1, sizeof(*fsn) + ch->comm_node_size);
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
	if (ch->oper.comm_hash_init) {
		ret = ch->oper.comm_hash_init(jcc, fsn);
		if (ret != JC_OK) {
			if (fsn->var_name)
				free(fsn->var_name);
			if (fsn->lib)
				free(fsn->lib);
			free(fsn);
			return ret;
		}
	}

	return JC_OK;
}

static int
jc_type_file_comm_execute(
	struct jc_type_file_comm_hash *ch,
	struct json_config_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_comm_node *fsn = NULL;
	struct jc_type_file_comm_vuser_node *svn = NULL;
	struct jc_type_file_comm_var_node *svar = NULL;

	hnode = tc_hash_get(ch->vuser_hash, 
			    (unsigned long)jcc->id,
			    (unsigned long)jcc->id);
	if (!hnode) {
		fprintf(stderr, "no vuser whose id is %d\n", jcc->id);
		return JC_ERR;
	}
	svn = tc_list_entry(hnode, typeof(*svn), node);
	hnode = tc_hash_get(ch->var_hash, 
			    (unsigned long)jtp->node_name,
			    (unsigned long)jtp->node_name);
	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	if (ch->oper.comm_hash_execute) 
		ret = ch->oper.comm_hash_execute(jcc, fsn, svar, svn);
	return ret;

	/*svar->last_val = jc_file_val_get(
				fsn->col_num,
				1, 
				jtfp->comm->separate, 
			      	svar->cur_ptr,
			      	&svar->cur_ptr);
	if (!svar->last_val) 
		return JC_ERR;

out:
	if (svar->last_val)
		jcc->retval = strdup(svar->last_val);*/

	//return JC_OK;
}

static int
jc_type_file_comm_vuser_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int id = 0;
	struct jc_type_file_comm_vuser_node *svn = NULL;

	if (!hnode)
		id = (int)user_data;
	else {
		svn = tc_list_entry(hnode, typeof(*svn), node);
		id = svn->id;
	}

	return (id);
}

static int
jc_type_file_comm_vuser_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_file_comm_vuser_node *svn = NULL;

	svn = tc_list_entry(hnode, typeof(*svn), node);
	if (svn->id == (int)user_data)
		return JC_OK;

	return JC_ERR;
}

static int
jc_type_file_comm_vuser_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_comm_vuser_node *svn = NULL;

	svn = tc_list_entry(hnode, typeof(*svn), node);
	tc_hash_destroy(svn->var_spec_hash);
	free(svn->var_spec_hash);
	free(svn);

	return JC_OK;
}

static int
jc_type_file_comm_vuser_walk(
	unsigned long user_data,
	struct hlist_node *hnode,
	int *flag
)
{
	int ret = 0;
	struct jc_type_file_comm_hash *ch = NULL;
	struct jc_type_file_comm_node *fsn = NULL;
	struct jc_type_file_comm_vuser_node *vuser = NULL;
	struct jc_type_file_comm_var_node *svar = NULL;

	(*flag) = 0;
	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	vuser = (typeof(*vuser)*)user_data;
	ch = (typeof(*ch)*)vuser->comm_hash;
	svar = (typeof(*svar)*)calloc(1, sizeof(*svar) + ch->var_node_size);
	if (!svar) {
		fprintf(stderr, "calloc %d bytes error : %s\n", 
				sizeof(*svar), strerror(errno));
		exit(0);
	}
	svar->map_ptr = jc_file_mmap(fsn->lib, &fsn->file_size);
	svar->cur_ptr = svar->map_ptr;
	svar->var_name = fsn->var_name;
	svar->var_hash = vuser->comm_hash;
	if (ch->oper.comm_hash_copy) {
		ret = ch->oper.comm_hash_copy(svar, 
					      vuser);
		if (ret != JC_OK) {
			if (svar->map_ptr)
				jc_file_munmap(fsn->file_size, svar->map_ptr);
			free(svar);
			return JC_ERR;
		}
	}
	return tc_hash_add(vuser->var_spec_hash, 
			   &svar->node, 
			   (unsigned long)svar->var_name);
}

static int
jc_type_file_comm_var_spec_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_file_comm_var_node *svn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!name)
		name = 0;
	else {
		svn = tc_list_entry(hnode, typeof(*svn), node);
		if (svn->var_name)
			name = svn->var_name[0];
	}

	return (name % JC_TYPE_FILE_COMM_HASH_SIZE);
}

static int
jc_type_file_comm_var_spec_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_file_comm_var_node *svn = NULL;

	svn = tc_list_entry(hnode, typeof(*svn), node);
	if (!svn->var_name && !user_data)
		return JC_OK;
	if (!svn->var_name || !user_data)
		return JC_ERR;
	if (!strcmp(svn->var_name, (char*)user_data))
		return JC_OK;

	return JC_ERR;
}

static int
jc_type_file_comm_var_spec_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_comm_hash *ch = NULL;
	struct jc_type_file_comm_node *fsn = NULL;
	struct jc_type_file_comm_var_node *svn = NULL;

	svn = tc_list_entry(hnode, typeof(*svn), node);
	if (svn->last_val)
		free(svn->last_val);
	ch = (typeof(*ch)*)svn->var_hash;
	hnode = tc_hash_get(ch->var_hash, 
			    (unsigned long)svn->var_name, 
			    (unsigned long)svn->var_name);
	if (!hnode) {
		fprintf(stderr, "no variable named %s\n", svn->var_name);
		return JC_ERR;
	}
	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	if (svn->map_ptr)
		jc_file_munmap(fsn->file_size, svn->map_ptr);

	free(svn);

	return JC_OK;
}

static struct jc_type_file_comm_vuser_node *
jc_type_file_comm_vuser_create(
	int id,
	struct jc_type_file_comm_hash *ch
)
{
	struct jc_type_file_comm_vuser_node *svn = NULL;

	svn = (typeof(*svn)*)calloc(1, sizeof(*svn));
	if (!svn) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*svn), strerror(errno));
		exit(0);
	}
	svn->id = id;
	svn->comm_hash = (void*)ch;
	svn->var_spec_hash = tc_hash_create(
					JC_TYPE_FILE_COMM_HASH_SIZE, 
					jc_type_file_comm_var_spec_hash, 
					jc_type_file_comm_var_spec_hash_get, 
					jc_type_file_comm_var_spec_hash_destroy);
	if (svn->var_spec_hash == TC_HASH_ERR) {
		free(svn);
		return NULL;
	}
	tc_hash_traversal(
			(unsigned long)svn, 
			ch->var_hash, 
			jc_type_file_comm_vuser_walk);

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

	ch->vuser_hash = tc_hash_create(
				data_num, 
				jc_type_file_comm_vuser_hash,
				jc_type_file_comm_vuser_hash_get,
				jc_type_file_comm_vuser_hash_destroy);
	if (ch->vuser_hash == TC_HASH_ERR)
		return JC_ERR;

	ch->vuser_num = data_num;
	for (; i < data_num; i++) {

		svn = jc_type_file_comm_vuser_create(i, ch);
		if (!svn) 
			return JC_ERR;
		ret = tc_hash_add(
				ch->vuser_hash, 
				&svn->node, 
				(unsigned long)svn->id);
	}

}

int
json_config_type_file_comm_destroy(
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

static int
jc_type_file_comm_var_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_file_comm_node *fsn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		fsn = tc_list_entry(hnode, typeof(*fsn), node);
		if (fsn->var_name) 
			name = fsn->var_name[0];
	}

	return (name % JC_TYPE_FILE_COMM_HASH_SIZE);
}

static int
jc_type_file_comm_var_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_file_comm_node *fsn = NULL;

	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	if (!fsn->var_name && !user_data)
		return JC_OK;
	if (!fsn->var_name || !user_data)
		return JC_ERR;
	if (!strcmp(fsn->var_name, (char*)user_data))
		return JC_OK;

	return JC_ERR;
}

static int
jc_type_file_comm_var_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_comm_node *fsn = NULL;

	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	if (fsn->var_name)
		free(fsn->var_name);
	if (fsn->lib)
		free(fsn->var_name);
	free(fsn);

	return JC_OK;
}

int
json_config_type_file_comm_create(
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

	fch->var_hash =  jc_letter_create(NULL, NULL, NULL);
	fch->init = jc_type_file_comm_init;
	fch->copy = jc_type_file_comm_copy;
	fch->execute = jc_type_file_comm_execute;
	fch->var_node_size = var_node_size;
	fch->comm_node_size = comm_node_size;
	if (oper)
		memcpy(&fch->oper, oper, sizeof(*oper));

	return JC_OK;
}

