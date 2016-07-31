#ifndef TC_CMD_PRIVATE_H
#define TC_CMD_PRIVATE_H

/**
 * tc_cmd_handle() - dispose the console command
 * @argc:	the number of the console command
 * @argv:	the console command array
 *
 * Return: 0 if successful, -1 if failed and the errno will
 *	   be set
 */
int
tc_cmd_handle(
	int argc,
	char **argv
);

/**
 * tc_user_cmd_init() - init user added cmd
 *
 * Return: 0 if successful, -1 if failed and the errno will 
 *         be set	
 */
int
tc_user_cmd_init();

#endif
