#ifndef TC_PARAM_MANAGE_PRIVATE_H
#define TC_PARAM_MANAGE_PRIVATE_H

#include "tc_param_manage.h"

// parameter manage operation structure
struct tc_param_oper {
	//All these callback must be implemented
	int (*param_oper)(int oper_cmd, struct tc_param *); 
	void (*param_set)(struct tc_param *new_param, struct tc_param *old_param); 
	void (*param_destroy)(struct tc_param *param);
	char *(*param_value_get)(struct tc_param *param); 
	struct tc_param *(*param_copy)(struct tc_param *param); 
};

// add parameter type
int
tc_param_type_add(
	char *param_type,
	struct tc_param_oper *oper
);

#endif
