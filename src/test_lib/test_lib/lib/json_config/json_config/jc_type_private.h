#ifndef JC_TYPE_PRIVATE_H
#define JC_TYPE_PRIVATE_H

/*
 * 可以参考loadrunner的参数化设计:
 * 类型模块:
 * object
 * array
 * file
 * plain string
 * unique number
 * ...
 *
 * 每个模块下可能会有其他的子模块，主要是与mode相关联的
 * 比如文件参数就可能分为:
 *	each_iteration_file
 *	unique_file
 *	...
 */

#include "jc_private.h"

struct jc_type_oper {
	int (*jc_type_copy)(unsigned int data_num);
	int (*jc_type_init)(struct jc_comm *jcc);
	int (*jc_type_execute)(struct jc_comm *jcc);
};

struct jc_type_private {
	char *node_name;
	unsigned long user_data;
	unsigned long sub_module_private;
	cJSON *obj;
	int (*init_cb)(char *node_name, 
		       cJSON *obj, 
		       unsigned long user_data,
		       struct jc_comm *jcc);
	int (*execute_cb)(char *node_name, 
		          char *module, 
		          unsigned long user_data, 
		          struct jc_comm *jcc);
};

int
jc_type_module_add(
	char *module,
	struct jc_type_oper *oper
);

int
json_config_type_init();

int
json_config_type_uninit();

#endif
