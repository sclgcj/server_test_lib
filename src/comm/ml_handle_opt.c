#include <unistd.h>
#include "ml_comm.h"
#include "ml_handle_opt.h"
#include "ml_comm_config.h"

typedef struct _MLOptManage
{
	int iArgc;
	char **ssArgv;
	char *sFmt;
	MLCommConfig *pStruConfHead;
	int (*MLCommConfFunc)(char *sName, MLCommConfig *pStruConf, int iValLen, char *sVal);
}MLOptManage, *PMLOptManage;

static int
ml_parse_opt(
	MLOptHandle struHandle
)
{
	int i = 0;
	int iOpt = 0;	
	int iRet = ML_OK;
	char *sPath = NULL;
	char sData[2] = { 0 };
	MLOptManage *pStruM = NULL;
	MLCommConfig *pStruNew = NULL, *pStruCur = NULL;

	pStruM = (MLOptManage*)struHandle;
	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	if( !pStruM->sFmt )
	{
		return ML_OK;
	}

	pStruCur = pStruM->pStruConfHead;
	while((iOpt = getopt(pStruM->iArgc, pStruM->ssArgv, pStruM->sFmt)) != -1)
	{
		sData[0] = (char)iOpt;
		ML_SET_CONFIG_WITH_NEXT(sData, optarg, pStruCur);
		optarg = NULL;
	}

	return iRet;
}

void
ml_create_opt_config(
	int					iArgc,
	char			  **ssArgv,
	char        *sParseFmt,
	MLOptHandle *pStruHandle
)
{
	MLOptManage *pStruM = NULL;

	ML_CALLOC(pStruM, MLOptManage, 1);
	ML_CALLOC(pStruM->pStruConfHead, MLCommConfig, 1);
	if( sParseFmt )
	{
		ML_CALLOC(pStruM->sFmt, char, (strlen(sParseFmt) + 1));
		memcpy(pStruM->sFmt, sParseFmt, strlen(sParseFmt));
	}
	pStruM->iArgc = iArgc;
	pStruM->ssArgv = ssArgv;

	(*pStruHandle) = (MLOptHandle)pStruM;

	ml_parse_opt((*pStruHandle));
}

void
ml_destroy_opt_config(
	MLOptHandle struHandle
)
{
	MLOptManage *pStruM = (MLOptManage*)struHandle;

	if( !struHandle )
	{
		return;
	}

	ML_DEMLROY_COMM_CONFIG(pStruM->pStruConfHead);
	ML_FREE(pStruM->sFmt);
	ML_FREE(pStruM);
}

static void
ml_set_opt_config(
	char  ch,
	char  *sVal,
	MLCommConfig *pStruConf
)
{
	int iLen = 2;

	ML_CALLOC(pStruConf->sName, char, iLen);
	pStruConf->sName[0] = ch;

	if( !sVal )
	{
		return;
	}
	iLen = strlen(sVal);
	ML_CALLOC(pStruConf->sVal, char ,iLen + 1);
	memcpy(pStruConf->sVal, sVal, iLen);
}

static void
ml_calloc_new_config(
	MLCommConfig *pStruCur 
)
{
	MLCommConfig *pStruConf = NULL;

	ML_CALLOC(pStruConf, MLCommConfig, 1);
	pStruCur->pStruNext = pStruConf;
}

int
ml_get_opt_val(
	char *sName, 
	char **sVal,
	MLOptHandle struHandle
)
{
	int iRet = 0;
	MLOptManage *pStruM = (MLOptManage *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	ML_GET_VAL(sName, sVal, pStruM->pStruConfHead, iRet);

	return iRet;
}
