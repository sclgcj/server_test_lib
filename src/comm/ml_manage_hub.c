#include "ml_manage_hub.h"

int
ml_manage_add_hub(
	void						*pData,
	unsigned long		*pulHubID,
	MLHandle				struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_add_hub(pData, pulHubID, pStruML->struHubHandle);
}

int
ml_manage_del_hub(
	unsigned long ulHubID,
	MLHandle      struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_del_hub(ulHubID, pStruML->struHubHandle);
}

