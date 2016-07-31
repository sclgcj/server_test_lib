#include <getopt.h>

#include "tc_comm.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_cmd_private.h"

/**
 * This module should begin at the beginning of the 
 * program, so we don't take any effort to make it 
 * thread-safety. Of course, we don't need to.
 *
 */

struct tc_cmd_node{
	char *cmd;
	int  type;
	unsigned long user_data;
	int (*cmd_handle)(char *val, unsigned long user_data);
	struct hlist_node node;
};

struct tc_cmd_config{
	int  opt_args_len;
	char *opt_args;
	int long_option_num;
	struct option *long_option;
	tc_hash_handle_t cmd_handle;
};

struct tc_cmd_init_node{
	int (*cmd_init)();
	struct list_head node;
};

#define TC_CMD_TABLE_SIZE 26

static struct tc_cmd_config global_cmd_config;
static struct list_head global_cmd_init_list =  \
				LIST_HEAD_INIT(global_cmd_init_list);

int
tc_user_cmd_init()
{
	int ret = TC_OK;
	struct list_head *sl = NULL;
	struct tc_cmd_init_node *cmd_node = NULL;

	list_for_each_entry(cmd_node, &global_cmd_init_list, node) {
		if (cmd_node->cmd_init) {
			ret = cmd_node->cmd_init();
			if (ret != TC_OK)
				return ret;
		}
	}

	return TC_OK;
}

