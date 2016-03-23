#ifndef ST_READ_CONFIG_H
#define ST_READ_CONFIG_H

typedef void * STRCHandle;

enum
{
	ST_RC_INT,
	ST_RC_UINT,
	ST_RC_CHAR,
	ST_RC_UCHAR,
	ST_RC_SHORT,
	ST_RC_USHORT,
	ST_RC_STRING,
	ST_RC_IPADDR,
	ST_RC_FUNC,
	ST_RC_MAX
};

int
st_create_read_config(
	char				*sFile,
	STRCHandle  *pStruHandle
);

void
st_destroy_read_config(
	STRCHandle struHandle
);

int
st_get_read_config_val(
	char *sName,
	int  iValLen,
	char *sVal,
	STRCHandle struHandle
);

#endif
