#ifndef M_BOOT_H
#define M_BOOT_H 1

#include "manage.h"

int 
m_boot(
	MLink *pStruML
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
