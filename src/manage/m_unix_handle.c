#include "manage.h"
#include "m_json_comm.h"
#include "m_unix_handle.h"

typedef struct _MLDetailParam
{
	unsigned short usPort;
	unsigned int   uiIP;
	unsigned int   iID;
}MLDetailParam, *PMLDetailParam;

static void
m_unix_test_response(
	char *sTmp,
	MLink *pStruML
)
{
	int iLen = 0;
	char *sRes = "{\"Result\": 0, \"data\":\"%s\"}";
	char sSendData[512] = { 0 };

	sprintf(sSendData, sRes, sTmp);
	
	m_send_data(pStruML->iSockfd, sSendData);
}

int
m_unix_test(
	cJSON *pStruRoot,
	void *pData
)
{
	char *sTmp = NULL;
	MLink *pStruML = (MLink*)pData;

	if( !pData )
	{
		return ML_PARAM_ERR;
	}

	m_json_get_object_str_malloc("data", pStruRoot, &sTmp);
	ML_ERROR("sTmp = %s\n", sTmp);

	m_unix_test_response(sTmp, pStruML);

	ML_FREE(sTmp);
	return ML_OK;
}

static void
m_add_test_basic_info(
	char *sIP,
	unsigned short usPort,
	cJSON *pStruArray
)
{
	cJSON *pStruObj = NULL;
	char  sPort[6] = { 0 };

	sprintf(sPort, "%d", usPort);

	pStruObj = cJSON_CreateObject();
	m_json_set_object_str(
									sIP,
									"ip",
									pStruObj 
								);
	m_json_set_object_str(sPort, "port", pStruObj);

	cJSON_AddItemToArray(pStruArray, pStruObj);
}

static  void
m_add_server_basic_info(
	MLink *pStruML, 
	cJSON *pStruArray,
	void  *pUserData
)
{
	m_add_test_basic_info(
				inet_ntoa(pStruML->struAddr.sin_addr), 
				ntohs(pStruML->struAddr.sin_port), 
				pStruArray 
			);
}



static int
m_send_server_data(
	int   iType,
	void  *pData,
	void  (*pGetData)(MLink *pStruML, cJSON *pStruArray, void *pUserData),
	void  *pUserData
)
{
	int i = 0;
	int iNum = 0, iRet = 0;
	unsigned long *pulArray = NULL;
	cJSON *pStruJson = NULL, *pStruArray = NULL;
	MLink *pStruML = (MLink *)pData;
	MLink *pStruTmp = NULL;
	MBListManage *pStruMBM = NULL;

	pStruMBM = &pStruML->pStruM->struMBM;
	pStruMBM->pGetListFunc(iType, pStruMBM, &iNum, &pulArray);

	pStruJson = cJSON_CreateObject();
	pStruArray = cJSON_CreateArray();

	m_json_set_object_val(0, "Result", pStruJson);
	ML_ERROR("iNum = %d", iNum);

	for( ; i < iNum; i++ )
	{
		pStruTmp = list_entry((struct list_head *)pulArray[i], MLink, struNode);
		if( pGetData )
		{
			pGetData(pStruTmp, pStruArray, pUserData);
		}
	}

	cJSON_AddItemToObject(pStruJson, "data", pStruArray);

	iRet = m_send_json_data(pStruML->iSockfd, pStruJson);

	ML_FREE(pulArray);
	cJSON_Delete(pStruJson);

	return iRet;
}

int
m_get_cur_run_servers(
	cJSON *pStruRoot,
	void *pData
)
{
	int iRet = 0;

	if( !pStruRoot || !pData )
	{
		return ML_PARAM_ERR;
	}

	iRet = m_send_server_data(
				M_LIST_RUNNGING, 
				pData,
				m_add_server_basic_info,
				NULL
			);

	return iRet;
}

