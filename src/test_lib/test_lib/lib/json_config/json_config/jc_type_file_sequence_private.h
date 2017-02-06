#ifndef JC_TYPE_FILE_SEQUENCE_PRIVATE_H
#define JC_TYPE_FILE_SEQUENCE_PRIVATE_H

#include "jc_type_file_manage_private.h"

int
jc_type_file_seq_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper
);

int
jc_type_file_sequence_init();

int
jc_type_file_sequence_uninit();

#endif
