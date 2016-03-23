#include "st_manage_create.h"

int
st_manager_start_create_link(
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	return st_start_create_link(pStruST->struCLHandle);
}
