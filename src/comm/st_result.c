#include "st_result.h"

typedef struct _STResult
{
	char sResultName[256];
	int  iTotalCnt;
	int  iPassCnt;
	int  iFailCnt;
	void *pUserData;
	STResultFunc pFunc;
}STResult, *PSTResult;

void
st_handle_result(
	STResultHandle struHandle
)
{
	FILE *pFp = NULL;
	STResult *pStruR = (STResult *)struHandle;

	if(!struHandle)
	{
		return;
	}

	pFp = fopen(pStruR->sResultName, "w");
	if( pFp == NULL )
	{
		ST_ERROR("fopen %s error : %s\n", pStruR->sResultName, strerror(errno));
		return;
	}
	if( pStruR->pFunc )
	{
		pStruR->pFunc(pFp, pStruR->pUserData);
	}
	else
	{
		ST_ERROR("result ok!");
	}

	fclose(pFp);
}

int
st_create_result_handle(
	int  iTotalCnt,
	void *pUserData,
	char *sResultName,
	STResultFunc pResultFunc,
	STResultHandle *pStruRHandle
)
{
	int iLen = 0;
	STResult *pStruR = NULL;

	ST_CALLOC(pStruR, STResult, 1);
	pStruR->iTotalCnt = iTotalCnt;
	pStruR->pFunc     = pResultFunc;
	pStruR->pUserData = pUserData;

	iLen = strlen(sResultName);
	if( iLen >= 256 )
	{
		return ST_FILE_NAME_TOO_LONG;	
	}
	memcpy(pStruR->sResultName, sResultName, iLen);

	(*pStruRHandle) = (STResultHandle)pStruR;

	return ST_OK;
}

void
st_destroy_result_handle(
	STResultHandle struHandle
)
{
	STResult *pStruR = (STResult *)struHandle;

	if( !struHandle )	
	{
		return;
	}

	ST_FREE(pStruR);
}

