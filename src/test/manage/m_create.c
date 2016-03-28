#include "manage.h"
#include <fcntl.h>

static int
m_set_nonblock_fd(
	int iSockfd
)
{
	int iFlag = 0;

	iFlag = fcntl(iSockfd, F_GETFL);
	if( iFlag < 0 )
	{
		ML_ERROR("FCNTL error:%s\n", strerror(errno));
		return ML_ERR;
	}
	iFlag |= O_NONBLOCK;
	iFlag = fcntl(iSockfd,F_SETFL, iFlag);
	if( iFlag < 0 )
	{
		ML_ERROR("fcntl F_SETFL err: %s\n", strerror(errno));
	}

	return ML_OK;
}

static int
m_set_listen_sockopt(
	int iSockfd
)
{
	int iReuseaddr = 1;
	struct timeval struML;
	struct linger struLinger;

	if( setsockopt(iSockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&iReuseaddr, sizeof(int)) < 0 )
	{
		ML_ERROR("set SO_REUSEADDR error: [%d] %s\n", iSockfd, strerror(errno));
		return ML_ERR;
	}

	if( m_set_nonblock_fd(iSockfd) != ML_OK )
	{
		ML_ERROR("set nonblock_fderror\n");
		return ML_ERR;
	}
	
	return ML_OK;
}

static void
m_create_tcp_listen(
	struct in_addr struAddr,
	unsigned short usPort,
	MBase        *pStruM,
	int						 *piListenFd
)
{
	int iRet = 0;
	int iSize = sizeof(struct sockaddr_in);
	int iEvent = 0;
	void *pData = NULL;
	struct sockaddr_in struListenAddr;

	(*piListenFd) = socket(AF_INET, SOCK_STREAM, 0);
	if( (*piListenFd) < 0 )
	{
		ML_ERROR("socket error : %s\n", strerror(errno));
		exit(0);
	}

	struListenAddr.sin_addr.s_addr = struAddr.s_addr;
	struListenAddr.sin_port = ntohs(usPort);
	struListenAddr.sin_family = AF_INET;

	iRet = m_set_listen_sockopt((*piListenFd));
	if( iRet != ML_OK )
	{
		exit(0);
	}

	iRet = bind((*piListenFd), (struct sockaddr*)&struListenAddr, sizeof(struListenAddr));
	if( iRet < 0 )
	{
		ML_ERROR("bind error: %s\n", strerror(errno));
		exit(0);
	}	

	iRet = listen((*piListenFd), 10);
	if( iRet < 0 )
	{
		ML_ERROR("listen error: %s\n", strerror(errno));
		exit(0);
	}
}

static int
m_create_link_function(
	int						 iLinkID,
	struct in_addr struAddr,
	unsigned short usPort,
	void					 *pUserData
)
{
	int iSockfd = 0;
	MBase *pStruMB = (MBase *)pUserData;
	MLink *pStruML = NULL;

	ML_ERROR("-----------------\n");
	m_create_tcp_listen(struAddr, usPort, pStruMB, &iSockfd);

	m_calloc_mlink(iSockfd, M_LINK_TYPE_LISTEN, struAddr, usPort, pStruMB, &pStruML);

	return ml_manager_add_sockfd(
									(EPOLLONESHOT | EPOLLET | EPOLLIN),
									(iSockfd),
									(void*)pStruML,
									pStruMB->struHandle
							);
}

int
m_create_link(
	void    *pUserData,
	MConfig *pStruConf,
	MLHandle struHandle
)
{
	MLCLParam struParam;		

	memset(&struParam, 0, sizeof(struParam));
	struParam.iIpCount    = pStruConf->iIpCount;
	struParam.iTotalLink  = pStruConf->iTotalLink;
	struParam.iCreateLink = pStruConf->iCreateLink;
	struParam.iPortMapCnt = pStruConf->iPortMapCnt;
	struParam.usStartPort = pStruConf->usStartPort;
	struParam.usEndPort   = pStruConf->usEndPort;
	struParam.struStartAddr.s_addr = pStruConf->struStartAddr.s_addr;

	return ml_manager_create_link_handle(pUserData, &struParam, m_create_link_function, struHandle);
}

