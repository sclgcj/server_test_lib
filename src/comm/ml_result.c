#include "ml_result.h"

typedef struct _MLResult
{
	char sResultName[256];
	int  iTotalCnt;
	int  iPassCnt;
	int  iFailCnt;
	void *pUserData;
	MLResultFunc pFunc;
}MLResult, *PMLResult;

void
ml_handle_result(
	MLResultHandle struHandle
)
{
	FILE *pFp = NULL;
	MLResult *pStruR = (MLResult *)struHandle;

	if(!struHandle)
	{
		return;
	}

	pFp = fopen(pStruR->sResultName, "w");
	if( pFp == NULL )
	{
		ML_ERROR("fopen %s error : %s\n", pStruR->sResultName, strerror(errno));
		return;
	}
	if( pStruR->pFunc )
	{
		pStruR->pFunc(pFp, pStruR->pUserData);
	}
	else
	{
		ML_ERROR("result ok!\n");
	}

	fclose(pFp);
}

int
ml_create_result_handle(
	int  iTotalCnt,
	void *pUserData,
	char *sResultName,
	MLResultFunc pResultFunc,
	MLResultHandle *pStruRHandle
)
{
	int iLen = 0;
	MLResult *pStruR = NULL;

	ML_CALLOC(pStruR, MLResult, 1);
	pStruR->iTotalCnt = iTotalCnt;
	pStruR->pFunc     = pResultFunc;
	pStruR->pUserData = pUserData;

	iLen = strlen(sResultName);
	if( iLen >= 256 )
	{
		return ML_FILE_NAME_TOO_LONG;	
	}
	memcpy(pStruR->sResultName, sResultName, iLen);

	(*pStruRHandle) = (MLResultHandle)pStruR;

	return ML_OK;
}

void
ml_destroy_result_handle(
	MLResultHandle struHandle
)
{
	MLResult *pStruR = (MLResult *)struHandle;

	if( !struHandle )	
	{
		return;
	}

	ML_FREE(pStruR);
}

