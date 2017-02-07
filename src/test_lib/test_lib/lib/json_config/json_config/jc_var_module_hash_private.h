#ifndef JC_VAR_MODULE_HASH_PRIVATE_H
#define JC_VAR_MODULE_HASH_PRIVATE_H

#include "jc_private.h"

typedef void* jc_var_module_t;

struct jc_var_module_param {	
	int depth;
	union {
		char *var;	
		char *module;
	}un;
	unsigned long user_data;
};

jc_var_module_t
jc_var_module_create(
	int (*var_get)(unsigned long var_data, unsigned long cmp_data),
	int (*var_destroy)(unsigned long var_data),
	int (*vm_get)(unsigned long user_data, unsigned long cmp_data),
	int (*vm_destroy)(unsigned long data)
);

int
jc_var_module_add(
	char *module,
	unsigned long user_data,
	jc_var_module_t vm
);

int
jc_var_add(
	int depth,
	char *var,
	char *module,
	unsigned long user_data,
	jc_var_module_t vm
);

unsigned long
jc_var_module_get(
	struct jc_var_module_param *jmp,
	jc_var_module_t vm
);

int
jc_var_module_destroy(
	jc_var_module_t vm
);

int
jc_var_module_traversal(
	jc_var_module_t vm,
	unsigned long data,
	int (*vm_traversal)(unsigned long m_data, unsigned long data)
);

unsigned long
jc_module_get(
	struct jc_var_module_param *param,
	jc_var_module_t vm
);

char *
jc_var_data_get(
	struct jc_var_module_param *jmp,
	jc_var_module_t vm
);

unsigned long
jc_var_user_data_get(
	struct jc_var_module_param *jmp,
	jc_var_module_t vm
);

#endif
