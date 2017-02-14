#ifndef JC_MODE_PRIVATE_H
#define JC_MODE_PRIVATE_H

#include "jc_private.h"

/*
 * 模块的私有数据
 */
struct json_mode_private {
	char *node_name;
	cJSON *obj;
	unsigned long data;
	int (*other_mode_judge)(char *module, unsigned long user_data, struct jc_comm *comm);
};

struct json_mode_oper {
	int (*json_mode_init)(struct jc_comm *jcc);
	int (*json_mode_execute)(struct jc_comm *jcc);
};

int
json_mode_module_add(
	char *name, 
	int def,
	struct json_mode_oper *oper
);

int
jc_mode_init();

int
jc_mode_uninit();

#endif
