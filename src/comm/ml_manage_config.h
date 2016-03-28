#ifndef ML_MANAGE_CONFIG_H
#define ML_MANAGE_CONFIG_H 1

#include "ml_manage.h"

int
ml_manager_get_opt_val(
	char *sName,
	char **sVal,
	MLHandle struHandle
);

int
ml_manager_read_config_val(
	char *sName, 
	char **sVal,
	MLHandle struHandle
);

#endif
