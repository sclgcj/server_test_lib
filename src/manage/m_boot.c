#include "m_boot.h"
#include "m_proj.h"
#include "m_send.h"
#include "manage.h"

#include "cJSON.h"

/*
 * If the client connected to the server successfully, 
 * it should send a boot request to the server to report
 * its basic information.
 *
 * The information contains : 
 *	1、The client's project info
 *	2、The current running project
 *	3、The last ending project
 *	4、The start time of current running project
 *	5、The start time of last ending project
 *	6、The end time of last ending project
 *	7、The duration time of the current runngin project
 *
 *	注意：这里有一个点是：客户端必须持续连接服务器
 */

static void
m_create_method_json(
	char  *sVal,
	cJSON *pStruRoot
)
{
	cJSON *pStruData = NULL;

	pStruData = cJSON_CreateString(sVal);
	if( !pStruData )
	{
		ML_ERROR("createString error \n");
		exit(0);
	}
	
	cJSON_AddItemToObject(pStruRoot, "RPCMethod", pStruData);
}

static int
m_boot_get_object_val(
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

static int
m_boot_get_object_str(
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

static void
m_boot_set_object_val(
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

static void
m_boot_set_object_str(
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

static void
m_boot_set_proj_object(
	MProj *pStruProj,
	cJSON *pStruArray
) 
{
	cJSON *pStruObj  = NULL;

	pStruObj = cJSON_CreateObject();
	if( !pStruObj )
	{
		ML_ERROR("create object error\n");
		exit(0);
	}

	m_boot_set_object_val(pStruProj->iRunCnt, "run_count", pStruObj);
	m_boot_set_object_val(pStruProj->iProjStatus, "proj_status", pStruObj);
	m_boot_set_object_str(pStruProj->sName, "proj_name", pStruObj);
	m_boot_set_object_val((unsigned int)pStruProj->tCreateTime, "proj_create_time", pStruObj);
	m_boot_set_object_val((unsigned int)pStruProj->tLastRunStartTime, "proj_last_start_time", pStruObj);
	m_boot_set_object_val((unsigned int)pStruProj->tLastRunEndTime, "proj_last_end_time", pStruObj);
	m_boot_set_object_val((unsigned int)pStruProj->tCurRunStartTime, "proj_cur_start_time", pStruObj);
	m_boot_set_object_val((unsigned int)pStruProj->tCurRunDurationTime, "proj_cur_dur_time", pStruObj);

	cJSON_AddItemToArray(pStruArray, pStruObj);
}

static int
m_boot_set_proj_array(
	MBase *pStruMB,
	cJSON *pStruArray
)
{
	int i = 0;
	cJSON *pStruObj = NULL;
	MProj struProj;

	ML_ERROR("iProjCnt = %d\n", pStruMB->struPA.iProjCnt );
	if( pStruMB->struPA.iProjCnt == 0 )
	{
		return ML_OK;
	}

	for( ; i < pStruMB->struPA.iProjCnt; i++ )
	{
		memset(&struProj, 0, sizeof(struProj));

		m_get_project_by_id(i, &pStruMB->struPA, &struProj);

		m_boot_set_proj_object(&struProj, pStruArray);
	}

	return ML_OK;
}

void
m_boot(
	void *pData
)
{	
	int iRet = 0;				
	char *sTmp = NULL;
	cJSON *pStruRoot = NULL;
	cJSON *pStruArray = NULL;
	MLink *pStruML = (MLink *)pData;

	pStruRoot = cJSON_CreateObject();
	if( !pStruRoot )
	{
		ML_ERROR("create object error\n");
		exit(0);
	}

	m_create_method_json("Boot",pStruRoot);

	pStruArray = cJSON_CreateArray();
	if( !pStruArray )
	{
		ML_ERROR("create array error\n");
		exit(0);
	}

	m_boot_set_object_str(
						pStruML->pStruM->struPA.sResultPath, 
						"result_path",
						pStruRoot
					);

	m_boot_set_proj_array(pStruML->pStruM, pStruArray);

	cJSON_AddItemToObject(pStruRoot, "projects", pStruArray);

	sTmp = cJSON_Print(pStruRoot);
	if( !sTmp )
	{
		ML_ERROR("cJSON_Print error\n");
		exit(0);
	}

	pthread_mutex_lock(&pStruML->struLinkMutex);
	pStruML->iLinkStatus = M_STATUS_SEND_BOOT;
	pthread_mutex_unlock(&pStruML->struLinkMutex);

	iRet = m_send_data(pStruML->iSockfd, sTmp);

	ML_FREE(sTmp);
	cJSON_Delete(pStruRoot);
	
	return iRet;
}

static void
m_boot_get_proj_info(
	cJSON *pStruData,
	MProj *pStruHead
)
{
	MProj *pStruD = NULL;

	ML_CALLOC(pStruD, MProj, 1);
	m_boot_get_object_val("run_count", pStruData, &pStruD->iRunCnt);
	m_boot_get_object_val("proj_status", pStruData, &pStruD->iProjStatus);
	m_boot_get_object_str("proj_name", pStruData, pStruD->sName);
	m_boot_get_object_val("proj_create_time", pStruData, (unsigned int *)&pStruD->tCreateTime);
	m_boot_get_object_val("proj_last_start_time", pStruData, (unsigned int*)&pStruD->tLastRunStartTime);
	m_boot_get_object_val("proj_last_end_time", pStruData, (unsigned int*)&pStruD->tLastRunEndTime);
	m_boot_get_object_val("proj_cur_start_time", pStruData, (unsigned int*)&pStruD->tCurRunStartTime);
	m_boot_get_object_val("proj_cur_dur_time", pStruData, (unsigned int*)&pStruD->tCurRunDurationTime);

	m_add_proj_node( pStruD, pStruHead );
}

static void
m_boot_prin_proj_info(
	MProj *pStruHead
)
{
	int iProjCnt = 0;
	MProj *pStruCur = pStruHead->pStruNext;

	while(pStruCur)
	{
		iProjCnt++;
		ML_ERROR("iProjCnt = %d\n", iProjCnt);
		ML_ERROR("iRunCnt = %d\n", pStruCur->iRunCnt);
		ML_ERROR("iProjStatus = %d\n", pStruCur->iProjStatus);
		ML_ERROR("sName = %s\n", pStruCur->sName);
		ML_ERROR("tCreateTime = %s\n", ctime(&pStruCur->tCreateTime));
		ML_ERROR("tLastRunStartTime = %s\n", ctime(&pStruCur->tLastRunStartTime));
		ML_ERROR("tLastRunEndTime = %s\n", ctime(&pStruCur->tLastRunEndTime));
		ML_ERROR("tCurRunStartTime = %s\n", ctime(&pStruCur->tCurRunStartTime));
		ML_ERROR("tCurRunDurationTime = %s\n", ctime(&pStruCur->tCurRunDurationTime));

		pStruCur = pStruCur->pStruNext;
	}
}

int
m_boot_handle_request(
	cJSON *pStruRoot,
	void  *pData
)
{
	int   i = 0;
	int   iNum = 0;
	char  *sTmp = NULL;
	MLink *pStruML = (MLink*)pData;
	cJSON *pStruProjs = NULL, *pStruData = NULL;

	if( !pStruRoot || !pStruML )
	{
		return ML_ERR;
	}

	pStruProjs = cJSON_GetObjectItem(pStruRoot, "projects");
	if( !pStruProjs )
	{
		ML_ERROR("no projects\n");
		return ML_ERR;
	}

	iNum = cJSON_GetArraySize(pStruProjs);
	ML_ERROR("projects num = %d\n", iNum);

	ML_CALLOC(pStruML->pStruProjInfo, MProjInfo, 1);
	pStruML->pStruProjInfo->iProjNum = iNum;
	m_boot_get_object_str(
							"result_path", 
							pStruRoot, 
							pStruML->pStruProjInfo->sResultPath 
						);
	ML_ERROR("sResultPath = %s", pStruML->pStruProjInfo->sResultPath);

	for( i = 0; i < iNum; i++ )
	{
		pStruData = cJSON_GetArrayItem(pStruProjs, i);
		m_boot_get_proj_info(pStruData, &pStruML->pStruProjInfo->struProjHead);
	}

	m_boot_prin_proj_info(&pStruML->pStruProjInfo->struProjHead);

	return ML_OK;
}

int
m_boot_handle_response(
	cJSON *pStruRoot,
	void  *pData
)
{
	return ML_OK;
}
