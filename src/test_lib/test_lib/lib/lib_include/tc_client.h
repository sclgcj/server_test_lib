#ifndef TC_CLIENT_H
#define TC_CLIENT_H

#include "tc_client_oper.h"

/**
 * tc_proj_register() - used to register the current project info
 * @data_size:			the size of upper structure size
 * @oper:			the operation of the arch	
 *
 * This function used to register current project infomation.
 *
 * Return: success: 0, fail with related error code defined int tc_result.c
 */
int tc_proj_register(
			int data_size,
			struct test_client_operation *oper 
		);

/**
 * tc_proj_unregister() - used to unregister the current project info
 *
 * This function used to unregister current project infomation.
 *
 * Return: success: 0, fail with related error code defined int tc_result.c
 */
int tc_proj_unregister();

/**
 * tc_proj_ctrl() - used to get or set the current project config 
 * @cmd:		command
 * @data:		set cmd used to transfer data to the downstream,
 *			get cmd used to store info from downstrem
 *
 * This function used to unregister current project infomation.
 *
 * Return: success: 0, fail with related error code defined int tc_result.c
 */
int tc_proj_ctrl(int cmd, unsigned long data);



#endif
