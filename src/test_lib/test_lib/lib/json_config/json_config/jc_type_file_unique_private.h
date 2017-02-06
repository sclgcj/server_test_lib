#ifndef JC_TYPE_FILE_UNIQUE_PRIVATE_H
#define JC_TYPE_FILE_UNIQUE_PRIVATE_H

#include "jc_type_file_manage_private.h"

struct jc_type_file_unique_private {
	int alloc_num;
};

int
jc_type_file_unique_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper
);

int
json_config_type_file_unique_uninit();

int
json_config_type_file_unique_init();

#endif
