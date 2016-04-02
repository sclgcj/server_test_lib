#include "ml_manage_data.h"

int
ml_manager_add_mmap_data(
	char		 *sFileName,
	int			 iFlag,
	int			 iDataNum,
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

	return ml_add_mmap_data(sFileName, iFlag, iDataNum, iTypeSize, pStruML->struDataHandle, piID);
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

void
ml_manager_destroy_data(
	int iID,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return;
	}

	return ml_destroy_special_data(iID, pStruML->struDataHandle);
}
