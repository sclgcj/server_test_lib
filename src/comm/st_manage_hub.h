#ifndef ST_MANAGE_HUB_H 
#define ST_MANAGE_HUB_H 1

#include "st_manage.h"

int
st_manage_add_hub(
	void          *pData,
	unsigned long *pulHubID,
	STHandle	    struHandle
);

int
st_manage_del_hub(
	unsigned long ulHubID,
	STHandle		  struHandle
);


#endif
