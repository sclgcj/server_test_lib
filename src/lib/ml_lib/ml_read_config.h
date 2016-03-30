#ifndef ML_READ_CONFIG_H
#define ML_READ_CONFIG_H

typedef void * MLRCHandle;

int
ml_create_read_config(
	char				*sFile,
	MLRCHandle  *pStruHandle
);

void
ml_destroy_read_config(
	MLRCHandle struHandle
);

int
ml_get_read_config_val(
	char *sName,
	char **sVal,
	MLRCHandle struHandle
);

#endif
