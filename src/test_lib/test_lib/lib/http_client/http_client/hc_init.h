#ifndef HC_INIT_H
#define HC_INIT_H

#include "tc_init.h"

int
hc_init_register(
	int (*init)()
);

int
hc_uninit_register(
	int (*uninit)()
);

#endif
