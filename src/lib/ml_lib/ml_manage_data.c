#include "ml_manage_data.h"

int
ml_manager_add_mmap_data(
	char		 *sFileName,
	int			 iFlag,
	int			 iTypeSize,
	MLHandle struHandle,
	int			 *piID
)
{
	ServerTest  *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_add_mmap(sFileName, iFlag, iTypeSize, pStruML->struDataHandle, piID);
}

int
ml_manager_get_mmap_data(
	int			 iID,
	int			 iOffset,
	MLHandle struHandle,
	void		 *pData
)
{
	ServerTest  *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_get_mmap_data(iID, iOffset, pStruML->struDataHandle, pData);
}

int
ml_manager_set_mmap_data(
	int			 iID,
	int			 iOffset,
	void		 *pData,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_set_mmap_data(iID, iOffset, pData, pStruML->struDataHandle);
}