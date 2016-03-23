#include "push_client.h"
#include "st_hub.h"
#include "st_boot.h"
#include "st_create.h"
#include "st_config.h"
#include "st_result.h"
#include "st_handle.h"
#include "st_test_data.h"
#include "cJSON.h"

enum
{
	ST_TYPE_RESPONSE,
	ST_TYPE_REQUEST,
	ST_PTYPE_MAX,
};

typedef int (*PCHandleFunc)(cJSON *pStruRoot, PushClient *pStruPC);


typedef struct _PCHandleFunc
{
	int					 uiStatus;
	char				 *sFuncName;
	PCHandleFunc pFunc;
}PCHandle, *PPCHandle;

static int giThreadID = 0;
static PCHandle gStruHF[] = {
	{-1, "TestData", st_handle_request_test_data},
	{ST_STATUS_SEND_UPING, NULL, st_handle_uping_response},
	{ST_STATUS_SEND_UACTIVE, NULL, st_handle_uactive_response},
	{ST_STATUS_SEND_UCONN, NULL, st_handle_uconn_response},
	{ST_STATUS_SEND_TEST_DATA, NULL, st_handle_response_test_data},
	{ST_STATUS_HUB, NULL, st_handle_hub}
};

static int
st_master_handle_data(
	struct list_head *pStruNode
)
{
	int iLen = 0;
	PCHubNode *pStruHN = NULL;
	char *pData = NULL;

	if( !pStruNode )
	{
		return ST_ERR;
	}
	pStruHN = list_entry(pStruNode, PCHubNode, struNode);
	PC_DEBUG("-----------------------------\n");
	st_stop_recv_check(pStruHN->iSockfd, pStruHN->pData);
	PC_DEBUG("master data\n");

	/* 可以使用 pthread_getspecifick 或者pthread_setspecifick 来用于管理函数
	 * 和分配函数之间的参数传递
	 */
	
	return ST_OK;
}

int
st_add_handle_node(
		struct list_head *pStruNode
)
{
	st_master_handle_data( pStruNode );
	return st_add_thread_pool_node(giThreadID, pStruNode);
}

static int
st_response_error(
	int iErrorCode
)
{
	switch(iErrorCode)
	{
		case -1:
			return ST_ERR;
		case -2:
			return ST_FAIL_WRONG_TOKEN;
		case -3:
			return ST_FAIL_TIMEOUT_PEER_NOT_CONNECT_TO_SERVER;
		case -4:
			return ST_FAIL_WRONG_ROUTER_ID;
		case -5:
			return ST_FAIL_PEER_NOT_CONNECT_TO_SERVER;
		case -6:
			return ST_FAIL_NO_AUTHORITY;
		case -7:
			return ST_FAIL_NO_FLOW;
		default:
			return ST_FAIL_SERVER_ERROR;
	}
}

static void
st_get_cur_status(
	PushClient *pStruPC,
	int				 *piStatus
)
{
	int i = 0;
	int iNum = 0;

	if( !pStruPC->pStruAssistantMutex )
	{
		return;
	}
	pthread_mutex_lock(pStruPC->pStruAssistantMutex);
	iNum = *(pStruPC->pusAssistantCnt);
	pthread_mutex_unlock(pStruPC->pStruAssistantMutex);

	for( ; i < iNum; i++ )
	{
		PC_DEBUG("sockfd = %d = %d\n", pStruPC->iCurSockfd, pStruPC->pStruAssistantInfo[i].iAssistantSockfd);
		if( pStruPC->iCurSockfd == pStruPC->pStruAssistantInfo[i].iAssistantSockfd ||
				pStruPC->iCurSockfd == pStruPC->pStruAssistantInfo[i].iTestSockfd
		  )
		{
			(*piStatus) = pStruPC->pStruAssistantInfo[i].iStatus;
			PC_DEBUG("status = %d\n", (*piStatus));
			return;
		}
	}

	//(*piStatus) = pStruPC->uiStatus;
}

static int
st_handle_response(
	cJSON			 *pStruRoot, 
	PushClient *pStruPC
)
{
	int i = 0;
	int iRet = 0;
	int iNum = sizeof(gStruHF) / sizeof(gStruHF[0]);
	int iStatus = 0;

	pthread_mutex_lock(pStruPC->pStruResultMutex);
	iStatus = *(pStruPC->puiStatus);
	pthread_mutex_unlock(pStruPC->pStruResultMutex);
	//PC_ERROR("ip = %s:%d\n", pStruPC->struClientIP, pStruPC->usClientPort);
	iRet = cJSON_GetNumItem(pStruRoot, "errcode");
	PC_DEBUG("\n");
	if( iRet != 0 )
	{
		st_set_fail(st_get_step_by_status(iStatus), st_response_error(iRet), pStruPC);
		return iRet;
	}

	st_get_cur_status(pStruPC, &iStatus);
	//PC_ERROR("iStatus = %d\n", iStatus);
	
	for( ; i < iNum; i++ )
	{
		if(iStatus == gStruHF[i].uiStatus)
		{
			return gStruHF[i].pFunc(pStruRoot, pStruPC);
		}
	}

	//PC_DEBUG("no func for status %d\n", pStruPC->uiStatus);
//	st_set_fail(st_get_step_by_status(pStruPC->uiStatus),ST_ERR, pStruPC);

	return ST_ERR;
}

