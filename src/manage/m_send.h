#ifndef M_SEND_H 
#define M_SEND_H 1

void
m_send(void *pUserData);

int
m_send_data(
	int  iSockfd,
	char *sRawData
);

#endif
