#ifndef TC_PARAM_MANAGE_H
#define TC_PARAM_MANAGE_H

struct tc_param {
	char data[0];
};

struct tc_param_manage_list {
	int param_num;
	char **param_name;
};

#endif
