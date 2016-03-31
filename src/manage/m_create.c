#include "manage.h"
#include <fcntl.h>
#include <sys/un.h>

#define M_CREATE_UNIX_PATH "/tmp/m_unix_path"

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
	struAddr.sin_addr = struIP;
	struAddr.sin_port = htons(usPort);
	struAddr.sin_family = AF_INET;

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

	ML_ERROR("ip = %s:%d\n", inet_ntoa(struAddr), usPort);
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
	struAddr.sin_family = AF_INET;
	
	ML_ERROR("server_ip = %s;%d\n", inet_ntoa(struAddr.sin_addr), ntohs(struAddr.sin_port));
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

static void
m_create_unix_sockfd(
	char *sUnixPath,
	int  *piSockfd
)
{
	int iSockfd = 0;
	struct sockaddr_un struUN;

	iSockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if( iSockfd < 0 )
	{
		ML_ERROR("create unix socket error: 5s\n", strerror(errno));
		exit(0);
	}

	unlink(sUnixPath);
	memset(&struUN, 0, sizeof(struUN));
	memcpy(struUN.sun_path, sUnixPath, strlen(sUnixPath));
	struUN.sun_family = AF_UNIX;

	if( bind(iSockfd, (struct sockaddr*)&struUN, sizeof(struUN)) < 0 )
	{
		ML_ERROR("bind unix path %s error: %s\n", sUnixPath, strerror(errno));
		exit(0);
	}

	(*piSockfd) = iSockfd;
}

static void
m_create_unix_listen(
	char  *sUnixPath,
	MBase *pStruMB
)
{
	int iLen = 0;
	int iSockfd = 0;
	struct in_addr struAddr;
	MLink *pStruML = NULL;

	m_create_unix_sockfd(sUnixPath, &iSockfd);

	if( listen(iSockfd, 10) < 0 )
	{
		ML_ERROR("listen error : %s\n", strerror(errno));
		exit(0);
	}

	struAddr.s_addr = 0;
	m_calloc_mlink(iSockfd, M_LINK_TYPE_UNIX_LISTEN, struAddr, 0, pStruMB, &pStruML);

	ml_manager_add_client_data((void*)pStruML, pStruMB->struHandle, &pStruML->iDataID);

	ml_manager_add_sockfd(
									(EPOLLONESHOT | EPOLLET | EPOLLIN),
									(iSockfd),
									(void*)pStruML,
									pStruMB->struHandle
							);
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

	if( pStruConf->iUnixListen )
	{
		m_create_unix_listen(M_CREATE_UNIX_PATH, pStruMB);
	}

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

	ml_manager_add_client_data((void*)pStruML, pStruMB->struHandle, &pStruML->iDataID);

	ml_manager_add_sockfd(
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

