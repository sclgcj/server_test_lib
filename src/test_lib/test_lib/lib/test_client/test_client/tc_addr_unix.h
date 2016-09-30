#ifndef TC_ADDR_UNIX_H
#define TC_ADDR_UNIX_H 1

#include "tc_std_comm.h"
#define TC_UNIX_PATH_MAX 108
struct tc_addr_unix {
	char unix_path[TC_UNIX_PATH_MAX];
};

#endif
