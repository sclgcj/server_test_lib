#ifndef TC_INIT_PRIVATE_H
#define TC_INIT_PRIVATE_H

#include "tc_init.h"
/*
 * tc_init() -	used to init all the modules which registered by 
 *		tc_local_init_register function.	
 *
 * Return: 0 if successful, -1 if failed;
 */
int
tc_init();

/*
 * tc_uninit() -	used to init all the modules which registered by 
 *			tc_local_init_register function.	
 *
 * Return: 0 if successful, -1 if failed;
 */
int
tc_uninit();

int
tc_local_init_register(
	int (*init)()
);

int
tc_local_uninit_register(
	int (*uninit)()
);

int 
tc_init_test();


#endif
