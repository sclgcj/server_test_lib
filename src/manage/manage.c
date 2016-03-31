#include "manage.h"
#include "m_recv.h"
#include "m_send.h"
#include "m_dispose.h"
#include "m_create.h"

#define M_PROJECT_FILE_PATH  "/var/m_prject_file"

void
m_calloc_mlink(
	int						 iSockfd,
	int						 iLinkType,
	struct in_addr struAddr,
	unsigned short usPort,
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
	if( iLinkType == M_LINK_TYPE_LISTEN )
	{
		(*ppStruML)->pRecvFunc = m_recv;
	}
	else
	{
		(*ppStruML)->pRecvFunc = m_recv;
		(*ppStruML)->pSendFunc = m_send;
		(*ppStruML)->pDisposeFunc = m_dispose;
	}
	pthread_mutex_init(&((*ppStruML)->struLinkMutex), NULL);
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

	iRet = ml_manager_create_exit(struServer.struConf.iDurationTime, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_client_data(struServer.struConf.iClientNum, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_thread(0, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_timer(0, 0, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = m_create_link( 
							(void *)&struServer,
							&struServer.struConf,
							struServer.struHandle
						);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_recv_check(1, 60, 0, NULL,struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_dispose(0, 0, m_dispose, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_send(0, 0, m_send, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_recv(0, 0, m_recv, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_listener(
																	1000,
																	0,
																	NULL,
																	struServer.struHandle
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
															struServer.struHandle
														);

	iRet = ml_manager_create_data(struServer.struConf.iMLDataNum, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	return ml_manage_start(struServer.struHandle);
}

