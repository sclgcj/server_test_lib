#ifndef ML_MANAGE_CLIENT_DATA_H
#define ML_MANAGE_CLIENT_DATA_H 1

#include "ml_manage.h"

int
ml_manager_get_client_data(
	int			 iID,
	MLHandle struHandle,
	void		 **pUserData
);

int
ml_manager_add_client_data(
	void			 *pUserData,
	MLHandle   struHandle,
	int				 *piID
);

#endif
