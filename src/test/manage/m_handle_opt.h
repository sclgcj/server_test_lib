#ifndef M_HANDLE_OPT_H
#define M_HANDLE_OPT_H 1

#define M_MAKE_DAEMON		'd'
#define M_CONFIG_FILE   'f'

#include "ml_manage.h"

typedef struct _MOptConfig
{
	int iMakeDaemon;							//'d'
	char sConfigFile[256];				//'f'
}MOptConfig , *PMOptConfig;

void
m_get_opt_config(MOptConfig *pStruMOC, MLHandle struHandle);

#endif
