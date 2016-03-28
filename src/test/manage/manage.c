#include "manage.h"
#include "m_recv.h"
#include "m_send.h"
#include "m_dispose.h"
#include "m_create.h"

static char *gsArgFmt = "f:d";
static char *gsFilePath = "./config";

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

	ml_create_manager(10, &struServer.struHandle);

	iRet = ml_manager_create_opt_config(iArgc, ppArgv, "df:", struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}
	m_get_opt_config(&struServer.struMOC, struServer.struHandle);

	iRet = ml_manager_create_read_config(struServer.struMOC.sConfigFile, struServer.struHandle);
	if( iRet !=	ML_OK )
	{
		return iRet;
	}
	m_get_config(&struServer.struConf, struServer.struHandle);

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

	iRet = ml_manager_create_dispose(0, 0, NULL, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_send(0, 0, NULL, struServer.struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_recv(0, 0, NULL, struServer.struHandle);
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

	return ml_manage_start(struServer.struHandle);
}

