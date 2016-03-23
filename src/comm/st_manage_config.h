#ifndef ST_MANAGE_CONFIG_H
#define ST_MANAGE_CONFIG_H 1

#include "st_manage.h"

int
st_manage_get_opt_val(
	char *sName,
	int  iValLen,
	char *sVal,
	STHandle struHandle
);

int
st_manage_read_config_val(
	char *sName, 
	int  iValLen,
	char *sVal,
	STHandle struHandle
);

#endif
