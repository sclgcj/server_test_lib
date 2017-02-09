#ifndef JC_HANDLE_PRIVATE_H
#define JC_HANDLE_PRIVATE_H

#include "jc_private.h"

struct jc_handle_oper {
	int (*handle_init)(struct jc_comm *jcc);
	int (*handle_execute)(struct jc_comm *jcc);
};

struct jc_handle_private {
	char *node_name;
};

int
jc_handle_module_add(
	char *module,
	struct jc_handle_oper *oper
);

int
json_config_handle_init();

int
json_config_handle_uninit();

#endif
