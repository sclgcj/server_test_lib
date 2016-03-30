#ifndef M_CLIENT_DATA_H
#define M_CLIENT_DATA_H 1

typedef void * MLCDHandle;

void
ml_create_client_data(
	int iClientNum,
	MLCDHandle *pStruDHandle
);

void
ml_destroy_client_data(
	MLCDHandle struHandle
);

int
ml_add_client_data(
	void			 *pUserData,
	MLCDHandle struHandle,
	int				 *piID
);

int
ml_get_client_data(
	int				 iID,
	MLCDHandle struHandle,
	void			 **pUserData
);

#endif
