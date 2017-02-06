#ifndef JC_TYPE_FILE_RAND_PRIVATE_H
#define JC_TYPE_FILE_RAND_PRIVATE_H

#include "jc_type_file_manage_private.h"

int
jc_type_file_rand_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper
);

int
json_config_type_file_rand_uninit();

int
json_config_type_file_rand_init();

#endif
