#include "tc_comm.h"
#include "tc_err.h"
#include "tc_param_api.h"
#include "tc_create_private.h"
#include "tc_param_api_private.h"
#include "tc_param_manage_private.h"

tc_param_manage_t *
tc_param_create()
{
	(tc_param_manage_t*)tc_param_manage_create();
}

void
tc_param_destroy(
	tc_param_manage_t *pm
)
{
	tc_param_manage_destroy((struct tc_param_manage*)pm);
}

int
tc_param_add(
	char		*param_name,
	char		*param_type,
	tc_param_t	*param
)
{
	return tc_param_manage_add(param_name, param_type, (struct tc_param*)param);
}

int
tc_param_del(
	char *param_name
)
{
	return tc_param_manage_del(param_name);
}

int
tc_param_list_get(
	tc_param_list_t *list	
)
{
	return tc_param_manage_list_get((struct tc_param_manage_list *)list);
}

void
tc_param_list_free(
	tc_param_list_t *list
)
{
	tc_param_manage_list_free((struct tc_param_manage_list*)list);
}

tc_param_t *
tc_param_config_get(
	char		*param_name,
	unsigned long	user_data
)
{
	struct tc_create_link_data *cl_data = NULL;

	if (!param_name || !user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return NULL;
	}

	cl_data = tc_create_link_data_get(user_data);

	return (tc_param_t*)cl_data->pm->pm_config_get(param_name, cl_data->pm);
}

int
tc_param_set(
	char		*param_name,
	tc_param_t	*param,
	unsigned long	user_data
)
{
	struct tc_create_link_data *cl_data = NULL;

	if (!param_name || !param || !user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	cl_data = tc_create_link_data_get(user_data);

	return cl_data->pm->pm_set(param_name, param, cl_data->pm);
}

char *
tc_param_value_get(
	char		*param_name,
	unsigned long	user_data
)
{
	struct tc_create_link_data *cl_data = NULL;

	if (!param_name || !user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return NULL;
	}
	cl_data = tc_create_link_data_get(user_data);

	return cl_data->pm->pm_value_get(param_name, cl_data->pm);
}

int
tc_param_oper(
	int oper_cmd,
	char *param_name,
	unsigned long user_data
)
{
	struct tc_create_link_data *cl_data = NULL;

	if (!param_name || !user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	cl_data = tc_create_link_data_get(user_data);

	return cl_data->pm->pm_oper(oper_cmd, param_name, cl_data->pm);
}
