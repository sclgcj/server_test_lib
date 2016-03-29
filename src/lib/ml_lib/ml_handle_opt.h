#ifndef ML_HANDLE_OPT_H
#define ML_HANDLE_OPT_H 1

#include "ml_read_config.h"
#include "ml_comm_config.h"

typedef void * MLOptHandle;

void
ml_create_opt_config(
	int					iArgc,
	char 				**sArgv,
	char        *sParseFmt,
	MLOptHandle *pStruHandle
);

void
ml_destroy_opt_config(
	MLOptHandle struHandle
);

int
ml_get_opt_val(
	char *sName, 
	char **sVal,
	MLOptHandle struHandle
);

#endif
