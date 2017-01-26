#include "json_config_type_private.h"
#include "json_config_type_file_private.h"
#include "json_config_comm_func_private.h"
#include "json_config_type_file_unique_private.h"
#include "tc_hash.h"

#include <pthread.h>

#define JC_TYPE_FILE_UNIQUE "unique"
#define JC_TYPE_FILE_UNIQUE_HASH_SIZE 26
#define JC_TYPE_FILE_UNIQUE_CALLOC_NUM_DEFAULT 1

/*
 * 这个单个文件模板
 */
struct jc_type_file_unique_node {
	char *var_name;			
	void *map_ptr;			//变量文件内容指针
	int  calloc_num;		//分配的个数
	int  col_num;			//指明数据在第几 
	int  file_size;                  //指明文件大小
	char *lib;                      //指明文件路径
	struct hlist_node node;
};

/*
 * 这个是每个文件模板具体的值
 */
struct jc_type_file_unique_var_node {
	int  cur_num;			//当前已经分配的个数
	void *cur_ptr;
	char *last_val;
	char *var_name;			//为了节省空间，直接引用的struct jc_type_file_unique_node中的var_name
	struct hlist_node node;
};

struct jc_type_file_unique {
	int  vuser_num;			 
	tc_hash_handle_t var_hash;
	tc_hash_handle_t vuser_hash;	
};

struct jc_type_file_seq_vuser_node {
	int id;
	tc_hash_handle_t var_spec_hash;
	struct hlist_node node;
};

static struct jc_type_file_unique global_unique;

static int
jc_type_file_unique_init(
	struct json_config_comm *jcc
)
{
	cJSON *obj = NULL;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_unique_node *fsn = NULL;

	jtp = (typeof(*jtp)*)jcc->module_private;
	jtfp = (typeof(*jtfp)*)jtp->sub_module_private;

	if (jtp->node_name)
		fsn->var_name = strdup(jtp->node_name);
	fsn->lib = strdup(jtfp->comm->path);
	fsn->col_num = jtfp->comm->col_num;
	fsn->map_ptr = jc_file_mmap(fsn->lib, &fsn->file_size);
	obj = cJSON_GetObjectItem(jcc->conf_val, "calloc_num");
	if (!obj) 
		fsn->calloc_num = JC_TYPE_FILE_UNIQUE_CALLOC_NUM_DEFAULT;
	else {
		if (obj->valuestring)
			fsn->calloc_num = atoi(obj->valuestring);
		else if (obj->valueint)
			fsn->calloc_num = obj->valueint;
		else 
			fsn->calloc_num = JC_TYPE_FILE_UNIQUE_CALLOC_NUM_DEFAULT;
	}
	return tc_hash_add(global_unique.var_hash,
			   &fsn->node, 
			   (unsigned long)fsn->var_name);
}

static int
jc_type_file_unique_execute(
	struct json_config_comm *jcc
)
{
	struct hlist_node *hnode = NULL;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_unique_node *fsn = NULL;
	struct jc_type_file_seq_vuser_node *svn = NULL;
	struct jc_type_file_unique_var_node *svar = NULL;

	hnode = tc_hash_get(global_unique.vuser_hash, 
			    (unsigned long)jcc->id,
			    (unsigned long)jcc->id);
	if (!hnode) {
		fprintf(stderr, "no vuser whose id is %d\n", jcc->id);
		return JC_ERR;
	}
	svn = tc_list_entry(hnode, typeof(*svn), node);
	if (jcc->getvalue == JC_IGNORE_VALUE) 
		goto out;
	jtp = (typeof(*jtp)*)jcc->module_private;
	jtfp = (typeof(*jtfp)*)jtp->sub_module_private;
	hnode = tc_hash_get(svn->var_spec_hash, 
			    (unsigned long)jtp->node_name, 
			    (unsigned long)jtp->node_name);
	if (!hnode) {
		fprintf(stderr, "no variable named %s\n", jtp->node_name);
		return JC_ERR;
	}
	svar = tc_list_entry(hnode, typeof(*svar), node);
	if (svar->last_val)	
		free(svar->last_val);
	hnode = tc_hash_get(global_unique.var_hash, 
			    (unsigned long)jtp->node_name,
			    (unsigned long)jtp->node_name);
	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	svar->last_val = jc_file_val_get(
				fsn->col_num,
				1, 
				jtfp->comm->separate, 
			      	svar->cur_ptr,
			      	&svar->cur_ptr);
	if (!svar->last_val) 
		return JC_ERR;

out:
	if (svar->last_val)
		jcc->retval = strdup(svar->last_val);

	return JC_OK;
}

static int
jc_type_file_unique_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int id = 0;
	struct jc_type_file_seq_vuser_node *svn = NULL;

	if (!hnode)
		id = (int)user_data;
	else {
		svn = tc_list_entry(hnode, typeof(*svn), node);
		id = svn->id;
	}

	return (id % global_unique.vuser_num);
}

static int
jc_type_file_unique_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_file_seq_vuser_node *svn = NULL;

	svn = tc_list_entry(hnode, typeof(*svn), node);
	if (svn->id == (int)user_data)
		return JC_OK;

	return JC_ERR;
}

static int
jc_type_file_unique_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_seq_vuser_node *svn = NULL;

	svn = tc_list_entry(hnode, typeof(*svn), node);
	tc_hash_destroy(svn->var_spec_hash);
	free(svn->var_spec_hash);
/*	if (svn->mmap)
		jc_file_munmap(global_unique.file_size, svn->mmap);
	if (svn->last_val)
		free(svn->last_val);*/
	free(svn);

	return JC_OK;
}