static int
st_handle_request(
	cJSON			 *pStruRoot, 
	PushClient *pStruPC
)
{
	int i = 0;
	int iLen = 0;
	int iNum = sizeof(gStruHF) / sizeof(gStruHF[0]);
	int iRet = 0;
	char *sTmp = NULL;

	//PC_ERROR("\n");
	sTmp = cJSON_GetStringItem(pStruRoot, "cmd");
	iLen = strlen(sTmp) + 1;
	for( ; i < iNum; i++ )
	{
		PC_DEBUG("i = %d\n", i );
		if( gStruHF[i].sFuncName && !strncmp(sTmp, gStruHF[i].sFuncName, iLen) )
		{
			//PC_ERROR("\n");
			return gStruHF[i].pFunc(pStruRoot, pStruPC);
		}
	}
	//PC_DEBUG("no func for status %d\n", pStruPC->uiStatus);
	return ST_ERR;
}

static int
st_get_json_root_type(
	char	 *sKey,
	char	 *sRawData, 
	char   *sData,
	int		 *piType,
	cJSON  **pStruRoot

)
{
	int iRet = 0;
	char sRsaData[512] = { 0 };

	iRet = base64_decode(sRawData, sRsaData);
	decrypt(sRsaData, iRet, sData, sKey);

	(*pStruRoot) = cJSON_Parse( sData );
	if( !(*pStruRoot) )
	{
		PC_ERROR("\n");
		return ST_FAIL_CANNOT_PARSE_JSON;
	}

	if( strstr(sData, "errcode") )
	{
		*piType = ST_TYPE_RESPONSE;
	}
	else if( strstr(sData, "cmd") )
	{
		*piType = ST_TYPE_REQUEST;
	}
	else
	{
		*piType = ST_PTYPE_MAX;
	}

	return ST_OK;
}

static int
st_handle_data(
	struct list_head *pStruNode	
)
{
	int iRet = 0;
	int iType = 0;
	static int iBootCnt = 0, iRegisterCnt = 0, iHubCnt = 0;
	char sData[512] = { 0 };
	char *sTmp = NULL;
	cJSON *pStruRoot;
	PushClient struPC, *pStruPC = NULL;
	PCHubNode  *pStruHN;
	static pthread_mutex_t struTmpMutex = PTHREAD_MUTEX_INITIALIZER;

	PC_DEBUG("\n");
	if( !pStruNode )
	{
		return ST_ERR;
	}

	pStruHN = list_entry(pStruNode, PCHubNode, struNode);
				
	pStruPC = (PushClient *)pStruHN->pData;
	memset(&struPC, 0, sizeof(struPC));
	PC_DEBUG("iSockfd = %d\n", pStruHN->iSockfd);
	st_read_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &struPC);
	struPC.ulData = (unsigned long)pStruHN->pEvent;

	iRet = st_get_json_root_type(struPC.sRandKey, pStruHN->sData, sData, &iType, &pStruRoot);
	if( iRet != ST_OK )
	{
		goto err;
	}
	struPC.iCurSockfd = pStruHN->iSockfd;

	if( iType == ST_TYPE_RESPONSE )
	{
		iRet = st_handle_response(pStruRoot, &struPC);
	}
	else if( iType == ST_TYPE_REQUEST )
	{
		iRet = st_handle_request(pStruRoot, &struPC);
	}
	else
	{
		iRet = ST_ERR;
	}
err:
	if( iRet != ST_OK )
	{
		PC_ERROR("status = %d\n", struPC.uiStatus);
		PC_ERROR("sKey = %s\n", struPC.sRandKey);
		PC_ERROR("sRaw data = %s\n", pStruHN->sData);
		PC_ERROR("sRecv = %s\n", sData);
		st_set_fail(st_get_step_by_status(struPC.uiStatus), iRet, &struPC);
	}
end:
	if( pStruRoot )
	{
		cJSON_Delete(pStruRoot);
	}	
	if( pStruHN->sData )
	{
	//	PC_ERROR("sData = %p\n", pStruHN->sData);
		free(pStruHN->sData);
		pStruHN->sData = NULL;
	}
	free(pStruHN);
	pStruHN = NULL;

	return ST_OK;
}

void
st_handle_node_free(
	struct list_head *pStruNode
)
{
	PCHubNode *pStruHN = NULL;

	if(!pStruNode)
	{
		return;
	}

	pStruHN = list_entry(pStruNode, PCHubNode, struNode);
	if( pStruHN->sData )
	{
		free(pStruHN->sData);
		pStruHN->sData = NULL;
	}

	free(pStruHN);

	return;
}

int 
st_get_handle_threadid()
{
	return giThreadID;
}

int
st_init_handle(int iThreadNum)
{
	return st_create_thread_pool(
															iThreadNum, 
															st_handle_node_free, 
															NULL,
															st_handle_data,
															&giThreadID
														);
}

/*
void*
st_handle_data( 
	void *pData 
)
{
	free(pData);
	pthread_detach(pthread_self()); //设置县城为分离模式

  //循环等待处理
	while( 1 )
	{
		pStruNode = NULL;
		pStruRoot = NULL;

		//检测是否需要退出
		iRet = st_check_exit();
		if( iRet == ST_OK )
		{	
			//退出线程
			break;
		}

		//从处理链表中获取数据	
		iRet = st_get_thread_group_node(
																giThreadID, 
																iHandleId, 
																&struNode,
																1
															);
		if( iRet != ST_OK )
		{
			if( iRet == ST_FAIL_TIMEOUT )
			{
				continue;
			}
			break;
		}
		
	
	}	

	if( pStruNode )
	{
		struPC.uiResult = ST_RESULT_FAIL;
		st_write_data_by_sock(pStruNode->iSockfd, &struPC);
	}

	pthread_exit(NULL);
}
*/
