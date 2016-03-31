#include "m_boot.h"
#include "m_proj.h"
#include "m_send.h"
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

#define M_DEFAULT_PROJECT_PATH "/var/project_info"

static void
m_create_method_json(
	char  *sVal
	cJSON *pStruRoot,	
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

int
m_boot_set_proj_array(
	MBase *pStruMB,
	cJSON pStruArray
)
{
	int i = 0;
	cJSON *pStruObj = NULL;
	MProj struProj;

	if( pStruMB->pStruPA->iProjCnt == 0 )
	{
		return ML_OK;
	}

	for( ; i < pStruMB->pStruPA->iProjCnt; i++ )
	{
		memset(&struProj, 0, sizeof(struProj));

		m_get_project_by_id(i, pStruMB->pStruPA, &struProj);

		m_boot_set_proj_object(&struProj, pStruArray);
	}

	return ML_OK;
}


int 
m_boot(
	MLink *pStruML
)
{	
	int iRet = 0;				
	char *sTmp = NULL;
	cJSON *pStruRoot = NULL;
	cJSON *pStruArray = NULL;

	pStruRoot = cJSON_CreateObject();
	if( !pStruRoot )
	{
		ML_ERROR("create object error\n");
		exit(0);
	}

	m_create_method_json(pStruRoot, "Boot");

	pStruArray = cJSON_CreateArray();
	if( !pStruArray )
	{
		ML_ERROR("create array error\n");
		exit(0);
	}

	m_boot_set_proj_array(pStruML->pStruM, pStruArray);

	cJSON_AddItemToObject(pStruRoot, "projects", pStruArray);

	sTmp = cJSON_Print(pStruRoot);
	if( !sTmp )
	{
		ML_ERROR("cJSON_Print error\n");
		exit(0);
	}

	iRet = m_send_data(sTmp);

	ML_FREE(sTmp);
	
	return iRet;
}

