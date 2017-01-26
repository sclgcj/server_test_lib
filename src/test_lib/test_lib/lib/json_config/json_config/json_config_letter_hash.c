#include "json_config_letter_hash_private.h"

#define JC_LETTER_HASH_SIZE 26

struct jc_letter_node {
	char *name;
	jc_letter_t letter;
	unsigned long data;
	struct hlist_node node;
};

struct jc_letter {
	int (*letter_destroy)(unsigned long data);
	int (*letter_hash)(unsigned long user_data, unsigned long hash_data);
	int (*letter_get)(unsigned long user_data, unsigned long hash_data);
	tc_hash_handle_t handle;
};

struct jc_letter_traversal_param {
	unsigned long user_data;
	int (*letter_traversal)(unsigned long jl_data, unsigned long data);
};

static int
jc_letter_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_letter_node *jlh = NULL;
	
	if (!hnode && user_data) 
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		jlh = tc_list_entry(hnode, typeof(*jlh), node);
		if (jlh->name)
			name = jlh->name[0];
	}

	return (name % JC_LETTER_HASH_SIZE);
}

static int 
jc_letter_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int flag = 0;
	struct jc_letter *jl = NULL;
	struct jc_letter_node *jln = NULL;
	struct jc_letter_param *param = NULL;

	param = (struct jc_letter_param *)user_data;
	jln = tc_list_entry(hnode, typeof(*jln), node);

	if (!param->name && !jln->name)
		flag = 1;
	if (!param->name || !jln->name)
		flag = 0;
	if (!strncmp((char*)param->name, jln->name, strlen(jln->name)))
		flag = 1;

	if (flag) {
		jl = jln->letter;
		if (jl->letter_get)
			return jl->letter_get(jln->data, param->user_data);
		else
			return JC_OK;
	}

	return JC_ERR;
}

static int
jc_letter_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_letter *jl = NULL;
	struct jc_letter_node *jln = NULL;

	jln = tc_list_entry(hnode, typeof(*jln), node);
	if (jln->name)
		free(jln->name);
	jl = (struct jc_letter*)jln->letter;
	if (jl->letter_destroy)
		jl->letter_destroy(jln->data);
	free(jln);

	return JC_OK;
}

jc_letter_t
jc_letter_hash_create(
	int (*letter_hash)(unsigned long user_data, unsigned long hash_data),
	int (*letter_get)(unsigned long user_data, unsigned long cmp_data),
	int (*letter_destroy)(unsigned long data)
)
{
	struct jc_letter *jl = NULL;

	jl = (typeof(*jl)*)calloc(1, sizeof(*jl));
	if (!jl) {
		fprintf(stderr, "calloc %d bytes error: %s\n",
				sizeof(*jl), strerror(errno));
		exit(0);
	}
	jl->handle = tc_hash_create(
			JC_LETTER_HASH_SIZE, 
			jc_letter_hash, 
			jc_letter_hash_get, 
			jc_letter_hash_destroy);
	if (jl->handle == TC_HASH_ERR) 
		return NULL;
	jl->letter_destroy = letter_destroy;
	jl->letter_get = letter_get;
	jl->letter_hash = letter_hash;

	return (jc_letter_t)jl;
}

int
jc_letter_add(
	char *name,
	unsigned long user_data,
	jc_letter_t letter
)
{
	struct jc_letter *jl = NULL;
	struct jc_letter_node *jln = NULL;

	if (!jl)
		return JC_ERR;

	jln = (typeof(*jln)*)calloc(1, sizeof(*jln));
	if (!jln) {
		fprintf(stderr, "calloc %d bytes error: %s\n", 
				sizeof(*jln), strerror(errno));
		exit(0);
	}
	if (name)
		jln->name = strdup(name);
	jln->data = user_data;
	jln->letter = letter;
	
	jl = (struct jc_letter*)letter;
	return tc_hash_add(jl->handle, 
			   &jln->node, 
			   (unsigned long)jl);
}

unsigned long
jc_letter_get(
	struct jc_letter_param *jlp,
	jc_letter_t letter
)
{
	struct jc_letter *jl = NULL;
	struct hlist_node *hnode = NULL;
	struct jc_letter_node *jln = NULL;

	if (!letter)
		return JC_ERR;
	jl = (typeof(jl))letter;
	hnode = tc_hash_get(jl->handle, 
			    (unsigned long)jlp->name, 
			    (unsigned long)jlp);
	if (!hnode) {
		fprintf(stderr, "no node named %s\n", jlp->name);
		return (unsigned long)JC_ERR;
	}
	jln = tc_list_entry(hnode, typeof(*jln), node);

	return jln->data;
}

int
jc_letter_destroy(
	jc_letter_t letter
)
{
	int ret = 0;
	struct jc_letter *jl = NULL;

	if (!letter) return JC_OK;

	jl = (struct jc_letter*)letter;
	ret = tc_hash_destroy(jl->handle);
	free(jl);

	return ret;
}

static int
jc_letter_walk(
	unsigned long user_data,
	struct hlist_node *hnode,	
	int *flag
)
{
	struct jc_letter_node *jln = NULL;
	struct jc_letter_traversal_param *param = NULL;

	jln = tc_list_entry(hnode, typeof(*jln), node);
	param = (typeof(param))user_data;
	if (param->letter_traversal)
		return param->letter_traversal(jln->data, param->user_data);

	return JC_OK;
}

int
jc_letter_traversal(
	unsigned long user_data,
	jc_letter_t letter,
	int (*jl_traversal)(unsigned long jl_data, unsigned long data)
)
{
	struct jc_letter *jl = NULL;
	struct jc_letter_traversal_param param;

	if (!letter)
		return JC_ERR;

	jl = (typeof(jl))letter;
	memset(&param, 0, sizeof(param));
	param.user_data = user_data;
	param.letter_traversal = jl_traversal;
	
	return tc_hash_traversal((unsigned long)&param, 
				 jl->handle, 
				 jc_letter_walk);
}

