#ifndef JSON_CONFIG_TYPE_FILE_SEQUENCE_PRIVATE_H
#define JSON_CONFIG_TYPE_FILE_SEQUENCE_PRIVATE_H

#include "json_config_type_file_manage_private.h"

int
jc_type_file_seq_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper
);

int
json_config_type_file_sequence_init();

int
json_config_type_file_sequence_uninit();

#endif
