#ifndef ML_RECV_H
#define ML_RECV_H 1

#include "ml_dispose.h"

typedef void * MLRecvHandle;
typedef int (*MLRecvFunc)(void *pEventData, int *piRecvLen, char **ssRecvData);

int
ml_create_recv_handle(
	int iThreadNum,
	int	iStackSize,
	MLRecvFunc		 pFunc,
	MLThreadHandle struThreadHandle,
	MLDisposeHandle struDisposeHandle,
	MLRecvHandle *pStruHandle	
);

void
ml_destroy_recv(
	MLRecvHandle struRecvHandle
);

int
ml_add_recv_node(
	void *pUserData,
	MLRecvHandle struHandle
);

#endif
