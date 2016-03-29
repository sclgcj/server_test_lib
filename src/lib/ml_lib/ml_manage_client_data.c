#include "ml_comm.h"
#include "ml_manage_client_data.h"

int
ml_manager_get_client_data(
	int			 iID,
	MLHandle struHandle,
	void		 **pUserData
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_get_client_data(iID, pStruML->struDataHandle, pUserData);
}

int
ml_manager_add_client_data(
	void				 *pUserData,
	MLDataHandle struHandle,
	int					 *piID
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_add_client_data(pUserData, pStruML->struDataHandle, piID);
}
