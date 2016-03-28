#ifndef ML_READ_CONFIG_H
#define ML_READ_CONFIG_H

typedef void * MLRCHandle;

enum
{
	ML_RC_INT,
	ML_RC_UINT,
	ML_RC_CHAR,
	ML_RC_UCHAR,
	ML_RC_SHORT,
	ML_RC_USHORT,
	ML_RC_MLRING,
	ML_RC_IPADDR,
	ML_RC_FUNC,
	ML_RC_MAX
};

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
