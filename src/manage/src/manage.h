#ifndef MANAGE_H
#define MANAGE_H 1

#include "ml_comm.h"
#include "ml_manage.h"

#include "m_handle_opt.h"
#include "m_read_config.h"

#include <sys/epoll.h>

#define M_DEFAULT_DURATION_TIME  10

typedef struct _MBase
{
	MConfig    struConf;
	MOptConfig struMOC;
	MLHandle struHandle;			
}MBase, *PMBase;

typedef struct _MLink
{
	int				 iSockfd;
	int				 iDataID;
	int				 iLinkStatus;
	int				 iRecvCheckID;
	int				 iLinkType;
	struct sockaddr_in struAddr;
	MBase      *pStruM;
	MLRecvFunc pRecvFunc;
	MLSendFunc pSendFunc;
	MLDisposeFunc pDisposeFunc;
	pthread_mutex_t struLinkMutex;
}MLink, *PMLink;

enum
{
	M_LINK_TYPE_LISTEN,
	M_LINK_TYPE_NORMAL,
	M_LINK_TYPE_UNIX_LISTEN,
	M_LINK_TYPE_UNIX_NORMAL,
	M_LINK_TYPE_MAX
};

enum
{
	M_LINK_STATUS_CONNECT,
	M_LINK_STATUS_OK,
	M_LINK_STATUS_MAX
};

void
m_calloc_mlink(
	int						 iSockfd,
	int						 iLinkType,
	struct in_addr struAddr,
	unsigned short usPort,
	MBase					 *pStruM,
	MLink					 **ppStruML
);

#endif

