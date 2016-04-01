#ifndef M_RECV_H
#define M_RECV_H 1

#include "manage.h"

int
m_recv(
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
);

int
m_unix_listen_function( 
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
);

int
m_listen_function(
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
);

int 
m_recv_function(
	void *pData,
	int  *piRecvLen,
	char **ssRecvData	
);

#endif
