#ifndef TC_PARAM_API_H
#define TC_PARAM_API_H


#include "tc_param_manage.h"
#include "tc_param_api_private.h"

typedef struct tc_param			tc_param_t;	
typedef struct tc_param_manage_list	tc_param_list_t;

int
tc_param_add(
	char		*param_name,
	char		*param_type,	
	tc_param_t	*param
);

int
tc_param_del(
	char	*param_name
);

tc_param_t*
tc_param_config_get(
	char		*param_name
);

int
tc_param_set(
	char		*param_name,
	tc_param_t	*param
);

char *
tc_param_value_get(
	char *param_name,
	unsigned long user_data
);

int
tc_param_oper(
	int oper_cmd,
	char *param_name
);

int
tc_param_list_get(
	tc_param_list_t *list
);

void
tc_param_list_free(
	tc_param_list_t *list
);

#endif
