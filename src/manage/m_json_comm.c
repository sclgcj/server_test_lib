#include "ml_comm.h"
#include "m_json_comm.h"

int
m_json_get_object_val(
	char         *sName,
	cJSON        *pStruRoot,
	unsigned int *puiVal
)
{
	cJSON *pStruData = NULL;

	pStruData = cJSON_GetObjectItem(pStruRoot, sName);
	if( !pStruData )
	{
		ML_ERROR("no item %s\n", sName);
		return ML_ERR;
	}
	
	(*puiVal) = (unsigned int)pStruData->valueint;

	return ML_OK;
}

int
m_json_get_object_str(
	char  *sName,			
	cJSON *pStruRoot,
	char  *sData
)
{
	cJSON *pStruData = NULL;

	pStruData = cJSON_GetObjectItem(pStruRoot, sName);
	if( !pStruData )
	{
		ML_ERROR("no item : %s\n", sName);
		return ML_ERR;
	}

	memcpy(sData, pStruData->valuestring, strlen(pStruData->valuestring));

	return ML_OK;
}

void
m_json_set_object_val(
	unsigned int  iVal,
	char *sName,
	cJSON *pStruObj
)
{
	cJSON *pStruData = NULL;

	pStruData = cJSON_CreateNumber(iVal);
	if( !pStruData )
	{
		ML_ERROR("create number error\n");
		exit(0);
	}

	cJSON_AddItemToObject(pStruObj, sName, pStruData);
}

void
m_json_set_object_str(
	char *sStr,
	char *sName,
	cJSON *pStruObj
)
{
	cJSON *pStruData = NULL;	

	pStruData = cJSON_CreateString(sStr);
	if( !pStruData )
	{
		ML_ERROR("create string error\n");
		exit(0);
	}

	cJSON_AddItemToObject(pStruObj, sName, pStruData);
}

int
m_json_get_object_str_malloc(
	char *sName,
	cJSON *pStruRoot,
	char **sData
)
{
	int iLen = 0;
	cJSON *pStruData = NULL;

	pStruData = cJSON_GetObjectItem(pStruRoot, sName);
	if( !pStruData )
	{
		ML_ERROR("no item : %s\n", sName);
		return ML_ERR;
	}

	iLen = strlen(pStruData->valuestring);
	ML_CALLOC((*sData), char, (iLen + 1));
	memcpy((*sData), pStruData->valuestring, strlen(pStruData->valuestring));

	return ML_OK;

}

int
m_send_json_data(
	int   iSockfd,
	cJSON *pStruJson
)
{
	int iRet = 0;
	char *sTmp = NULL;

	sTmp = cJSON_Print(pStruJson);

	ML_ERROR("iSockfd = %d, sTmp = %s\n", iSockfd, sTmp);	

	iRet = m_send_data(iSockfd, sTmp);

	ML_FREE(sTmp);

	return iRet;
}
