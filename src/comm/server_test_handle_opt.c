#include <unistd.h>
#include "server_test_comm.h"
#include "server_test_handle_opt.h"
#include "server_test_comm_config.h"

typedef struct _STOptManage
{
	char *sFmt;
	STCommConfig *pStruConfHead;
	int (*STCommConfFunc)(char *sName, STCommConfig *pStruConf, int iValLen, char *sVal);
}STOptManage, *PSTOptManage;

void
server_test_create_opt_config(
	char        *sParseFmt,
	STOptHandle *pStruHandle
)
{
	STOptManage *pStruM = NULL;

	SERVER_TEST_CALLOC(pStruM, STOptManage, 1);
	SERVER_TEST_CALLOC(pStruM->pStruConfHead, STCommConfig, 1);
	if( sParseFmt )
	{
		SERVER_TEST_CALLOC(pStruM->sFmt, char, (strlen(sParseFmt) + 1));
		memcpy(pStruM->sFmt, sParseFmt, strlen(sParseFmt));
	}

	(*pStruHandle) = (STOptHandle)pStruM;
}

void
server_test_destroy_opt_config(
	STOptHandle struHandle
)
{
	STOptManage *pStruM = (STOptManage*)struHandle;

	if( !struHandle )
	{
		return;
	}

	SERVER_TEST_DESTROY_COMM_CONFIG(pStruM->pStruConfHead);
	SERVER_TEST_FREE(pStruM->sFmt);
	SERVER_TEST_FREE(pStruM);
}

static void
server_test_set_opt_config(
	char  ch,
	char  *sVal,
	STCommConfig *pStruConf
)
{
	int iLen = 2;

	SERVER_TEST_CALLOC(pStruConf->sName, char, iLen);
	pStruConf->sName[0] = ch;

	if( !sVal )
	{
		return;
	}
	iLen = strlen(sVal);
	SERVER_TEST_CALLOC(pStruConf->sVal, char ,iLen + 1);
	memcpy(pStruConf->sVal, sVal, iLen);
}

static void
server_test_calloc_new_config(
	STCommConfig *pStruCur 
)
{
	STCommConfig *pStruConf = NULL;

	SERVER_TEST_CALLOC(pStruConf, STCommConfig, 1);
	pStruCur->pStruNext = pStruConf;
}

int
server_test_parse_opt(
	int  iArgc,
	char **sArgv,
	STOptHandle struHandle
)
{
	int i = 0;
	int iOpt = 0;	
	int iRet = SERVER_TEST_OK;
	char *sPath = NULL;
	char sData[2] = { 0 };
	STOptManage *pStruM = NULL;
	STCommConfig *pStruNew = NULL, *pStruCur = NULL;

	pStruM = (STOptManage*)struHandle;
	if( !struHandle )
	{
		return SERVER_TEST_PARAM_ERR;
	}
	if( !pStruM->sFmt )
	{
		return SERVER_TEST_OK;
	}

	pStruCur = pStruM->pStruConfHead;
	while((iOpt = getopt(iArgc, sArgv, pStruM->sFmt)) != -1)
	{
		ST_ERROR("iOpt = %c\n", (char)iOpt);

		sData[0] = (char)iOpt;
		SERVER_TEST_SET_CONFIG_WITH_NEXT(sData, optarg, pStruCur);
		optarg = NULL;
	}

	return iRet;
}

int
server_test_get_opt_val(
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
		return SERVER_TEST_PARAM_ERR;
	}

	SERVER_TEST_GET_VAL(sName, iValLen, sVal, pStruM->pStruConfHead, iRet);

	return iRet;
}
