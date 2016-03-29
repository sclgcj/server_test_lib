#ifndef ML_MANAGE_HUB_H 
#define ML_MANAGE_HUB_H 1

#include "ml_manage.h"

int
ml_manage_add_hub(
	void          *pData,
	unsigned long *pulHubID,
	MLHandle	    struHandle
);

int
ml_manage_del_hub(
	unsigned long ulHubID,
	MLHandle		  struHandle
);


#endif
