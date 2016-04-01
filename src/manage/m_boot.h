#ifndef M_BOOT_H
#define M_BOOT_H 1

#include "ml_comm.h"
#include "cJSON.h"

void
m_boot(
	void *pData
);

int
m_boot_handle_request(
	cJSON *pStruRoot,
	void  *pData
);

int
m_boot_handle_response(
	cJSON *pStruRoot,
	void  *pData
);
#endif
