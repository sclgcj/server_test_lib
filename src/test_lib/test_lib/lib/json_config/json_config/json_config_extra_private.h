#ifndef JSON_CONFIG_EXTRA_PRIVATE_H
#define JSON_CONFIG_EXTRA_PRIVATE_H 1

#include "json_config_private.h"

struct jc_extra_oper {
	int (*extra_init)(struct json_config_comm *comm);
	int (*extra_execute)(struct json_config_comm *comm);
};

struct jc_extra_private {
	char *var_name;
	unsigned long user_data;
	cJSON *obj;
	int (*other_extra_module)(char *node_name, 
				  char *module_name,
				  unsigned long data, 
				  struct json_config_comm *jcc);
};

int
jc_extra_module_add(
	char *module_name,
	struct jc_extra_oper *oper
);

int
json_config_extra_init();

int
json_config_extra_uninit();

#endif
