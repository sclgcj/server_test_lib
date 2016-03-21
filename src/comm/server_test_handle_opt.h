#ifndef SERVER_TEST_HANDLE_OPT_H
#define SERVER_TEST_HANDLE_OPT_H 1

#include "server_test_read_config.h"
#include "server_test_comm_config.h"

typedef void * STOptHandle;

int
server_test_parse_opt(
	int  iArgc,
	char **sArgv,
	STOptHandle struHandle
);

void
server_test_create_opt_config(
	char        *sParseFmt,
	STOptHandle *pStruHandle
);

void
server_test_destroy_opt_config(
	STOptHandle struHandle
);

#endif
