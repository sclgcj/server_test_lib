#include "manage.h"
#include "m_error.h"
#include "m_status.h"
#include "m_boot.h"
#include "m_recv.h"
#include "m_send.h"
#include "m_create.h"
#include "m_dispose.h"
#include "m_unix_handle.h"
#include "ml_manage_listen.h"

#define M_PROJECT_FILE_PATH  "/var/m_prject_file"

void
m_free_mlink(
	MLink *pStruML
)
{
	if( !pStruML )
	{
		return;
	}
	ml_manager_del_sockfd(pStruML->iSockfd, pStruML->pStruM->struHandle);
	close(pStruML->iSockfd);
	m_destroy_proj_info(pStruML->pStruProjInfo);
	ML_FREE(pStruML->pStruProjInfo);
	pthread_mutex_destroy(&pStruML->struLinkMutex);
	ML_FREE(pStruML);
}

static void
m_listen_comm(
	void *pData
)
{
	ML_ERROR("\n");
	m_free_mlink((MLink*)pData);
}

void
m_calloc_mlink(
	int						 iSockfd,
	int						 iLinkType,
	struct in_addr struAddr,
	unsigned short usPort,
	MLRecvFunc     pRecvFunc,
	MLSendFunc     pSendFunc,
	MBase					 *pStruM,
	MLink					 **ppStruML
)
{
	ML_CALLOC((*ppStruML), MLink, 1);
	(*ppStruML)->iSockfd						 = iSockfd;
	(*ppStruML)->struAddr.sin_addr   = struAddr;
	(*ppStruML)->struAddr.sin_port   = htons(usPort);
	(*ppStruML)->struAddr.sin_family = AF_INET;
	(*ppStruML)->pStruM							 = pStruM;
	if( iLinkType == M_LINK_TYPE_LISTEN || iLinkType == M_LINK_TYPE_UNIX_LISTEN )
	{
		(*ppStruML)->pRecvFunc = pRecvFunc;
	}
	else
	{
		(*ppStruML)->pRecvFunc = pRecvFunc;
		(*ppStruML)->pSendFunc = pSendFunc;
		(*ppStruML)->pDisposeFunc = m_dispose;
	}
	pthread_mutex_init(&((*ppStruML)->struLinkMutex), NULL);
}

static int
m_manager_lib_init(
	MBase *pStruServer 
)
{
	int iRet = 0;

	iRet = ml_manager_create_exit(pStruServer->struConf.iDurationTime, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_client_data(pStruServer->struConf.iClientNum, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_thread(0, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_timer(0, 0, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = m_create_link( 
							(void *)pStruServer,
							&pStruServer->struConf,
							pStruServer->struHandle
						);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_recv_check(1, 60, 0, NULL,pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_dispose(0, 0, m_dispose, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_send(0, 0, m_send, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_recv(0, 0, m_recv, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	pStruServer->struOper.pEpollErrFunc = m_listen_comm;
	pStruServer->struOper.pEpollRDHupFunc = m_listen_comm;
	pStruServer->struOper.pEpollHupFunc = m_listen_comm;
	iRet = ml_manager_create_listener(
																	1000,
																	0,
																	&pStruServer->struOper,
																	pStruServer->struHandle
															);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_result(
															1,
															NULL,
															"./result.txt",
															NULL,
															pStruServer->struHandle
														);

	iRet = ml_manager_create_data(pStruServer->struConf.iMLDataNum, pStruServer->struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}
}

static void
m_init(
	MBase *pStruServer 
)
{
	int iRet = 0;	

	iRet = m_create_proj_file(
												pStruServer->struConf.iProjNum,
												pStruServer->struConf.iClearFile,
												pStruServer->struConf.sResultPath,
												pStruServer->struConf.sProjFilePath,
												pStruServer->struHandle,
												&pStruServer->struPA
											);
	if( iRet != ML_OK )
	{
		exit(0);
	}

	m_create_dispose(&pStruServer->struDispose);
	m_add_dispose(-1, "Boot", m_boot_handle_request, &pStruServer->struDispose);
	m_add_dispose(M_STATUS_SEND_BOOT, NULL, m_boot_handle_response, &pStruServer->struDispose);
	m_add_dispose(-1, "Test", m_unix_test, &pStruServer->struDispose);
}

int 
main(
	int  iArgc,
	char **ppArgv
)
{
	int iRet = 0;
	MBase struServer;
	MLListenOp struListenOper;

	if( iArgc < 3 )
	{		
		ML_ERROR("usage: %s ip port", ppArgv[0]);
		return 1;
	}

	memset(&struServer, 0, sizeof(struServer));

	ml_create_manager(&struServer.struHandle);

	iRet = ml_manager_create_opt_config(iArgc, ppArgv, "df:t:", struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}
	m_get_opt_config(&struServer.struMOC, struServer.struHandle);

	m_handle_opt(&struServer.struMOC);

	iRet = ml_manager_create_read_config(struServer.struMOC.sConfigFile, struServer.struHandle);
	if( iRet !=	ML_OK )
	{
		return iRet;
	}
	m_get_config(&struServer.struConf, struServer.struHandle);

	iRet = m_manager_lib_init(&struServer);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	m_init(&struServer);

	return ml_manage_start(struServer.struHandle);
}

