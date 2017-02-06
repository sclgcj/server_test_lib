#include "jc_var_module_hash_private.h"
#include "jc_letter_hash_private.h"
#include "tc_hash.h"

struct jc_module_node {
	unsigned long user_data;
	jc_var_module_t vm;
};

struct jc_var_node {
	int  depth;
	char *module;
};

struct jc_vm_traversal_param {
	jc_var_module_t vm;
	unsigned long data;
	int (*vm_traversal)(unsigned long m_data, unsigned long data);
};

struct jc_var_module {
	jc_var_module_t var_hash;
	jc_var_module_t module_hash;
	int (*vm_hash)(unsigned long user_data, unsigned long hash_data);
	int (*vm_get)(unsigned long user_data, unsigned long cmp_data);
	int (*vm_destroy)(unsigned long data);
	
};

static int
jc_var_destroy(
	unsigned long data
)
{
	struct jc_var_node *jvn = NULL;

	if (!data)
		return JC_OK;

	jvn = (typeof(*jvn)*)data;
	if (jvn->module)
		free(jvn->module);
	free(jvn);

	return JC_OK;
}

static int
jc_module_destroy(
	unsigned long data
)
{
	int ret = 0;
	struct jc_var_module *jvm = NULL;
	struct jc_module_node *jmn = NULL;
	
	if (!data)
		return JC_OK;
		
	jmn = (typeof(*jmn)*)data;
	jvm = (typeof(*jvm)*)jmn->vm;
	if (jvm->vm_destroy)
		ret = jvm->vm_destroy(jmn->user_data);
	free(jmn);

	return ret;
}

static int
jc_module_hash_get(
	unsigned long user_data,
	unsigned long cmp_data
)
{
	struct jc_var_module *jvm = NULL;
	struct jc_module_node *jmn = NULL;
	struct jc_var_module_param *jvmp = NULL;
	
	jmn = (typeof(jmn))user_data;
	jvmp = (typeof(jvmp))cmp_data;
	jvm = (typeof(jvm))jmn->vm;

	if (jvm->vm_get) 
		return jvm->vm_get(jmn->user_data, jvmp->user_data);

	return JC_OK;
}

static int
jc_var_hash_get(
	unsigned long user_data,
	unsigned long cmp_data
)
{
	struct jc_var_node *jvn = NULL;
	struct jc_var_module_param *jvmp = NULL;

	jvn = (typeof(jvn))user_data;
	jvmp = (typeof(jvmp))cmp_data;

	if (jvn->depth == jvmp->depth) 
		return JC_OK;

	return JC_ERR;
}

jc_var_module_t
jc_var_module_create(
	int (*vm_hash)(unsigned long user_data, unsigned long hash_data),
	int (*vm_get)(unsigned long user_data, unsigned long cmp_data),
	int (*vm_destroy)(unsigned long data) 
)
{
	struct jc_var_module *jvm = NULL;

	jvm = (typeof(*jvm)*)calloc(1, sizeof(*jvm));
	if (!jvm) {
		fprintf(stderr, "calloc %d bytes error : %s\n",
				sizeof(*jvm), strerror(errno));
		exit(0);
	}
	jvm->vm_get = vm_get;
	jvm->vm_hash = vm_hash;
	jvm->vm_destroy = vm_destroy;
	jvm->var_hash = jc_letter_create(NULL, jc_var_hash_get, jc_var_destroy);
	jvm->module_hash = jc_letter_create(NULL, jc_module_hash_get, jc_module_destroy);

	return (jc_var_module_t)jvm;
}

int
jc_var_module_add(
	char *module,
	unsigned long user_data,
	jc_var_module_t vm
)
{
	struct jc_var_module *jvm = NULL;
	struct jc_module_node *jmn = NULL;

	if (!vm)
		return JC_ERR;

	jmn = (typeof(*jmn)*)calloc(1, sizeof(*jmn));
	if (!jmn) {
		fprintf(stderr, "calloc %d bytes error : %s\n", 
				sizeof(*jmn), strerror(errno));
		exit(0);
	}
	jmn->user_data = user_data;
	jmn->vm = vm;
	jvm = (typeof(*jvm)*)vm;

	return jc_letter_add(module, (unsigned long)jmn, jvm->module_hash);
}

