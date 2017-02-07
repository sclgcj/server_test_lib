#include "jc_var_hash_private.h"
#include "jc_letter_hash_private.h"

struct jc_var_hash_node {
	int depth;
	jc_letter_t var;
	unsigned long user_data;
};

struct jc_var_hash_param {
	int depth;
	unsigned long data;
};

struct jc_var_traversal_param {
	jc_variable_t var;
	unsigned long walk_data;
	int (*jv_traversal)(unsigned long jv_data, 
			    unsigned long walk_data);
};

struct jc_variable {
	jc_letter_t letter;
	int (*var_get)(unsigned long user_data, 
		       unsigned long cmp_data);
	int (*var_destroy)(unsigned long user_data);
};

static int
jc_variable_node_destroy(
	unsigned long user_data
)
{
	int ret = 0;
	struct jc_variable *var = NULL;
	struct jc_var_hash_node *hn = NULL;

	hn = (typeof(hn))user_data;
	var = (typeof(var))hn->var;	

	if (var->var_destroy) 
		ret = var->var_destroy(hn->user_data);
	free(hn);

	return ret;
}

static int
jc_variable_node_get(
	unsigned long user_data,
	unsigned long cmp_data
)
{
	struct jc_variable *var = NULL;
	struct jc_var_hash_node *hn = NULL;
	struct jc_var_hash_param *param = NULL;

	hn = (typeof(hn))user_data;
	param = (typeof(param))cmp_data;
	if (param->depth != hn->depth)
		return JC_ERR;
	var = (typeof(var))hn->var;
	if (var->var_get) 
		return var->var_get(hn->user_data, param->data);

	return JC_OK;
}

jc_variable_t
jc_variable_create(
	int (*var_get)(unsigned long user_data, 
		       unsigned long cmp_data),
	int (*var_destroy)(unsigned long user_data)
)
{
	struct jc_variable *jv = NULL;

	jv = (typeof(jv))calloc(1, sizeof(*jv));
	if (!jv) {
		fprintf(stderr, "calloc %d bytes error : %s\n",
				 sizeof(*jv), strerror(errno));
		return NULL;
	}
	jv->letter = jc_letter_create(NULL, 
				      jc_variable_node_get, 
				      jc_variable_node_destroy);
	if (!jv->letter)  {
		free(jv);
		return NULL;
	}
	jv->var_get = var_get;
	jv->var_destroy = var_destroy;

	return jv;
}

int
jc_variable_destroy(
	jc_variable_t var
)
{
	int ret = 0;
	struct jc_variable *jv = NULL;

	var = (typeof(jv))var;

	ret = jc_letter_destroy(jv->letter);

	free(jv);

	return ret;
}

unsigned long
jc_variable_get(
	char *name,
	int depth,	
	unsigned long cmp_data,
	jc_variable_t var
)
{
	struct jc_variable *jv = NULL;
	struct jc_var_hash_node *jv_node = NULL;
	struct jc_letter_param param;
	struct jc_var_hash_param vh_param;

	jv = (typeof(jv))var;
	memset(&vh_param, 0, sizeof(vh_param));
	vh_param.depth = depth;
	vh_param.data = cmp_data;
	memset(&param, 0, sizeof(param));
	param.name = name;
	param.user_data = (unsigned long)&vh_param;
	jv_node = (typeof(jv_node))jc_letter_get(&param, jv->letter);
	if (!jv_node) {
		fprintf(stderr, "no var named %s in depth %d\n",
				 name, depth);
		return 0;
	}
	
	return jv_node->user_data;
}

int
jc_variable_add(
	char *name,
	int depth,
	unsigned long user_data,
	jc_variable_t var
)
{
	struct jc_variable *jv = NULL;
	struct jc_var_hash_node *jv_node = NULL;

	jv = (typeof(jv))var;
	jv_node = (typeof(jv_node))calloc(1, sizeof(*jv_node));
	if (!jv_node) {
		fprintf(stderr, "calloc %d bytes error: %s\n", 
				sizeof(*jv_node), strerror(errno));
		return JC_ERR;
	}
	jv_node->user_data = user_data;
	jv_node->depth = depth;
	jv_node->var = var;

	return jc_letter_add(name, (unsigned long)jv_node, jv->letter);
}

static int
jc_var_node_walk(
	unsigned long jv_data,
	unsigned long walk_data
)
{
	struct jc_var_hash_node *vnode = NULL;
	struct jc_var_traversal_param *param = NULL;
	
	vnode = (typeof(vnode))jv_data;
	param = (typeof(param))walk_data;

	if (param->jv_traversal) 
		return param->jv_traversal(vnode->user_data, 
					   param->walk_data);
	
	return JC_OK;
}

int
jc_variable_traversal(
	unsigned long user_data,		
	jc_variable_t var,
	int (*jv_traversal)(unsigned long jv_data, unsigned long data)
)
{
	struct jc_variable *jv = NULL;
	struct jc_var_traversal_param jv_param;	

	memset(&jv_param, 0, sizeof(jv_param));
	jv_param.walk_data = user_data;	
	jv_param.jv_traversal = jv_traversal;
	jv_param.var = var;

	jv = (typeof(jv))var;

	return jc_letter_traversal((unsigned long)&jv_param, 
				    jv->letter, 
				    jc_var_node_walk);
}
