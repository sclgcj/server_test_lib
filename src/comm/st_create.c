
#include "st_comm.h"
#include "st_hub.h"
#include "st_create.h"
//#include "st_recv_check.h"
#include "semaphore.h"
#include <sys/epoll.h>
//#include "st_message.h"


typedef struct _PCLinkHandleData
{
	int iRID;
	char sUserID[64];
	char sToken[256];
}PCLinkHandleData, *PPCLinkHandleData;

typedef struct _STCreateLink
{
	int iCLThreadID;
	int iCHThreadID;
	void *pUserData;
	pthread_mutex_t struCLMutex;
	STCLParam			 struParam;
	STThreadHandle struThreadHandle;
	STCreateLinkFunc pCLFunc;
}STCreateLink, *PSTCreateLink;

typedef struct _STCreateLinkData
{
	STCreateLink *pStruCL;
	struct list_head struNode;
}STCreateLinkData, *PSTCreateLinkData;

typedef struct _STCLHData
{
	int						 iCLID;
	unsigned short usPort;
	struct in_addr struAddr; //unsigned int uiIP;
	struct list_head struNode;
	STCreateLink   *pStruCL;
}STCLHData,*PSTCLHData;

static void
st_add_create_link_node(
	int						 iID,
	unsigned int   uiIP,
	unsigned short usPort,
	STCreateLink   *pStruCL
)
{
	struct in_addr struAddr;
	STCLHData *pStruCLHData = NULL;
	
	struAddr.s_addr = uiIP;
	ST_CALLOC(pStruCLHData, STCLHData, 1);
	pStruCLHData->iCLID		 = iID;
	pStruCLHData->usPort    = usPort;
	pStruCLHData->struAddr  = struAddr;
	pStruCLHData->pStruCL   = pStruCL;

	st_add_table_thread_pool_node(pStruCL->iCHThreadID, &pStruCLHData->struNode, pStruCL->struThreadHandle);
}

static int
st_start_create_link_thread(
	struct list_head *pStruNode
)
{
	int i = 0, j = 0;
	int iRet = 0;
	int iCount = 0;
	int iOffset = 0;
	unsigned short usEndPort = 0;
	unsigned int uiStartIP = 0;
	STCreateLinkData *pStruCLD = NULL;
	STCreateLink *pStruCL = NULL;

	pStruCLD = list_entry(pStruNode, STCreateLinkData, struNode);
	pStruCL = pStruCLD->pStruCL;

	uiStartIP = pStruCL->struParam.struStartAddr.s_addr;
	iOffset   = pStruCL->struParam.iPortMapCnt;
	iCount    = pStruCL->struParam.usEndPort - pStruCL->struParam.usStartPort;
	usEndPort = (iCount - iCount % iOffset) + pStruCL->struParam.usStartPort;
	iCount    = 0;

	for( i = 0; i < pStruCL->struParam.iIpCount; i++ )
	{
		if( i != 0 )
		{
			uiStartIP += 1<<24;
		}
		for( j = pStruCL->struParam.usStartPort; j < usEndPort; j += iOffset )
		{
			iRet = st_check_exit();
			if( iRet == ST_OK )
			{
				i = pStruCL->struParam.iIpCount;
				j = usEndPort;
				continue;
			}
			st_add_create_link_node(iCount, uiStartIP, j, pStruCL);	

			iCount++;
			if( iCount >= pStruCL->struParam.iTotalLink )
			{
				i = pStruCL->struParam.iIpCount;
				break;
			}

			if( pStruCL->struParam.iCreateLinkTime > 0 )
			{
				if( iCount % pStruCL->struParam.iCreateLink == 0 )
				{
					sleep(pStruCL->struParam.iCreateLinkTime);
				}
			}
		}
	}

	pthread_exit(NULL);
}

static void
st_free_create_link_handle_node(
	struct list_head *pStruHead
)
{
	STCLHData *pStruCLD = NULL;

	if( !pStruHead )
	{
		return;
	}

	pStruCLD = list_entry(pStruHead, STCLHData, struNode);

	ST_FREE(pStruCLD);
}

static void
st_free_create_link_node(
	struct list_head *pStruHead
)
{
	STCreateLinkData *pStruCLD = NULL;

	pStruCLD = list_entry(pStruHead, STCreateLinkData, struNode);

	ST_FREE(pStruCLD);
}

static int 
st_create(
	struct list_head *pStruNode
)
{
	STCLHData *pStruCLD = NULL;
	STCreateLink *pStruCL = NULL;

	pStruCLD = list_entry(pStruNode, STCLHData, struNode);
	pStruCL = pStruCLD->pStruCL;

	if( pStruCL->pCLFunc )
	{
		pStruCL->pCLFunc(
								pStruCLD->iCLID, 
								pStruCLD->struAddr, 
								pStruCLD->usPort,
								pStruCL->pUserData 
							);
	}

	free(pStruCLD);
	pStruCLD = NULL;

	return ST_OK;
}

int
st_create_link_handle(
	void						  *pUserData,
	STCLParam			 	  *pStruCLParam,
	STCreateLinkFunc  pCLFunc,	
	STThreadHandle	  struThreadHandle,
	STCLHandle		 	  *pStruHandle
)
{
	STCreateLink *pStruCL = NULL;

	ST_CALLOC(pStruCL, STCreateLink, 1);
	memcpy(&pStruCL->struParam, pStruCLParam, sizeof(STCLParam));
	pStruCL->pUserData        = pUserData;
	pStruCL->struThreadHandle = struThreadHandle;
	pthread_mutex_init(&pStruCL->struCLMutex, NULL);

	st_create_thread_table_pool(
													1,
													pStruCLParam->iStackSize,				
													st_free_create_link_node,
													NULL,
													st_start_create_link_thread,
													struThreadHandle,
													&pStruCL->iCLThreadID
											);
	st_create_thread_table_pool(
													pStruCLParam->iThreadNum,
													pStruCLParam->iStackSize,
													st_free_create_link_handle_node,
													NULL,
													st_create,
													struThreadHandle,
													&pStruCL->iCHThreadID
											);

	(*pStruHandle) = (STCLHandle)pStruCL;

	return ST_OK;
}

void
st_destroy_link_handle(
	STCLHandle struHandle
)
{
	STCreateLink *pStruCL = (STCLHandle)struHandle;

	if( !struHandle )
	{
		return;
	}

	pthread_mutex_destroy(&pStruCL->struCLMutex);

	ST_FREE(pStruCL);
}

int
st_start_create_link(
	STCLHandle struHandle	
)
{
	STCreateLink *pStruCL = (STCLHandle)struHandle;
	STCreateLinkData *pStruCLD = NULL;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	ST_CALLOC(pStruCLD, STCreateLinkData, 1);
	pStruCLD->pStruCL = pStruCL;

	st_add_table_thread_pool_node(pStruCL->iCLThreadID, &pStruCLD->struNode, pStruCL->struThreadHandle);

	return ST_OK;
}

