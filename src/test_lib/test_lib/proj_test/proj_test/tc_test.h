#ifndef TC_TEST_H
#define TC_TEST_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tc_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_config.h"

#include "cJSON.h"

/*
 *
 */
enum{
	TC_TEST_HARD_ERR = TC_MAX_ERRNO_GET(),
	TC_TEST_SOFT_ERR,
	TC_TEST_ERR_MAX
};

struct hello_test
{
	int i;
	int b;
	int c;
	int d;
};

struct hello_test2
{
	int x;
	int y;
	int z;
};


#endif
