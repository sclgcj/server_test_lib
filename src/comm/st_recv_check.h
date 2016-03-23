#ifndef ST_RECV_CHECK_H
#define ST_RECV_CHECK_H 1

#include "push_client.h"

typedef struct _PCCheckNode
{
	int iOffset;
	struct list_head struNode;
}PCCheckNode, *PPCCheckNode;



#define ST_CHECK_SIZE  150000
#define ST_CHECK_TICK_STOP  127

int 
st_init_recv_check();

int 
st_uninit_recv_check();

int
st_add_recv_check(
	PushClient *pStruPC
);

void 
st_stop_recv_check(int iSockfd, PushClient *pStruPC);

void
st_start_recv_check(int iSockfd, PushClient *pStruPC);

#endif
