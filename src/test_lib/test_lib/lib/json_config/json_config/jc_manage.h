#ifndef JSON_CONFIG_MANAGE_H
#define JSON_CONFIG_MANSGE_H

#define JSON_CONFIG_INIT(func) \
	int __attribute__((constructor)) func()

int
json_config_manage_create(
	char *name,
	char *file_path
);

int
json_config_manage_destroy();

int
json_config_manage_special_destroy(char *name);

char *
json_config_manage_param_get(
	int  id,
	unsigned long user_data,
	char *name
);

#endif
