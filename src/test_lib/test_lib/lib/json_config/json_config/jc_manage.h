#ifndef JC_MANAGE_H
#define JC_MANSGE_H

#define JC_INIT(func) \
	int __attribute__((constructor)) func()

int
jc_manage_create(
	char *name,
	char *file_path
);

int
jc_manage_destroy();

int
jc_manage_special_destroy(char *name);

char *
jc_manage_param_get(
	int  id,
	unsigned long user_data,
	char *name
);

#endif
