#ifndef TC_CMD_H
#define TC_CMD_H 1

enum {
	TC_CMD_SHORT,			//single dash('-') with optional param
	TC_CMD_SHORT_PARAM,		//signle dash('-') which must have a param
	TC_CMD_LONG,			//double dash('--') with optional param
	TC_CMD_LONG_PARAM,		//double dash('--') which mutes have a param
	TC_CMD_MAX
};

/**
 * tc_cmd_add() - used to add cmd opt to the downstream
 * @cmd:	the option command
 */

int
tc_cmd_add(
	char *cmd, 
	int type, 
	int (*cmd_handle)(char *, unsigned long),
	unsigned long user_data);

/**
 * tc_user_cmd_add() - used to add user defined init function
 *
 * Return: 0 if successful, -1 if failed and the errno will 
 *	   be set
 */
int
tc_user_cmd_add(
	int (*cmd_init)()
);

#endif
