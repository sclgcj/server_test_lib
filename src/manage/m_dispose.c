#include "manage.h"
#include "m_error.h"
#include "m_dispose.h"

void
m_create_dispose(
	MDispose *pStruMD
)
{
	return;
}

void
m_destroy_dispose(
	MDispose *pStruHead
)
{
	MDispose *pStruD = NULL, *pStruS = NULL;

	if( !pStruHead )
	{
		return;	
	}
	pStruD = pStruHead->pStruNext;
	while(pStruD)
	{
		pStruS = pStruD->pStruNext;	
		if( pStruD->sMethod )
		{
			ML_FREE(pStruD->sMethod);
		}
		ML_FREE(pStruD);
		pStruD = pStruS;
	}
}

int
m_add_dispose(
	int iStatus,	
	char *sMethod,
	MDisposeFunc pFunc,
	MDispose *pStruHead
)
{
	int iLen = 0;
	MDispose *pStruD = NULL;

	if( !pStruHead )
	{
		return ML_PARAM_ERR;
	}

	ML_CALLOC(pStruD, MDispose, 1);
	pStruD->iStatus = iStatus;
	if( sMethod )
	{
		iLen = strlen(sMethod);
		ML_CALLOC(pStruD->sMethod, char, (iLen + 1));
		memcpy(pStruD->sMethod, sMethod, iLen);
	}
	pStruD->pFunc = pFunc;

	pStruD->pStruNext = pStruHead->pStruNext;
	pStruHead->pStruNext = pStruD;

	return ML_OK;
}

static int
m_dispatch_request(
	unsigned long ulData,	
	MDispose      *pStruD
)
{
	int iRet = 0;
	char *sTmp = NULL;

	sTmp = (char *)ulData;

	ML_CMP_DATA(sTmp, pStruD->sMethod, iRet);

	return iRet;
}

static int
m_dispatch_response(
	unsigned long ulData,
	MDispose      *pStruD
)
{
	if( ulData == pStruD->iStatus )
	{
		return ML_OK;
	}

	return ML_ERR;
}

static int
m_dispose_dispatch(
	unsigned long ulData,
	MDisposeDispatchFunc pFunc,
	cJSON *pStruRoot,
	MLink *pStruML
)
{
	int iRet = 0;
	char *sTmp = NULL;
	MDispose *pStruD = NULL;

	pStruD = &pStruML->pStruM->struDispose;

	while(pStruD)
	{
		if( !pStruD->sMethod )
		{
			pStruD = pStruD->pStruNext;	
			continue;
		}
		iRet = pFunc(ulData, pStruD);
		if( iRet == ML_OK )
		{
			if( pStruD->pFunc )
			{
				return pStruD->pFunc(pStruRoot, pStruML);
			}
		}
	}

	return M_NO_RELATED_FUNC;
}

void
m_dispose(
	int  iRecvLen,
	char *sRecvData,
	void *pUserData
)
{
	int iRet = 0;
	unsigned long ulData = 0;
	cJSON *pStruRoot = NULL;
	cJSON *pStruData = NULL;
	MLink *pStruML = (MLink *)pUserData;

	if( iRecvLen == 0 || !sRecvData )
	{
		ML_ERROR("dispose error\n");
		return;
	}

	pStruRoot = cJSON_Parse(sRecvData);
	pStruData = cJSON_GetObjectItem(pStruRoot, "RPCMethod");
	if( pStruData )
	{
		ulData = (unsigned long)pStruData->valuestring;
		iRet = m_dispose_dispatch(ulData, m_dispatch_request, pStruRoot, pStruML);
		goto out;
	}
	pStruData = cJSON_GetObjectItem(pStruRoot, "Result");
	if( pStruData )
	{
		ulData = pStruData->valueint;
	iRet = m_dispose_dispatch(ulData, m_dispatch_response, pStruRoot, pStruML);
	}

out:
	if( iRet != ML_OK )
	{
		ML_ERROR("recv data = %s\n", sRecvData);
	}

	cJSON_Delete(pStruRoot);
	
	return;
}

