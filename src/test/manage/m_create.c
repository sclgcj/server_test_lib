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

static int
m_create_socket(
	int						 iType,	
	struct in_addr struIP,
	unsigned short usPort,
	int						 *piSockfd
)
{
	int iRet = 0;
	int iSize = 0;
	struct sockaddr_in struAddr;

	(*piSockfd) = socket(AF_INET, iType, 0);

	if( (*piSockfd) < 0 )
	{
		ML_ERROR("socket error : %s\n", strerror(errno));
		return ML_ERR;
	}

	iSize = sizeof(struct sockaddr_in);
	memset(&struAddr, 0, iSize);

	if( bind((*piSockfd), (struct sockaddr*)&struAddr, iSize) < 0 )
	{
		ML_ERROR("bind_sock error:%s :%d error: %s\n", 
				inet_ntoa(struIP), usPort, strerror(errno));
		return ML_ERR;
	}

	return ML_OK;
}

static void
m_create_tcp_listen(
	struct in_addr struAddr,
	unsigned short usPort,
	MBase          *pStruM,
	int						 *piListenFd
)
{
	int iRet = 0;

	iRet = m_create_socket( SOCK_STREAM, struAddr, usPort, piListenFd );
	if( iRet != ML_OK )
	{
		exit(0);
	}

	iRet = m_set_listen_sockopt((*piListenFd));
	if( iRet != ML_OK )
	{
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
m_set_sockopt(
	int iSockfd
)
{
	int iReuseaddr = 1;
	int	iBufferSize = 32 * 1024;
	struct timeval struST;
	struct linger struLinger;

	if(setsockopt(iSockfd, SOL_SOCKET ,SO_REUSEADDR, (const char*)&iReuseaddr, sizeof(int)) < 0)
	{
		ML_ERROR("set SO_REUSEADDR fail:[%d] %s\n", iSockfd, strerror(errno));
		return ML_ERR;
	}

/*	struLinger.l_onoff = 1;
	struLinger.l_linger = 0;
	if( setsockopt(iSockfd, SOL_SOCKET, SO_LINGER, (void*)&struLinger, sizeof(struLinger)) )
	{
		PC_ERROR("set SO_LINGER fail: [%d] %s\n", iSockfd, strerror(errno));
		return PUSH_CLIENT_ERR;
	}*/

	if(setsockopt(iSockfd, SOL_SOCKET, SO_RCVBUF, (char*)&iBufferSize, sizeof(iBufferSize)))
	{
		ML_ERROR("setsockopt error:%s\n", strerror(errno));
		return ML_ERR;
	}
	if( m_set_nonblock_fd(iSockfd) != ML_OK )
	{
		ML_ERROR("set nonblock_fd error\n");
		return ML_ERR;
	}

	return ML_OK;
}

static int 
m_tcp_connect(
	int						 iSockfd,
	struct in_addr struServerAddr, 
	unsigned short usServerPort
)
{
	int iRet = 0;
	int iSize = 0;
	struct sockaddr_in struAddr;

	iSize = sizeof(struct sockaddr_in);
	memset(&struAddr, 0, iSize);
	struAddr.sin_addr = struServerAddr;
	struAddr.sin_port = htons(usServerPort);
	
	while( 1 )
	{
		iRet = connect(iSockfd, (struct sockaddr*)&struAddr, iSize);
		if( iRet < 0 )
		{
			if( errno == EISCONN )
			{
				return ML_OK;
			}
			else if( errno == EINTR )
			{
				continue;
			}
			else if(errno != EINPROGRESS && errno != EALREADY && errno != EWOULDBLOCK )
			{
				ML_ERROR("conect error: %s\n", strerror(errno));
				return ML_ERR;
			}
		}
		break;
	}

	return ML_OK;
}

static void
m_create_tcp_connect(
	struct in_addr struAddr,
	unsigned short usPort,
	struct in_addr struServerAddr,
	unsigned short usServerPort,
	MBase          *pStruMB,
	int						 *piSockfd
)
{
	int iRet = 0;		

	iRet = m_create_socket(SOCK_STREAM, struAddr, usPort, piSockfd);
	if( iRet != ML_OK )
	{
		exit(0);
	}

	iRet = m_set_sockopt((*piSockfd));
	if( iRet != ML_OK )
	{
		exit(0);
	}

	iRet = m_tcp_connect((*piSockfd), struServerAddr, usServerPort);
	if( iRet != ML_OK )
	{
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
	int iType = 0;
	int iEvent = 0;
	int iSockfd = 0;
	MBase *pStruMB = (MBase *)pUserData;
	MLink *pStruML = NULL;
	MConfig *pStruConf = &pStruMB->struConf;

	if( pStruConf->iDevType == M_DEV_TYPE_SERVER )
	{
		m_create_tcp_listen(struAddr, usPort, pStruMB, &iSockfd);
		iEvent = (EPOLLONESHOT | EPOLLET | EPOLLIN);
		iType  = M_LINK_TYPE_LISTEN;
	}
	else
	{
		m_create_tcp_connect(
											struAddr, 
											usPort, 
											pStruConf->struServerAddr, 
											pStruConf->usServerPort, 
											pStruMB, 
											&iSockfd
										);
		iEvent = (EPOLLONESHOT | EPOLLET | EPOLLOUT);
		iType  = M_LINK_TYPE_NORMAL;
	}

	m_calloc_mlink(iSockfd, iType, struAddr, usPort, pStruMB, &pStruML);

	return ml_manager_add_sockfd(
									iEvent,
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

