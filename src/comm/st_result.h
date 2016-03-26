#ifndef ST_RESULT_H 
#define ST_RESULT_H 1

#include "st_comm.h"

typedef void * STResultHandle;
typedef void (*STResultFunc)(FILE *fp, void *pUserData);

void
st_handle_result(
	STResultHandle struHandle
);

int
st_create_result_handle(
	int  iTotalCnt,
	void *pUserData,
	char *sResultName,
	STResultFunc pResultFunc,
	STResultHandle *pStruRHandle
);

void
st_destroy_result_handle(
	STResultHandle struHandle
);

#endif