int
jc_var_add(
	int depth,
	char *var,
	char *module,
	jc_var_module_t vm
)
{
	struct jc_var_node *jvn = NULL;
	struct jc_var_module *jvm = NULL;

	if (!vm)
		return JC_ERR;

	jvn = (typeof(*jvn)*)calloc(1, sizeof(*jvn));
	if (!jvn) {
		fprintf(stderr, "calloc %d bytes error : %s\n", 
				sizeof(*jvn), strerror(errno));
		exit(0);
	}
	if (module)
		jvn->module = strdup(module);
	jvn->depth = depth;
	jvm = (typeof(*jvm)*)vm;

	return jc_letter_add(var, (unsigned long)jvn, jvm->var_hash);
}

unsigned long
jc_module_get(
	struct jc_var_module_param *jmp,
	jc_var_module_t vm
)
{
	struct jc_var_module *jvm = NULL;
	struct jc_module_node *jmn = NULL;
	struct jc_letter_param param;

	if (!vm)
		return JC_ERR;
	jvm = (typeof(jvm))vm;
	memset(&param, 0, sizeof(param));
	param.name = jmp->un.module;
	param.user_data = (unsigned long)jmp;
	jmn = (typeof(jmn))jc_letter_get(&param,
			         	 jvm->module_hash);
	if ((unsigned long)jmn == (unsigned long)JC_ERR) {
		fprintf(stderr, "no module named %s\n", jmp->un.module);
		return (unsigned long)JC_ERR;
	}

	return jmn->user_data;
}


unsigned long
jc_var_module_get(
	struct jc_var_module_param *jmp,
	jc_var_module_t vm
)
{
	struct jc_var_node *jvn = NULL;
	struct jc_var_module *jvm = NULL;
	struct jc_module_node *jmn = NULL;
	struct jc_letter_param lparam;
	struct jc_var_module_param param;

	if (!vm)
		return JC_ERR;
	jvm = (typeof(*jvm)*)vm;
	memset(&lparam, 0, sizeof(lparam));
	lparam.name = jmp->un.var;
	lparam.user_data = (unsigned long)jmp;
	jvn = (typeof(*jvn)*)jc_letter_get(&lparam,
					   jvm->var_hash);
	if ((unsigned long)jvn == (unsigned long)JC_ERR) {
		fprintf(stderr, "no variable named %s\n", jmp->un.var);
		return (unsigned long)JC_ERR;
	}
	lparam.name = (jvn->module);
	lparam.user_data = (unsigned long)jmp;
	jmn = (typeof(*jmn)*)jc_letter_get(&lparam,
					   jvm->module_hash);
	if ((unsigned long)jmn == (unsigned long)JC_ERR) {
		fprintf(stderr, "no module named %s\n", jvn->module);
		return (unsigned long)JC_ERR;
	}

	return jmn->user_data;
}

int
jc_var_module_destroy(
	jc_var_module_t vm
)
{
	int ret = 0;
	struct jc_var_module *jvm = NULL;

	if (!vm)
		return JC_OK;

	jvm = (typeof(*jvm)*)vm;

	ret = jc_letter_destroy(jvm->var_hash);
	ret |= jc_letter_destroy(jvm->module_hash);

	return ret;
}

static int
jc_vm_module_traversal(
	unsigned long module_data,
	unsigned long user_data
)
{
	struct jc_vm_traversal_param *param = NULL;

	param = (typeof(param))user_data;
	if (param->vm_traversal)
		return param->vm_traversal(module_data, param->data);

	return JC_OK;
}

static int
jc_vm_var_traversal(
	unsigned long var_data,
	unsigned long user_data
)
{
	struct jc_var_node *jvn = NULL;
	struct jc_var_module *jvm = NULL;
	struct jc_vm_traversal_param *param = NULL;
	
	jvn = (typeof(jvn))var_data;
	param = (typeof(param))user_data;
	jvm = (typeof(jvm))param->vm;

	return jc_letter_traversal((unsigned long)param,
				   jvm->module_hash, 
				   jc_vm_module_traversal);
}

int
jc_var_module_traversal(
	jc_var_module_t vm,
	unsigned long data,
	int (*vm_traversal)(unsigned long m_data, unsigned long data)
)
{
	struct jc_var_module *jvm = NULL;
	struct jc_vm_traversal_param param;	

	if (!vm)
		return JC_ERR;

	jvm = (struct jc_var_module *)vm;
	
	memset(&param, 0, sizeof(param));
	param.vm = vm;
	param.vm_traversal = vm_traversal;
	param.data = data;

	return jc_letter_traversal((unsigned long)&param, 
				   jvm->var_hash, 
				   jc_vm_var_traversal);
}

