#include "st_comm.h"
#include "st_read_config.h"
#include "st_comm_config.h"

typedef struct _STRCManage
{
	char *sFile;
	STCommConfig *pStruConfHead;
}STRCManage, *PSTRCManage;

int
st_create_read_config(
	char				*sFile,
	STRCHandle  *pStruHandle
)
{
	int iLen = 0;
	STRCManage *pStruM = NULL;

	if( !sFile )
	{
		return ST_PARAM_ERR;
	}

	iLen = strlen(sFile);
	ST_CALLOC(pStruM, STRCManage, 1);
	ST_CALLOC(pStruM->pStruConfHead, STCommConfig, 1);
	ST_CALLOC(pStruM->sFile, char , (iLen + 1));
	memcpy(pStruM->sFile, sFile, iLen);

	(*pStruHandle) = (STRCHandle)pStruM;

	return ST_OK;
}

void
st_destroy_read_config(
	STRCHandle struHandle
)
{
	STRCManage *pStruM = (STRCManage *)struHandle;
	
	if( !struHandle )
	{
		return;
	}
	
	ST_DESTROY_COMM_CONFIG(pStruM->pStruConfHead);
	ST_FREE(pStruM->sFile);
	ST_FREE(pStruM);
}

int 
st_read_config(
	STRCHandle struHandle
)
{
	char sLine[1024] = { 0 };
	FILE *pFp = NULL;
	char sName[32] = { 0 };
	char sVal[256] = { 0 };
	STRCManage *pStruM = (STRCManage *)struHandle;
	STCommConfig *pStruCur = NULL, *pStruNext = NULL;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	pFp = fopen(pStruM->sFile, "r");
	if( !pFp )
	{
		ST_ERROR("fopen %s error: %s\n", pStruM->sFile, strerror(errno));
		exit(0);
	}

	pStruCur = pStruM->pStruConfHead;
	while(fgets(sLine, 1024, pFp))
	{
		sscanf(sLine, "%s = %s", sName, sVal);
		ST_SET_CONFIG_WITH_NEXT(sName, sVal, pStruCur);
		memset(sLine, 0, 1024);
	}

	fclose(pFp);
	return ST_OK;
}

int
st_get_read_config_val(
	char *sName,
	int  iValLen,
	char *sVal,
	STRCHandle struHandle
)
{
	int iRet = 0;
	STRCManage *pStruM = (STRCHandle)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	ST_GET_VAL(sName, iValLen, sVal, pStruM->pStruConfHead, iRet);

	return iRet;
}

/*
void
st_set_config_value(
	char				*sVal,
	STRConfig		*pStruConf
)
{
	int iLen = 0;

	switch(pStruConf->uiType)
	{
		case ST_RC_INT:
			*(int *)pStruConf->ulData = atoi(sVal);
			break;
		case ST_RC_UINT:
			*(unsigned int *)pStruConf->ulData = (unsigned int)atoi(sVal);
			break;
		case ST_RC_CHAR:
			*(char *)pStruConf->ulData = sVal[0];
			break;
		case ST_RC_UCHAR:
			*(unsigned char *)pStruConf->ulData = (unsigned char)sVal[0];
			break;
		case ST_RC_SHORT:
			*(short *)pStruConf->ulData = (short)atoi(sVal);
			break;
		case ST_RC_USHORT:
			*(unsigned short *)pStruConf->ulData = (unsigned short)atoi(sVal);
			break;
		case ST_RC_STRING:
			iLen = strlen(sVal);
			memcpy((void*)pStruConf->ulData, sVal, iLen);
			break;
		case ST_RC_IPADDR:
			*(unsigned int*)pStruConf->ulData = inet_addr(sVal);
			break;
		case ST_RC_FUNC:
			pStruConf->pFunc(sVal, pStruConf->ulData);
			break;
	}
}

*/
