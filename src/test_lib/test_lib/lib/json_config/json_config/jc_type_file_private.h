#ifndef JC_TYPE_FILE_PRIVATE_H
#define JC_TYPE_FILE_PRIVATE_H

#include "jc.h"

struct jc_type_file_oper {
	int (*file_init)(struct jc_comm *jcc);
	int (*file_copy)(unsigned int data_num);
	int (*file_execute)(struct jc_comm *jcc); };

#define JC_TYPE_FILE_COMM_DEFAULT_END  "continue_with_last_value"
#define JC_TYPE_FILE_COMM_DEFAULT_SEP  ','

struct jc_type_file_comm {
	char separate;
	char *path;
	char *module;
	char *col_name;
	char *end_action;
	int col_num;
};

struct jc_type_file_private {
	unsigned long private_data;
	struct jc_type_file_comm *comm;
	int (*file_init_cb)(struct jc_comm *jcc);
	int (*file_execute_cb)(struct jc_comm *jcc);
};

int
jc_type_file_module_add(
	int  default_module,
	char *module,
	struct jc_type_file_oper *oper
);

int
json_config_type_file_init();

int
json_config_type_file_uninit();

#endif
