#include "st_comm.h"
#include "st_manage_config.h"


int
st_manage_parse_opt(
	STHandle struHandle
)
{
	ServerTest *pStruHandle = (ServerTest *)struHandle;

	return st_parse_opt(pStruHandle->struOptHandle);
}

int
st_manage_read_config(
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	return st_read_config(pStruST->struRCHandle);
}

int
st_manage_get_opt_val(
	char *sName,
	int  iValLen,
	char *sVal,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	return st_get_opt_val(sName, iValLen, sVal, pStruST->struOptHandle);
}

int
st_manage_read_config_val(
	char *sName, 
	int  iValLen,
	char *sVal,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	return st_get_read_config_val(
														sName,
														iValLen,
														sVal,
														pStruST->struRCHandle
													);
}

