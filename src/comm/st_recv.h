#ifndef ST_RECV_H
#define ST_RECV_H 1

typedef void * STRecvHandle;
typedef int (*STRecvFunc)(void *pEventData);

int
st_create_recv_handle(
	int iThreadNum,
	int	iStackSize,
	STRecvFunc		 pFunc,
	STThreadHandle struThreadHandle,
	STRecvHandle *pStruHandle	
);

void
st_destroy_recv(
	STRecvHandle struRecvHandle
);

int
st_add_recv_node(
	void *pUserData,
	STRecvHandle struHandle
);

#endif
