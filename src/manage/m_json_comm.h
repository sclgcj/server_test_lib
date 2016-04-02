#ifndef M_JSON_COMM_H
#define M_JSON_COMM_H 1

#include "cJSON.h"

int
m_json_get_object_val(
	char         *sName,
	cJSON        *pStruRoot,
	unsigned int *puiVal
);

int
m_json_get_object_str(
	char  *sName,			
	cJSON *pStruRoot,
	char  *sData
);

void
m_json_set_object_val(
	unsigned int  iVal,
	char *sName,
	cJSON *pStruObj
);

void
m_json_set_object_str(
	char *sStr,
	char *sName,
	cJSON *pStruObj
);

int
m_json_get_object_str_malloc(
	char *sName,
	cJSON *pStruRoot,
	char **sData
);

#endif
