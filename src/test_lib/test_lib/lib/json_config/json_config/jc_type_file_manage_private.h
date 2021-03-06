#ifndef JC_TYPE_FILE_MANAGE_PRIVATE_H
#define JC_TYPE_FILE_MANAGE_PRIVATE_H

#include "jc_var_module_hash_private.h"

struct jc_type_file_manage_oper {
	int (*manage_init)(struct jc_comm *jcc);
	int (*manage_copy)(unsigned int data_num);
	int (*manage_execute)(struct jc_comm *jcc);
};

struct jc_type_file_manage{
	jc_var_module_t jvm;
//	struct jc_type_file_manage_oper oper;
	int (*init)(struct jc_type_file_manage *man, struct jc_comm *jcc);
	int (*copy)(struct jc_type_file_manage *man, unsigned int data_num);
	int (*execute)(struct jc_type_file_manage *man, struct jc_comm *jcc);
	int (*module_add)(char *module, struct jc_type_file_manage_oper *oper,
			  struct jc_type_file_manage *man);
};

struct jc_type_file_manage *
jc_type_file_manage_create();

int
jc_type_file_manage_destroy(
	struct jc_type_file_manage *fm 
);

#endif
