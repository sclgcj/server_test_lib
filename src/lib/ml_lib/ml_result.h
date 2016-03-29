#ifndef ML_RESULT_H 
#define ML_RESULT_H 1

#include "ml_comm.h"

typedef void * MLResultHandle;
typedef void (*MLResultFunc)(FILE *fp, void *pUserData);

void
ml_handle_result(
	MLResultHandle struHandle
);

int
ml_create_result_handle(
	int  iTotalCnt,
	void *pUserData,
	char *sResultName,
	MLResultFunc pResultFunc,
	MLResultHandle *pStruRHandle
);

void
ml_destroy_result_handle(
	MLResultHandle struHandle
);

#endif

