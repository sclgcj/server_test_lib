#include "toml.h"
#include "ml_comm.h"
#include "ml_read_config.h"
#include "ml_comm_config.h"

#include <sys/stat.h>

typedef struct _MLRCManage
{
	char *sFile;
	MLCommConfig *pStruConfHead;
}MLRCManage, *PMLRCManage;

static void  
ml_get_toml_root( 
	MLRCManage *pStruM, 
	struct toml_node **ppStruRoot 
)
{
	char *sFileBuf = NULL;
	FILE *pFp = NULL;
	struct stat sBuf;

	ML_ERROR("\n");
	memset(&sBuf, 0, sizeof(sBuf));
	stat(pStruM->sFile, &sBuf);

	ML_ERROR("\n");
	ML_CALLOC(sFileBuf, char, sBuf.st_size);


	ML_ERROR("\n");
	pFp = fopen(pStruM->sFile, "r");
	if( !pFp )
	{
		ML_ERROR("fopen %s error: %s\n", pStruM->sFile, strerror(errno));
		exit(0);
	}

	ML_ERROR("\n");
	fread(sFileBuf, sizeof(char), sBuf.st_size, pFp);

	ML_ERROR("sFileBuf = %s\n", sFileBuf);
	fclose(pFp);

	ML_ERROR("\n");
	toml_init(ppStruRoot);
	ML_ERROR("\n");
	toml_parse((*ppStruRoot), sFileBuf, sBuf.st_size);

	ML_ERROR("\n");
	ML_FREE(sFileBuf);
	ML_ERROR("\n");
}

static void 
ml_set_rc_node(
	struct toml_node *pStruTN,		
	void						 *pData
)
{
	char *sVal = NULL;
	char *sName = NULL;
	enum toml_type eType;
	MLCommConfig *pStruHead = (MLCommConfig *)pData;
	MLCommConfig *pStruCur  = NULL;

	ML_ERROR("\n");
	eType = toml_type(pStruTN);

	ML_ERROR("\n");
	switch( eType )
	{
		case TOML_ROOT:
		case TOML_LIST:
		case TOML_TABLE:
		case TOML_TABLE_ARRAY:
			break;
		default:
	ML_ERROR("\n");
			sVal = toml_value_as_string(pStruTN);
	ML_ERROR("sVal = %s\n", sVal);
			sName = toml_name(pStruTN);
	ML_ERROR("sName = %s\n", sName);
			ML_ADD_CONFIG(sName, sVal, pStruHead);
	ML_ERROR("\n");
			ML_FREE(sVal);
			ML_FREE(sName);
	ML_ERROR("\n");
			break;	
	}
}

static int 
ml_read_config(
	MLRCHandle struHandle
)
{
	char sLine[1024] = { 0 };
	char sName[32] = { 0 };
	char sVal[256] = { 0 };
	MLRCManage *pStruM = (MLRCManage *)struHandle;
	MLCommConfig *pStruCur = NULL, *pStruNext = NULL;
	struct toml_node *pStruRoot = NULL;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	ml_get_toml_root(pStruM, &pStruRoot);

	ML_ERROR("\n");
	toml_walk(pStruRoot, ml_set_rc_node, (void*)pStruM->pStruConfHead);

	ML_ERROR("\n");
/*	pStruCur = pStruM->pStruConfHead;
	while(fgets(sLine, 1024, pFp))
	{
		sscanf(sLine, "%s = %s", sName, sVal);
		ML_SET_CONFIG_WITH_NEXT(sName, sVal, pStruCur);
		memset(sLine, 0, 1024);
	}

	fclose(pFp);*/
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

	ML_ERROR("\n");
	ML_GET_VAL(sName, sVal, pStruM->pStruConfHead->pStruNext, iRet);

	return iRet;
}

