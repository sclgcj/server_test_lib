#ifndef MANAGE_H
#define MANAGE_H 1

#include "ml_comm.h"
#include "ml_manage.h"

#include "m_proj.h"
#include "m_error.h"
#include "m_status.h"
#include "m_dispose.h"
#include "m_handle_opt.h"
#include "m_read_config.h"

#include <sys/epoll.h>

#define M_DEFAULT_DURATION_TIME  10

//type of element
enum
{
	M_CTYPE_INT,
	M_CTYPE_UINT,
	M_CTYPE_CHAR,
	M_CTYPE_UCHAR,
	M_CTYPE_SHORT,
	M_CTYPE_USHORT,
	M_CTYPE_STRING,
	M_CTYPE_IPADDR,
	M_CTYPE_FUNC,
	M_CTYPE_MAX
};

typedef struct _MBase
{
	MConfig    struConf;
	MDispose   struDispose;
	MOptConfig struMOC;
	MProjArray struPA;
	MLHandle struHandle;			
	MLListenOp  struOper;
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
	MProjInfo  *pStruProjInfo;
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
);

void
m_free_mlink(
	MLink *pStruML
);

#endif

