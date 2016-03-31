#ifndef M_DISPOSE_H
#define M_DISPOSE_H 1

#include "ml_comm.h"
#include "cJSON.h"

typedef int (*MDisposeFunc)(cJSON *pStruRoot, void* pUserData);

struct _MDispose;
typedef struct _MDispose MDispose,*PMDispose;

struct _MDispose
{
	int iStatus;
	char *sMethod;
	MDisposeFunc pFunc;	
	MDispose *pStruNext;
};

typedef int (*MDisposeDispatchFunc)(unsigned long ulData, MDispose *pStruD);

void
m_create_dispose(
	MDispose *pStruMD
);

void
m_destroy_dispose(
	MDispose *pStruHead
);

int
m_add_dispose(
	int iStatus,	
	char *sMethod,
	MDisposeFunc pFunc,
	MDispose *pStruHead
);

void
m_dispose(
	int  iRecvLen, 
	char *sRecvData,
	void *pUserData
);


#endif
