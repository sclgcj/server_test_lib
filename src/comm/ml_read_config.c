#include "ml_comm.h"
#include "ml_read_config.h"
#include "ml_comm_config.h"

typedef struct _MLRCManage
{
	char *sFile;
	MLCommConfig *pStruConfHead;
}MLRCManage, *PMLRCManage;

static int 
ml_read_config(
	MLRCHandle struHandle
)
{
	char sLine[1024] = { 0 };
	FILE *pFp = NULL;
	char sName[32] = { 0 };
	char sVal[256] = { 0 };
	MLRCManage *pStruM = (MLRCManage *)struHandle;
	MLCommConfig *pStruCur = NULL, *pStruNext = NULL;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	pFp = fopen(pStruM->sFile, "r");
	if( !pFp )
	{
		ML_ERROR("fopen %s error: %s\n", pStruM->sFile, strerror(errno));
		exit(0);
	}

	pStruCur = pStruM->pStruConfHead;
	while(fgets(sLine, 1024, pFp))
	{
		sscanf(sLine, "%s = %s", sName, sVal);
		ML_SET_CONFIG_WITH_NEXT(sName, sVal, pStruCur);
		memset(sLine, 0, 1024);
	}

	fclose(pFp);
	return ML_OK;
}

int
ml_create_read_config(
	char				*sFile,
	MLRCHandle  *pStruHandle
)
{
	int iLen = 0;
	MLRCManage *pStruM = NULL;

	ML_CALLOC(pStruM, MLRCManage, 1);
	ML_CALLOC(pStruM->pStruConfHead, MLCommConfig, 1);
	if( sFile )
	{
		iLen = strlen(sFile);
		ML_CALLOC(pStruM->sFile, char , (iLen + 1));
		memcpy(pStruM->sFile, sFile, iLen);
	}

	(*pStruHandle) = (MLRCHandle)pStruM;

	ml_read_config((*pStruHandle));

	return ML_OK;
}

void
ml_destroy_read_config(
	MLRCHandle struHandle
)
{
	MLRCManage *pStruM = (MLRCManage *)struHandle;
	
	if( !struHandle )
	{
		return;
	}
	
	ML_DEMLROY_COMM_CONFIG(pStruM->pStruConfHead);
	ML_FREE(pStruM->sFile);
	ML_FREE(pStruM);
}

int
ml_get_read_config_val(
	char *sName,
	char **sVal,
	MLRCHandle struHandle
)
{
	int iRet = 0;
	MLRCManage *pStruM = (MLRCHandle)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	ML_GET_VAL(sName, sVal, pStruM->pStruConfHead, iRet);

	return iRet;
}