static int
jc_type_file_unique_vuser_walk(
	unsigned long user_data,
	struct hlist_node *hnode,
	int *flag
)
{
	struct jc_type_file_unique_node *fsn = NULL;
	struct jc_type_file_seq_vuser_node *vuser = NULL;
	struct jc_type_file_unique_var_node *svar = NULL;

	(*flag) = 0;
	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	vuser = (typeof(*vuser)*)user_data;
	svar = (typeof(*svar)*)calloc(1, sizeof(*svar));
	if (!svar) {
		fprintf(stderr, "calloc %d bytes error : %s\n", 
				sizeof(*svar), strerror(errno));
		exit(0);
	}
	svar->cur_ptr = fsn->map_ptr;
	svar->var_name = fsn->var_name;
	return tc_hash_add(vuser->var_spec_hash, 
			   &svar->node, 
			   (unsigned long)svar->var_name);
}

static int
jc_type_file_unique_var_spec_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_file_unique_var_node *svn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!name)
		name = 0;
	else {
		svn = tc_list_entry(hnode, typeof(*svn), node);
		if (svn->var_name)
			name = svn->var_name[0];
	}

	return (name % JC_TYPE_FILE_UNIQUE_HASH_SIZE);
}

static int
jc_type_file_unique_var_spec_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_file_unique_var_node *svn = NULL;

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
jc_type_file_unique_var_spec_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_unique_node *fsn = NULL;
	struct jc_type_file_unique_var_node *svn = NULL;

	svn = tc_list_entry(hnode, typeof(*svn), node);
	if (svn->last_val)
		free(svn->last_val);
	hnode = tc_hash_get(global_unique.var_hash, 
			    (unsigned long)svn->var_name, 
			    (unsigned long)svn->var_name);
	if (!hnode) {
		fprintf(stderr, "no variable named %s\n", svn->var_name);
		return JC_ERR;
	}
	fsn = tc_list_entry(hnode, typeof(*fsn), node);

	free(svn);

	return TC_OK;
}

static struct jc_type_file_seq_vuser_node *
jc_type_file_unique_vuser_create(
	int id	
)
{
	struct jc_type_file_seq_vuser_node *svn = NULL;

	svn = (typeof(*svn)*)calloc(1, sizeof(*svn));
	if (!svn) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*svn), strerror(errno));
		exit(0);
	}
	svn->id = id;
	svn->var_spec_hash = tc_hash_create(
					JC_TYPE_FILE_UNIQUE_HASH_SIZE, 
					jc_type_file_unique_var_spec_hash, 
					jc_type_file_unique_var_spec_hash_get, 
					jc_type_file_unique_var_spec_hash_destroy);
	if (svn->var_spec_hash == TC_HASH_ERR) {
		free(svn);
		return NULL;
	}
	tc_hash_traversal(
			(unsigned long)svn, 
			global_unique.var_hash, 
			jc_type_file_unique_vuser_walk);

	return svn;
}

static int
jc_type_file_unique_copy(
	unsigned int data_num
)
{
	int i = 0;
	int ret = 0;
	struct jc_type_file_seq_vuser_node *svn = NULL;

	global_unique.vuser_hash = tc_hash_create(
						data_num, 
						jc_type_file_unique_hash,
						jc_type_file_unique_hash_get,
						jc_type_file_unique_hash_destroy);
	if (global_unique.vuser_hash == TC_HASH_ERR)
		return JC_ERR;

	global_unique.vuser_num = data_num;
	for (; i < data_num; i++) {

		svn = jc_type_file_unique_vuser_create(i);
		if (!svn) 
			return JC_ERR;
		ret = tc_hash_add(
				global_unique.vuser_hash, 
				&svn->node, 
				(unsigned long)svn->id);
	}

}

int
json_config_type_file_unique_uninit()
{
	/*
	 * 顺序很重要，不要弄反了
	 */
	if (global_unique.vuser_hash) {
		tc_hash_destroy(global_unique.vuser_hash);
		free(global_unique.vuser_hash);
	}
	if (global_unique.var_hash) {
		tc_hash_destroy(global_unique.var_hash);
		free(global_unique.var_hash);
	}

	return JC_OK;
}

static int
jc_type_file_unique_var_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_file_unique_node *fsn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		fsn = tc_list_entry(hnode, typeof(*fsn), node);
		if (fsn->var_name) 
			name = fsn->var_name[0];
	}

	return (name % JC_TYPE_FILE_UNIQUE_HASH_SIZE);
}

static int
jc_type_file_unique_var_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_file_unique_node *fsn = NULL;

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
jc_type_file_unique_var_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_unique_node *fsn = NULL;

	fsn = tc_list_entry(hnode, typeof(*fsn), node);
	if (fsn->var_name)
		free(fsn->var_name);
	if (fsn->lib)
		free(fsn->var_name);
	if (fsn->map_ptr)
		jc_file_munmap(fsn->file_size, fsn->map_ptr);
	free(fsn);

	return JC_OK;
}

int
json_config_type_file_unique_init()
{
	struct jc_type_file_oper oper;	

	global_unique.var_hash = tc_hash_create(
						JC_TYPE_FILE_UNIQUE_HASH_SIZE, 
						jc_type_file_unique_var_hash, 
						jc_type_file_unique_var_hash_get,
						jc_type_file_unique_var_hash_destroy);

	memset(&oper, 0, sizeof(oper));
	oper.file_init = jc_type_file_unique_init;
	oper.file_execute = jc_type_file_unique_execute;
	oper.file_copy = jc_type_file_unique_copy;

	return jc_type_file_module_add(1, JC_TYPE_FILE_UNIQUE, &oper);
}