int
tc_user_cmd_add(
	int (*cmd_init)()
)
{
	struct tc_cmd_init_node *cmd_node = NULL;

	cmd_node = (struct tc_cmd_init_node*)calloc(1, sizeof(*cmd_node));
	if (!cmd_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	cmd_node->cmd_init = cmd_init;

	list_add_tail(&cmd_node->node, &global_cmd_init_list);

	return TC_OK;
}

static int
tc_user_cmd_uninit()
{
	struct list_head *sl = global_cmd_init_list.next;
	struct tc_cmd_init_node *cmd_node = NULL;

	while (sl != &global_cmd_init_list) {
		cmd_node = list_entry(sl, struct tc_cmd_init_node, node);
		sl = sl->next;
		list_del_init(&cmd_node->node);
		free(cmd_node);
	}

	return TC_OK;
}

static int
tc_cmd_hash(
	struct hlist_node *hnode,
	unsigned long     user_data
)
{
	char cmd = 0; 
	struct tc_cmd_node *cmd_node = NULL;

	if (!hnode && !user_data) {
		return TC_ERR;
	}
	if (!hnode) {
		cmd = ((char*)user_data)[0];
	} else {
		cmd_node = tc_list_entry(hnode, struct tc_cmd_node, node);
		cmd = cmd_node->cmd[0];
	}

	return (cmd % TC_CMD_TABLE_SIZE);
}

static int
tc_cmd_hash_get(
	struct hlist_node *hnode,
	unsigned long     user_data
)
{
	struct tc_cmd_node *cmd_node = NULL;

	if (!hnode || !user_data) 
		return TC_ERR;
	cmd_node = tc_list_entry(hnode, struct tc_cmd_node, node);
	if (strcmp(cmd_node->cmd, (char*)user_data)) 
		return TC_ERR;	

	return TC_OK;
}

static int
tc_cmd_destroy(
	struct hlist_node *hnode
)
{
	struct tc_cmd_node *cmd_node = NULL;

	if (!hnode) {
		return TC_ERR;
	}
	cmd_node = tc_list_entry(hnode, struct tc_cmd_node, node);
	TC_FREE(cmd_node->cmd);
	TC_FREE(cmd_node);

	return TC_OK;
}

static int
tc_cmd_setup()
{
	memset(&global_cmd_config, 0, sizeof(global_cmd_config));
	global_cmd_config.cmd_handle = tc_hash_create(
						TC_CMD_TABLE_SIZE,
						tc_cmd_hash,
						tc_cmd_hash_get,
						tc_cmd_destroy
						);
	if (global_cmd_config.cmd_handle == (TC_HASH_ERR)) 
		return TC_ERR;

	return TC_OK;
}


static struct tc_cmd_node *
tc_cmd_node_create(
	char *cmd,
	int  type,
	int (*cmd_handle)(char *val, unsigned long user_data),
	unsigned long user_data
)
{
	int cmd_len = 0;
	struct tc_cmd_node *ret_node = NULL;

	ret_node = (struct tc_cmd_node*)calloc(sizeof(*ret_node), 1);
	if (!ret_node) 
		return NULL;
	cmd_len = strlen(cmd);
	ret_node->cmd = (char*)calloc(sizeof(char), cmd_len + 1);
	if (!ret_node->cmd) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	memcpy(ret_node->cmd, cmd, cmd_len);
	ret_node->type = type;
	ret_node->cmd_handle = cmd_handle;
	ret_node->user_data = user_data;

	return ret_node;
}

static int
tc_cmd_option_calloc(
	char *cmd,
	int  type,
	int  has_arg,
	int (*cmd_handle)(char *val, unsigned long user_data),
	unsigned long user_data
)
{
	int len = 0;
	int idx = 0;
	struct option *loption = NULL;

	idx = global_cmd_config.long_option_num;
	len = (idx + 1) * sizeof(struct option);
	global_cmd_config.long_option = 
		(struct option*)realloc((void*)global_cmd_config.long_option, len);
	if (!global_cmd_config.long_option) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	loption = &global_cmd_config.long_option[idx];
	len = strlen(cmd);
	loption->name = (char *)malloc(len);
	if (!loption->name) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	memcpy((char*)loption->name, cmd, len + 1);
	loption->has_arg = has_arg;
	global_cmd_config.long_option_num++;
//	loption->val = global_cmd_config.long_option_num++;
//	loption->flag = &loption->val;
	return TC_OK;
}

static int
tc_cmd_short_option_calloc(
	char *cmd 
)
{
	int len = 0, cmd_len = 0;
	char cmd_param[64] = { 0 };

	len = global_cmd_config.opt_args_len;
	cmd_len = strlen(cmd);
	global_cmd_config.opt_args_len += cmd_len + 1;
	global_cmd_config.opt_args = (char*)
		realloc(global_cmd_config.opt_args, global_cmd_config.opt_args_len);
	if (!global_cmd_config.opt_args) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	memset(global_cmd_config.opt_args + len, 0, cmd_len + 1);
	strcat(global_cmd_config.opt_args, cmd);
	PRINT("opt_args = %s\n", global_cmd_config.opt_args);

	return TC_OK;
}


int
tc_cmd_add(
	char *cmd,
	int  type,
	int (*cmd_handle)(char *val, unsigned long user_data),
	unsigned long user_data
)
{
	int len = 0, ret = 0;
	char cmd_param[64] = { 0 };
	struct option *loption = NULL;
	struct tc_cmd_node *cmd_node = NULL;	

	if (!cmd || !cmd_handle) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	cmd_node = tc_cmd_node_create(cmd, type, cmd_handle, user_data);
	if (!cmd_node) 
		return TC_ERR;

	switch (type) {
	case TC_CMD_SHORT:
		memcpy(cmd_param, cmd, strlen(cmd));
		ret = tc_cmd_short_option_calloc(cmd_param);
		if (ret != TC_OK)
			return ret;
		break;
	case TC_CMD_SHORT_PARAM:
		sprintf(cmd_param, "%s:", cmd);
		ret = tc_cmd_short_option_calloc(cmd_param);
		if (ret != TC_OK)
			return ret;
		break;	
	case TC_CMD_LONG:
		ret = tc_cmd_option_calloc(cmd, type, 0, cmd_handle, user_data);
		if (ret != TC_OK)
			return ret;
		break;
	case TC_CMD_LONG_PARAM:
		ret = tc_cmd_option_calloc(cmd, type, 1, cmd_handle, user_data);
		if (ret != TC_OK)
			return ret;
		break;	
	}

	return tc_hash_add(global_cmd_config.cmd_handle, &cmd_node->node, 0);
}

static int
tc_cmd_hash_node_execute(
	char *opt_arg,
	char *cmd_name
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct tc_cmd_node *cmd_node = NULL;

	hnode = tc_hash_get(
		global_cmd_config.cmd_handle, 
		(unsigned long)cmd_name,
		(unsigned long)cmd_name);
	if (!hnode) {
		PRINT("not register such cmd : %s\n", cmd_name);
		return TC_ERR;
	}
	cmd_node = tc_list_entry(hnode, struct tc_cmd_node, node);
	if (cmd_node->cmd_handle) {
		ret = cmd_node->cmd_handle(opt_arg, cmd_node->user_data);
		if (ret != TC_OK) {
			PRINT("cmd_handle error\n");
			return TC_ERR;
		}
	} 

	return TC_OK;
}

int
tc_cmd_handle(
	int argc,
	char **argv
) 
{
	int c = 0;
	int ret = 0;
	int opt_idx = 0;	
	char cmd_name[64] = { 0 };
	struct option *long_option = global_cmd_config.long_option;

	if (argc < 2)
		return TC_OK;

	if (!global_cmd_config.opt_args && !long_option) {
		PRINT("no options\n");
		return TC_OK;
	}

	if (global_cmd_config.opt_args) 
		PRINT( "opt_arts = %s\n", global_cmd_config.opt_args);
	while (c != -1)	 {
		opt_idx = 0;
		c = getopt_long(argc, argv, global_cmd_config.opt_args ,
				long_option, &opt_idx);
		if (c == -1) {
			if (errno != 0)
			{
				TC_ERRNO_SET(TC_GETOPT_LONG_ERR);
				PRINT("system_error: %s\n", strerror(errno));
				return TC_ERR;
			}
			continue;
		}
		if (c == 0)
			//PRINT("long_option_num = %d,idx = %d\n", global_cmd_config.long_option_num, opt_idx);
			memcpy(
				cmd_name, 
				long_option[opt_idx].name, 
				strlen(long_option[opt_idx].name));
		else 
			sprintf(cmd_name, "%c", (char)c);
			
		ret = tc_cmd_hash_node_execute(optarg, cmd_name);
		if (ret != TC_OK)
			return ret;
	}
}


int
tc_cmd_uninit()
{
	int i = 0;

	tc_hash_destroy(global_cmd_config.cmd_handle);
	if (global_cmd_config.opt_args) {
		free(global_cmd_config.opt_args);
		global_cmd_config.opt_args = NULL;
	}
	for (; i < global_cmd_config.long_option_num; i++) {
		free((void*)global_cmd_config.long_option[i].name);
		global_cmd_config.long_option[i].name = NULL;
	}
	free(global_cmd_config.long_option);

	tc_user_cmd_uninit();

	return TC_OK;
}

int
tc_cmd_init()
{
	int ret = 0;

	PRINT("\n");
	ret = tc_cmd_setup();
	if (ret != TC_OK)
		return ret;
	/*ret = tc_init_register(tc_cmd_setup);
	if (ret != TC_OK) 
		return ret;*/

	return tc_uninit_register(tc_cmd_uninit);
}

TC_MOD_INIT(tc_cmd_init);
