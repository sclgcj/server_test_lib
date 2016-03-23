#include <unistd.h>
#include "st_comm.h"
#include "st_handle_opt.h"
#include "st_comm_config.h"

typedef struct _STOptManage
{
	int iArgc;
	char **ssArgv;
	char *sFmt;
	STCommConfig *pStruConfHead;
	int (*STCommConfFunc)(char *sName, STCommConfig *pStruConf, int iValLen, char *sVal);
}STOptManage, *PSTOptManage;

static int
st_parse_opt(
	STOptHandle struHandle
)
{
	int i = 0;
	int iOpt = 0;	
	int iRet = ST_OK;
	char *sPath = NULL;
	char sData[2] = { 0 };
	STOptManage *pStruM = NULL;
	STCommConfig *pStruNew = NULL, *pStruCur = NULL;

	pStruM = (STOptManage*)struHandle;
	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}
	if( !pStruM->sFmt )
	{
		return ST_OK;
	}

	pStruCur = pStruM->pStruConfHead;
	while((iOpt = getopt(pStruM->iArgc, pStruM->ssArgv, pStruM->sFmt)) != -1)
	{
		ST_ERROR("iOpt = %c\n", (char)iOpt);

		sData[0] = (char)iOpt;
		ST_SET_CONFIG_WITH_NEXT(sData, optarg, pStruCur);
		optarg = NULL;
	}

	return iRet;
}



void
st_create_opt_config(
	int					iArgc,
	char			  **ssArgv,
	char        *sParseFmt,
	STOptHandle *pStruHandle
)
{
	STOptManage *pStruM = NULL;

	ST_CALLOC(pStruM, STOptManage, 1);
	ST_CALLOC(pStruM->pStruConfHead, STCommConfig, 1);
	if( sParseFmt )
	{
		ST_CALLOC(pStruM->sFmt, char, (strlen(sParseFmt) + 1));
		memcpy(pStruM->sFmt, sParseFmt, strlen(sParseFmt));
	}
	pStruM->iArgc = iArgc;
	pStruM->ssArgv = ssArgv;

	(*pStruHandle) = (STOptHandle)pStruM;

	st_parse_opt((*pStruHandle));
}

void
st_destroy_opt_config(
	STOptHandle struHandle
)
{
	STOptManage *pStruM = (STOptManage*)struHandle;

	if( !struHandle )
	{
		return;
	}

	ST_DESTROY_COMM_CONFIG(pStruM->pStruConfHead);
	ST_FREE(pStruM->sFmt);
	ST_FREE(pStruM);
}

static void
st_set_opt_config(
	char  ch,
	char  *sVal,
	STCommConfig *pStruConf
)
{
	int iLen = 2;

	ST_CALLOC(pStruConf->sName, char, iLen);
	pStruConf->sName[0] = ch;

	if( !sVal )
	{
		return;
	}
	iLen = strlen(sVal);
	ST_CALLOC(pStruConf->sVal, char ,iLen + 1);
	memcpy(pStruConf->sVal, sVal, iLen);
}

static void
st_calloc_new_config(
	STCommConfig *pStruCur 
)
{
	STCommConfig *pStruConf = NULL;

	ST_CALLOC(pStruConf, STCommConfig, 1);
	pStruCur->pStruNext = pStruConf;
}

int
st_get_opt_val(
	char *sName, 
	int  iValLen,
	char *sVal,
	STOptHandle struHandle
)
{
	int iRet = 0;
	STOptManage *pStruM = (STOptManage *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	ST_GET_VAL(sName, iValLen, sVal, pStruM->pStruConfHead, iRet);

	return iRet;
}
