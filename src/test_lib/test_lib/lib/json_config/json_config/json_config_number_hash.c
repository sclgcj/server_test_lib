#include "json_config_private.h"
#include "json_config_number_hash_private.h"
#include "tc_hash.h"

struct jc_number_node {
	int id;
	unsigned long user_data;
	void *jnumber;
	struct hlist_node node;
};

struct jc_number_hash_param {
	int id;
	int total;
	unsigned long user_data;
	int (*jn_walk)(unsigned long jn_data, unsigned long walk_data);
};

struct jc_number {
	int total;
	tc_hash_handle_t handle;
	int (*number_get)(unsigned long user_data, unsigned long hash_data);
	int (*number_destroy)(unsigned long user_data);
};

static int
jc_number_hash(
	struct hlist_node *hnode,
	unsigned long user_data
) 
{
	int id = 0;
	struct jc_number_node *jnn = NULL;
	struct jc_number_hash_param *param = NULL;

	param = (typeof(param))user_data;
	if (!hnode)
		id = (unsigned long)param->id;
	else {
		jnn = tc_list_entry(hnode, typeof(*jnn), node);
		id = jnn->id;
	}

	return (id % param->total);
}

static int
jc_number_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_number *jn = NULL;
	struct jc_number_node *jnn = NULL;
	struct jc_number_hash_param *param = NULL;

	param = (typeof(param))user_data;
	jnn = tc_list_entry(hnode, typeof(*jnn), node);
	if ((int)param->id != jnn->id) 
		return JC_ERR;
	jn = (typeof(jn))jnn->jnumber;
	if (jn->number_get) 
		return jn->number_get(jnn->user_data, param->user_data);

	return JC_OK;
}

static int
jc_number_hash_destroy(
	struct hlist_node *hnode
)
{
	int ret = 0;
	struct jc_number *jn = NULL;
	struct jc_number_node *jnn = NULL;

	jnn = tc_list_entry(hnode, typeof(*jnn), node);
	jn = (typeof(jn))jnn->jnumber;
	if (jn->number_destroy) 
		ret = jn->number_destroy(jnn->user_data);
	free(jn);
	return ret;
}

jc_number_t
json_config_number_hash_create(
	int total,
	int (*number_get)(unsigned long user_data, unsigned long cmp_data),
	int (*number_destroy)(unsigned long user_data)
)
{
	struct jc_number *jn = NULL;

	jn = (typeof(jn))calloc(1, sizeof(*jn));
	if (!jn) {
		fprintf(stderr, "calloc %d bytes error: %s\n", 
				sizeof(*jn), strerror(errno));
		exit(0);
	}
	jn->total = total;
	jn->number_get  = number_get;
	jn->number_destroy = number_destroy;

	jn->handle = tc_hash_create(
				jn->total, 
				jc_number_hash, 
				jc_number_hash_get,
				jc_number_hash_destroy);

	return (jc_number_t)jn;
}

int
jc_number_add(
	int id,	
	unsigned long user_data,
	jc_number_t jnumber
)
{
	struct jc_number *jn = NULL;
	struct jc_number_node *jnn = NULL;
	struct jc_number_hash_param param;

	jnn = (typeof(jnn))calloc(1, sizeof(*jnn));
	if (!jnn) {
		fprintf(stderr, "calloc %d bytes error: %s\n", 
				sizeof(*jnn), strerror(errno));
		exit(0);
	}
	jnn->jnumber = jnumber;
	jnn->id = id;
	jnn->user_data = user_data;

	jn = (typeof(jn))jnumber;
	memset(&param, 0, sizeof(param));
	param.id = id;
	param.user_data = 0;
	param.total = jn->total;
	
	return tc_hash_add(jn->handle,
			   &jnn->node,
			   (unsigned long)&param);
}

unsigned long
jc_number_get(
	int id,
	unsigned long cmp_data,
	jc_number_t jnumber
)
{
	struct jc_number *jn = NULL;	
	struct hlist_node *hnode = NULL;
	struct jc_number_node *jnn = NULL;
	struct jc_number_hash_param param;

	jn = (typeof(jn))jnumber;
	memset(&param, 0, sizeof(param));
	param.user_data = cmp_data;
	param.id = id;
	param.total = jn->total;

	hnode = tc_hash_get(jn->handle, 
			    (unsigned long)&param, 
			    (unsigned long)&param);
	if (!hnode) {
		fprintf(stderr, "no id = %d \n", id);
		return (unsigned long)JC_ERR;
	}
	jnn = tc_list_entry(hnode, typeof(*jnn), node);

	return jnn->user_data;
}

static int
jc_number_walk(
	unsigned long user_data,
	struct hlist_node *hnode,
	int *flag
)
{
	struct jc_number *jn = NULL;
	struct jc_number_node *jnn = NULL;
	struct jc_number_hash_param *param = NULL;

	(*flag) = 0;
	jnn = tc_list_entry(hnode, typeof(*jnn), node);
	jn = (typeof(jn))jnn->jnumber;
	param = (typeof(param))user_data;
	if (param->jn_walk)
		return param->jn_walk(jnn->user_data, 
					   param->user_data);
	return JC_OK;
}

int
jc_number_traversal(
	unsigned long user_data,
	jc_number_t jnumber,
	int (*jn_traversal)(unsigned long jn_data, unsigned long data)
)
{
	struct jc_number *jn = NULL;	
	struct jc_number_hash_param param;

	if (!jnumber) 
		return JC_ERR;

	jn = (typeof(jn))jnumber;
	memset(&param, 0, sizeof(param));
	param.jn_walk = jn_traversal;
	param.user_data = user_data;

	return tc_hash_traversal((unsigned long)&param,
				 jn->handle,
				 jc_number_walk);
}

int
json_config_number_hash_destroy(
	jc_number_t jnumber
)
{
	struct jc_number *jn = NULL;

	if (!jnumber)
		return JC_OK;

	if (jn->handle)  {
		tc_hash_destroy(jn->handle);
		free(jn->handle);
	}
	
	free(jnumber);
}

