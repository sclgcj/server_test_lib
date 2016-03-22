#include "st_manage_hub.h"

int
st_manage_add_hub(
	void						*pData,
	unsigned long		*pulHubID,
	STHandle				struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	return st_add_hub(pData, pulHubID, pStruST->struHubHandle);
}

int
st_manage_del_hub(
	unsigned long ulHubID,
	STHandle      struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	return st_del_hub(ulHubID, pStruST->struHubHandle);
}

