#ifndef SERVER_TEST_READ_CONFIG_H
#define SERVER_TEST_READ_CONFIG_H

typedef void * STRCHandle;

enum
{
	SERVER_TEST_RC_INT,
	SERVER_TEST_RC_UINT,
	SERVER_TEST_RC_CHAR,
	SERVER_TEST_RC_UCHAR,
	SERVER_TEST_RC_SHORT,
	SERVER_TEST_RC_USHORT,
	SERVER_TEST_RC_STRING,
	SERVER_TEST_RC_IPADDR,
	SERVER_TEST_RC_FUNC,
	SERVER_TEST_RC_MAX
};

int
server_test_create_read_config(
	char				*sFile,
	STRCHandle  *pStruHandle
);

void
server_test_destroy_read_config(
	STRCHandle struHandle
);

int 
server_test_read_config(
	STRCHandle struHandle
);

int
server_test_read_config(STRCHandle struHandle);

#endif
