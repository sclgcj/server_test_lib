#ifndef M_UNIX_HANDLE_H
#define M_UNIX_HANDLE_H

#include "cJSON.h"

int
m_unix_test(
	cJSON *pStruRoot,
	void *pData
);

int
m_get_cur_run_servers(
	cJSON *pStruRoot,
	void *pData
);

int
m_get_servers(
	cJSON *pStruRoot,
	void  *pData
);

int
m_get_test_list(
	cJSON *pStruRoot,
	void  *pData
);

#endif
