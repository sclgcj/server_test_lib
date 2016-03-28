#include "ml_manage_create.h"

int
ml_manager_start_create_link(
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_start_create_link(pStruML->struCLHandle);
}
