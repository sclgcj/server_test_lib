#ifndef ST_SEND_H
#define ST_SEND_H 1

#include "st_thread.h"

typedef void * STSendHandle;
typedef void (*STSendFunc)(void *pUserData);

void
st_create_send_handle(
	int iThreadNum,
	int iStackSize,
	STSendFunc pFunc,
	STThreadHandle struThreadHandle,
	STSendHandle *pStruSendHandle
);

void
st_destroy_send_handle(
	STSendHandle struSendHandle
);

int
st_add_send_node(
	void *pUserData,
	STSendHandle struSendHandle
);

#endif