//Here we don't know how to distinguish the test machine's role before
//it real plays. Thus, the server and test role mean that the machine 
//currently is running as a server or a test client. This is determined 
//by the decision made by users. Of course, we also will use a machine 
//as either a server or a test, so a machine can also has double identies

static void
m_add_server_role(
	MLink *pStruML,
	cJSON *pStruArray,
	void  *pUserData
)
{
	if( pStruML->iRole & ML_ROLE_SERVER )
	{
		m_add_test_basic_info(
			inet_ntoa(pStruML->struAddr.sin_addr), 
			ntohs(pStruML->struAddr.sin_port), 
			pStruArray 
		);
	}
}

int
m_get_servers(
	cJSON *pStruRoot,
	void  *pData
)
{
	int iRet = 0;

	if( !pStruRoot || !pData )
	{
		return ML_PARAM_ERR;
	}

	iRet = m_send_server_data(
						M_LIST_RUNNGING, 
						pData, 
						m_add_server_role,
						NULL
					);
	
	return iRet;
}

static void
m_add_test_role(
	MLink *pStruML,
	cJSON *pStruArray,
	void  *pUserData
)
{
	if( pStruML->iRole & ML_ROLE_CLIENT )
	{
		m_add_test_basic_info(
			inet_ntoa(pStruML->struAddr.sin_addr), 
			ntohs(pStruML->struAddr.sin_port), 
			pStruArray 
		);
	}
}

int
m_get_test_list(
	cJSON *pStruRoot,
	void  *pData
)
{
	int iRet = 0;

	if( !pStruRoot || !pData )
	{
		return ML_PARAM_ERR;
	}

	iRet = m_send_server_data(
						M_LIST_RUNNGING, 
						pData, 
						m_add_test_role,
						NULL
					);
	
	return iRet;
}

static void
m_add_project_list(
	MProjInfo *pStruInfo,
	cJSON			*pStruArray
)
{
	int i = 0;
	MProj *pStruCur = NULL;

	pStruCur = pStruInfo->struProjHead.pStruNext;
	while(pStruCur)
	{
		m_boot_set_proj_object(pStruCur, pStruArray);
		pStruCur = pStruCur->pStruNext;
	}
}

static void
__m_get_detail(
	MLink *pStruML,
	cJSON *pStruArray,
	void  *pUserData
)
{
	MLDetailParam *pStruMDP = (MLDetailParam *)pUserData;
	
	if( pStruML->struAddr.sin_addr.s_addr == pStruMDP->uiIP && 
			ntohs(pStruML->struAddr.sin_port) == pStruMDP->usPort)
	{
			m_add_project_list(
								pStruML->pStruProjInfo,
								pStruArray 
							);	
	}
}


int
m_get_detail(
	cJSON *pStruRoot,
	void  *pData
)
{
	int iRet = 0;
	char sID[6] = { 0 };
	char sIP[16] = {0};
	char sPort[6] = { 0 };
	char *sTmp = NULL;
	MLDetailParam struMDP;

	if( !pStruRoot || !pData )
	{
		return ML_PARAM_ERR;
	}
	
	sTmp = cJSON_Print(pStruRoot);
	ML_ERROR("sTmp = %s\n", sTmp);
	ML_FREE(sTmp);
	memset(&struMDP, 0, sizeof(struMDP));
	m_json_get_object_str("ip", pStruRoot, sIP);
	struMDP.uiIP = inet_addr(sIP);
	m_json_get_object_str("port", pStruRoot, sPort);
	struMDP.usPort = (unsigned short)atoi(sPort);
	m_json_get_object_str("id", pStruRoot, sID);
	struMDP.iID = atoi(sID);
	ML_ERROR("id= %d,ip = %x:%d\n",struMDP.iID,struMDP.uiIP,struMDP.usPort);

	iRet = m_send_server_data(
										M_LIST_RUNNGING,												
										pData,															
										__m_get_detail,
										(void *)&struMDP
									);
	return iRet;
}
