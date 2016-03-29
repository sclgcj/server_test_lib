#ifndef ML_RECV_CHECK_H
#define ML_RECV_CHECK_H 1

#define ML_CHECK_SIZE  150000
#define ML_CHECK_TICK_MLART  0
#define ML_CHECK_TICK_MLOP   127

typedef void * MLRecvCheckHandle;
typedef void (*MLRecvCheckFailFunc)(void *pUserData);

int
ml_create_recv_check(
	int iTotalLink,	
	int iRecvTimeout,
	int iCheckListNum,
	MLTimerHandle struHandle,
	MLRecvCheckFailFunc pFunc,
	MLRecvCheckHandle *pStruHandle
);

void
ml_destroy_recv_check(
	MLRecvCheckHandle struHandle
);

int
ml_add_recv_check(
	void              *pUserData,
	MLRecvCheckHandle struHandle,
	unsigned long     *piRCID
);

#endif
