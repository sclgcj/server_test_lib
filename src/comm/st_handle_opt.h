#ifndef ST_HANDLE_OPT_H
#define ST_HANDLE_OPT_H 1

#include "st_read_config.h"
#include "st_comm_config.h"

typedef void * STOptHandle;

void
st_create_opt_config(
	int					iArgc,
	char 				**sArgv,
	char        *sParseFmt,
	STOptHandle *pStruHandle
);

void
st_destroy_opt_config(
	STOptHandle struHandle
);

int
st_get_opt_val(
	char *sName, 
	int  iValLen,
	char *sVal,
	STOptHandle struHandle
);

#endif
