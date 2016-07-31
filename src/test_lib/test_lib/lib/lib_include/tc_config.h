#ifndef TC_CONFIG_H
#define TC_CONFIG_H

/*
 * This module is an internal module used to manage all
 * the config procedure. Use this module we must register 
 * its open and close function of the config file. Please
 * note that we don't which config file we need to open and
 * close, these are decided by upstreams. Of course, this 
 * is just a test version, if we get a more wonderful way 
 * to dispose it, we will change it.
 */

/*
 * toml type, used for node handle
 */
enum
{
	TC_CONFIG_TOML_ROOT,
	TC_CONFIG_TOML_LIST,
	TC_CONFIG_TOML_TABLE,
	TC_CONFIG_TOML_TABLE_ARRAY,
	TC_CONFIG_TOML_NORMAL,
	TC_CONFIG_TOML_MAX
};

/*
 * protocol configure
 */
enum
{
	TC_PROTO_TCP, 
	TC_PROTO_UDP,
	TC_PROTO_HTTP,
	TC_PROTO_MAX
};

/*
 * server or client
 */
enum
{
	TC_DEV_CLIENT,
	TC_DEV_SERVER,
	TC_DEV_HTTP_CLIENT,
	TC_DEV_MAX
};

struct tc_config_oper{
	/*
	 *
	 * get_config_file() -	get the config file from the upstream, 
	 *			Return the .
	 * @file_path_len: 	the length of the file path
	 * @file_path:		the string used to store the file path
	 * @real_len:		the real length of the file path
	 *
	 * in get_config_file, when pass a path len less than the 
	 * real lenght of the file path, please store the real path
	 * length in real_len, and return -1
	 *
	 * Return: 0 if successfull and -1 if not
	 
	 */
	int (*get_config_file)(int file_path_len, char *file_path, int *real_len);

	/*
	 * config_start() -	do some upstreams' preperation work before start 
	 *			the config analysis
	 */
	void (*config_start)();

	/*
	 * config_end() -	do some upstreams' end work after the config analysis
	 */
	void (*config_end)();
};

/*
 * tc_config_oper_register() - register the config open, close
 *				function
 * 
 * It must be called at first in upstreams' init function. This
 * is useful, so we can be sure that our config operation will
 * work fine.
 *
 * Return: 0 if successful, -1 if not and specific errno will be 
 *	   set
 */
int
tc_config_oper_register(
	struct tc_config_oper *config_oper
);

/*
 * tc_config_add() - add config option to the downstream procedure
 *
 * upstreams can use this function to add its configuration options
 * to the downstream. It's simple and flexible.
 *
 * Return: 0 if successful, -1 if not and specific errno will be set
 */
int
tc_config_add(
	char *conf_name,
	int toml_type,
	unsigned long user_data,
	void (*config_handle)(int toml_type, char *name, char *val, unsigned long user_data)
);
	
#endif
