#ifndef JSON_CONFIG_MODE_PRIVATE_H
#define JSON_CONFIG_MODE_PRIVATE_H

#include "json_config_private.h"

/*
 * 模块的私有数据
 */
struct json_mode_private {
	char *node_name;
	cJSON *obj;
	unsigned long data;
	int (*other_mode_judge)(char *module, unsigned long user_data, struct json_config_comm *comm);
};

struct json_mode_oper {
	int (*json_mode_init)(struct json_config_comm *jcc);
	int (*json_mode_execute)(struct json_config_comm *jcc);
};

int
json_mode_module_add(
	char *name, 
	struct json_mode_oper *oper
);

int
json_config_mode_init();

int
json_config_mode_uninit();

#endif
