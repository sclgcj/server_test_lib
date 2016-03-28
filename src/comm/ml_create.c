
#include "ml_comm.h"
#include "ml_hub.h"
#include "ml_create.h"
//#include "ml_recv_check.h"
#include "semaphore.h"
#include <sys/epoll.h>
//#include "ml_message.h"


typedef struct _PCLinkHandleData
{
	int iRID;
	char sUserID[64];
	char sToken[256];
}PCLinkHandleData, *PPCLinkHandleData;

typedef struct _MLCreateLink
{
	int iCLThreadID;
	int iCHThreadID;
	void *pUserData;
	pthread_mutex_t struCLMutex;
	MLCLParam			 struParam;
	MLThreadHandle struThreadHandle;
	MLCreateLinkFunc pCLFunc;
}MLCreateLink, *PMLCreateLink;

typedef struct _MLCreateLinkData
{
	MLCreateLink *pStruCL;
	struct list_head struNode;
}MLCreateLinkData, *PMLCreateLinkData;

typedef struct _MLCLHData
{
	int						 iCLID;
	unsigned short usPort;
	struct in_addr struAddr; //unsigned int uiIP;
	struct list_head struNode;
	MLCreateLink   *pStruCL;
}MLCLHData,*PMLCLHData;

static void
ml_add_create_link_node(
	int						 iID,
	unsigned int   uiIP,
	unsigned short usPort,
	MLCreateLink   *pStruCL
)
{
	struct in_addr struAddr;
	MLCLHData *pStruCLHData = NULL;
	
	struAddr.s_addr = uiIP;
	ML_CALLOC(pStruCLHData, MLCLHData, 1);
	pStruCLHData->iCLID		 = iID;
	pStruCLHData->usPort    = usPort;
	pStruCLHData->struAddr  = struAddr;
	pStruCLHData->pStruCL   = pStruCL;

	ml_add_table_thread_pool_node(pStruCL->iCHThreadID, &pStruCLHData->struNode, pStruCL->struThreadHandle);
}

static int
ml_start_create_link_thread(
	struct list_head *pStruNode
)
{
	int i = 0, j = 0;
	int iRet = 0;
	int iCount = 0;
	int iOffset = 0;
	unsigned short usEndPort = 0;
	unsigned int uiStartIP = 0;
	MLCreateLinkData *pStruCLD = NULL;
	MLCreateLink *pStruCL = NULL;

	pStruCLD = list_entry(pStruNode, MLCreateLinkData, struNode);
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
			iRet = ml_check_exit();
			if( iRet == ML_OK )
			{
				i = pStruCL->struParam.iIpCount;
				j = usEndPort;
				continue;
			}
			ml_add_create_link_node(iCount, uiStartIP, j, pStruCL);	

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
ml_free_create_link_handle_node(
	struct list_head *pStruHead
)
{
	MLCLHData *pStruCLD = NULL;

	if( !pStruHead )
	{
		return;
	}

	pStruCLD = list_entry(pStruHead, MLCLHData, struNode);

	ML_FREE(pStruCLD);
}

static void
ml_free_create_link_node(
	struct list_head *pStruHead
)
{
	MLCreateLinkData *pStruCLD = NULL;

	pStruCLD = list_entry(pStruHead, MLCreateLinkData, struNode);

	ML_FREE(pStruCLD);
}

static int 
ml_create(
	struct list_head *pStruNode
)
{
	MLCLHData *pStruCLD = NULL;
	MLCreateLink *pStruCL = NULL;

	pStruCLD = list_entry(pStruNode, MLCLHData, struNode);
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

	return ML_OK;
}

int
ml_create_link_handle(
	void						  *pUserData,
	MLCLParam			 	  *pStruCLParam,
	MLCreateLinkFunc  pCLFunc,	
	MLThreadHandle	  struThreadHandle,
	MLCLHandle		 	  *pStruHandle
)
{
	MLCreateLink *pStruCL = NULL;

	ML_CALLOC(pStruCL, MLCreateLink, 1);
	memcpy(&pStruCL->struParam, pStruCLParam, sizeof(MLCLParam));
	pStruCL->pUserData        = pUserData;
	pStruCL->struThreadHandle = struThreadHandle;
	pStruCL->pCLFunc					= pCLFunc;
	pthread_mutex_init(&pStruCL->struCLMutex, NULL);

	ml_create_thread_table_pool(
													1,
													pStruCLParam->iStackSize,				
													ml_free_create_link_node,
													NULL,
													ml_start_create_link_thread,
													struThreadHandle,
													&pStruCL->iCLThreadID
											);
	ml_create_thread_table_pool(
													pStruCLParam->iThreadNum,
													pStruCLParam->iStackSize,
													ml_free_create_link_handle_node,
													NULL,
													ml_create,
													struThreadHandle,
													&pStruCL->iCHThreadID
											);

	(*pStruHandle) = (MLCLHandle)pStruCL;

	return ML_OK;
}

void
ml_destroy_link_handle(
	MLCLHandle struHandle
)
{
	MLCreateLink *pStruCL = (MLCLHandle)struHandle;

	if( !struHandle )
	{
		return;
	}

	pthread_mutex_destroy(&pStruCL->struCLMutex);

	ML_FREE(pStruCL);
}

int
ml_start_create_link(
	MLCLHandle struHandle	
)
{
	MLCreateLink *pStruCL = (MLCLHandle)struHandle;
	MLCreateLinkData *pStruCLD = NULL;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	ML_CALLOC(pStruCLD, MLCreateLinkData, 1);
	pStruCLD->pStruCL = pStruCL;

	ml_add_table_thread_pool_node(pStruCL->iCLThreadID, &pStruCLD->struNode, pStruCL->struThreadHandle);

	return ML_OK;
}

