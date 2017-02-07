#ifndef JC_VAR_HASH_PRIVATE_H
#define JC_VAR_HASH_PRIVATE_H

typedef void * jc_variable_t;

jc_variable_t
jc_variable_create(
	int (*var_get)(unsigned long user_data, 
		       unsigned long cmp_data),
	int (*var_destroy)(unsigned long user_data)
);

int
jc_variable_destroy(
	jc_variable_t var
);

unsigned long
jc_variable_get(
	char *name,
	int depth,	
	unsigned long cmp_data,
	jc_variable_t var
);

int
jc_variable_add(
	char *name,
	int depth,
	unsigned long user_data,
	jc_variable_t var
);

int
jc_variable_traversal(
	unsigned long user_data,		
	jc_variable_t var,
	int (*jv_traversal)(unsigned long jv_data, unsigned long data)
);

#endif
