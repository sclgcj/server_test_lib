#include "ml_comm.h"
#include "ml_client_data.h"

typedef struct _MLClientData
{
	int  iInit;
	void *pClientData;
}MLClientData, *PMLClientData;

typedef struct _MLDataArray
{
	int iCnt;
	int iNum;
	MLClientData *pStruCD;
}MLDataArray, *PMLDataArray;

void
ml_create_client_data(
	int iClientNum,
	MLCDHandle *pStruDHandle
)
{
	MLDataArray *pStruDA = NULL;

	ML_CALLOC(pStruDA, MLDataArray, 1);
	pStruDA->iNum = iClientNum;
	ML_CALLOC(pStruDA->pStruCD, MLClientData, iClientNum);

	(*pStruDHandle) = (MLCDHandle)pStruDA;
}

void
ml_destroy_client_data(
	MLCDHandle struHandle
)
{
	MLDataArray *pStruDA = (MLDataArray *)struHandle;

	if( !struHandle )
	{
		return;
	}

	ML_FREE(pStruDA->pStruCD);
	ML_FREE(pStruDA);
}

int
ml_add_client_data(
	void			   *pUserData,
	MLCDHandle struHandle,
	int					 *piID
)
{
	MLDataArray *pStruDA = (MLDataArray *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	if( pStruDA->iCnt >= pStruDA->iNum )
	{
		return ML_SPACE_FULL;
	}

	(*piID) = pStruDA->iCnt++;

	pStruDA->pStruCD[(*piID)].iInit			  = 1;
	pStruDA->pStruCD[(*piID)].pClientData = pUserData;

	return ML_OK;
}

int
ml_get_client_data(
	int					 iID,
	MLCDHandle   struHandle,
	void				 **pUserData
)
{
	MLDataArray *pStruDA = (MLDataArray*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	if( iID >= pStruDA->iNum )
	{
		return ML_DATA_END;
	}
	if( !pStruDA->pStruCD[iID].iInit )
	{
		(*pUserData) = NULL;
		return ML_OK;
	}

	(*pUserData) = pStruDA->pStruCD[iID].pClientData;

	return ML_OK;
}

