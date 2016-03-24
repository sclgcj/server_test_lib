#ifndef ST_RECV_CHECK_H
#define ST_RECV_CHECK_H 1

#define ST_CHECK_SIZE  150000
#define ST_CHECK_TICK_START  0
#define ST_CHECK_TICK_STOP   127

typedef void * STRecvCheckHandle;
typedef void (*STRecvCheckFailFunc)(void *pUserData);

int
st_create_recv_check(
	int iTotalLink,	
	int iRecvTimeout,
	int iCheckListNum,
	STTimerHandle struHandle,
	STRecvCheckFailFunc pFunc,
	STRecvCheckHandle *pStruHandle
);

void
st_destroy_recv_check(
	STRecvCheckHandle struHandle
);

int
st_add_recv_check(
	void              *pUserData,
	STRecvCheckHandle struHandle,
	unsigned long     *piRCID
);

#endif
