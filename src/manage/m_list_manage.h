#ifndef M_LIST_MANAGE_H
#define M_LIST_MANAGE_H 1

#include "ml_manage.h"
#include "list.h"

enum{
	M_LIST_RUNNGING,	
	M_LIST_OTHER,
	M_LIST_MAX
};

#define M_GET_MBLISST_MANAGE(list_type, struct_type, mbm, len, array) do{ \
	struct list_head *pStruNode = NULL; \
	struct_type *pStruPos = NULL; \
	if( mbm && list_type < M_LIST_MAX ) \
	{\
		pthread_mutex_lock(&mbm->struMBList[list_type].struMutex); \
		len = mbm->struMBList[list_type].iNum; \
		ML_MALLOC(array, struct_type *, len); \
		list_for_each_entry(pStruPos, &pStruMBM->struMBList[list_type].struHead, struNode) \
		{\
			array[i]	= pStruPos; \
		}\
		pthread_mutex_unlock(&mbm->struMBList[list_type].struMutex); \
	}\
}while(0)

typedef struct _MListArray
{
	int iNum;
	unsigned long *pulData;
}MListArray, *PMListArray;

struct _MBListManage;
typedef struct _MBListManage MBListManage, *PMBListManage;

typedef struct _MBList
{
	int iNum;
	struct list_head struHead;
	pthread_mutex_t struMutex;
}MBList, *PMBList;

struct _MBListManage
{
	MBList struMBList[M_LIST_MAX];
	int (*pAddListFunc)(int iType, MBListManage *pStruMBL, struct list_head *pStruNew);
	int (*pDelListFunc)(int iType, MBListManage *pStruMBL, struct list_head *pStruNode);
	void (*pFreeListFunc)(int iType, MBListManage *pStruMBL, void (*pMBListFreeFunc)(struct list_head *pStruNode));
	int (*pGetListFunc)(int iType, MBListManage *pStruMBL, int *iNum, unsigned long **ppulDataArray);
	int (*pCheckListEmpty)(int iType, MBListManage *pStruMBL);
};

void
m_init_mblist_manage(
	MBListManage *pStruMBM
);

void 
m_uninit_mlist_manage(
	MBListManage *pStruMBM
);

#endif
