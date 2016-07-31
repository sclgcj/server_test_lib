#ifndef TC_COMM_H
#define TC_COMM_H 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <pthread.h>
#include <sys/stat.h>

#include "list.h"

#define tc_list_entry(ptr, type, memb) \
		((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->memb)))

#define TC_FREE(ptr) do{ \
	if (ptr) { \
		free(ptr); \
		ptr = NULL; \
	} \
}while(0)

#endif
