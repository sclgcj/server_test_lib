#ifndef TC_INIT_PRIVATE_H
#define TC_INIT_PRIVATE_H

/*
 * tc_init() -	used to init all the modules which registered by 
 *		tc_init_register function.	
 *
 * Return: 0 if successful, -1 if failed;
 */
int
tc_init();

/*
 * tc_uninit() -	used to init all the modules which registered by 
 *			tc_init_register function.	
 *
 * Return: 0 if successful, -1 if failed;
 */
int
tc_uninit();



#endif
