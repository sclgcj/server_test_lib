#include "ml_comm.h"
#include "ml_manage_config.h"

int
ml_manager_get_opt_val(
	char *sName,
	char **sVal,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	return ml_get_opt_val(sName, sVal, pStruML->struOptHandle);
}

int
ml_manager_read_config_val(
	char *sName, 
	char **sVal,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	return ml_get_read_config_val(
														sName,
														sVal,
														pStruML->struRCHandle
													);
}

