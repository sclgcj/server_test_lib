#include "st_comm.h"
#include "st_send.h"
#include "st_thread.h"

typedef struct _STSend
{
	int iThreadID;	
	STSendFunc pSendFunc;
	STThreadHandle struThreadHandle;
}STSend, *PSTSend;

static int
st_send(
	struct list_head *pStruNode
)
{
	STSend  *pStruS = NULL;
	STCommNode *pStruCN = NULL;

	pStruCN = list_entry(pStruNode, STCommNode, struNode);
	pStruS = (STSend *)pStruCN->ulData;

	if( pStruS->pSendFunc )
	{
		pStruS->pSendFunc(pStruCN->pUserData);
	}

	ST_FREE(pStruS);

	return ST_OK;
}

int
st_add_send_node(
	void *pUserData,
	STSendHandle struSendHandle
)
{
	STSend *pStruS = (STSend*)struSendHandle;
	STCommNode *pStruCN = NULL;

	if( !struSendHandle )
	{
		return ST_PARAM_ERR;
	}

	ST_CALLOC(pStruCN, STCommNode, 1);
	pStruCN->ulData = (unsigned long)pStruS;
	pStruCN->pUserData = pUserData;

	st_add_table_thread_pool_node(pStruS->iThreadID, &pStruCN->struNode, pStruS->struThreadHandle);

	return ST_OK;
}

void
st_create_send_handle(
	int iThreadNum,
	int iStackSize,
	STSendFunc pFunc,
	STThreadHandle struThreadHandle,
	STSendHandle *pStruSendHandle
)
{
	STSend *pStruS = NULL;

	ST_CALLOC(pStruS, STSend, 1);
	pStruS->pSendFunc = pFunc;

	st_create_thread_table_pool(
													iThreadNum,
													iStackSize,
													st_destroy_comm_node,
													NULL,
													st_send,
													struThreadHandle,
													&pStruS->iThreadID
												);

	(*pStruSendHandle) = (STSendHandle)pStruS;
}

void
st_destroy_send_handle(
	STSendHandle struSendHandle
)
{
	STSend *pStruS = (STSend *)struSendHandle;

	if( !struSendHandle )
	{
		return;
	}

	ST_FREE(pStruS);
}

