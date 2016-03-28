#ifndef M_RECV_H
#define M_RECV_H 1

#include "manage.h"

int
m_recv(
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
);

#endif
