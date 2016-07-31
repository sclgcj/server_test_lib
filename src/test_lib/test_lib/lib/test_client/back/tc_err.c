#include "tc_comm.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_init.h"

/*
 * This module is an internal module, but we also want to make it modifiable.
 * A more portable module can be easier to maitain. What will import thread- 
 * safety problem in this module is tc_global_errno. Of course, this is based
 * on the basic modul hash which must be designed as thread-safety
 */

struct tc_err_msg_node{
	int err;
	char *msg;
	struct hlist_node node;
};

struct tc_err_handle{
	int curr_errcode;
	pthread_mutex_t err_mutex;
	tc_hash_handle_t err_hash;
};

#define TC_ERR_HASH_TABLE	128
#define TC_ERR_HASH_MASK	0x7f 


/*
 * Use global variable may be ugly, but at present this may be 
 * the best way to solve this kind of problem.
 */
static __thread int tc_global_errno;


static struct tc_err_handle global_err_handle;


static int
tc_err_hash(
	struct hlist_node *hnode, 
	unsigned long     user_data
)
{
	int err = 0;
	struct tc_err_msg_node *err_msg = NULL;

	if (!hnode && !user_data)
		return TC_ERR;

	if (!hnode) 
		err = (int)user_data;
	else {
		err_msg = tc_list_entry(hnode, struct tc_err_msg_node, node);
		err = err_msg->err;
	}

	return err & TC_ERR_HASH_MASK;
}

static int
tc_err_hash_get(
	struct hlist_node *hnode, 
	unsigned long user_data
)
{
	struct tc_err_msg_node *err_msg = NULL;

	if (!hnode) 
		return TC_PARAM_ERROR;
	err_msg = tc_list_entry(hnode, struct tc_err_msg_node, node);
	if (err_msg->err == (int)user_data) {
		return TC_OK;
	}

	return TC_ERR;
}

static int
tc_err_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_err_msg_node *err_msg = NULL;

	if (!hnode) 
		return TC_PARAM_ERROR;
	err_msg = tc_list_entry(hnode, struct tc_err_msg_node, node);
	if (err_msg->msg)
		free(err_msg->msg);

	free(err_msg);

	return TC_OK;
}

int
tc_error_setup()
{
	struct tc_err_handle err_handle;

	memset(&err_handle, 0, sizeof(err_handle));
	global_err_handle.err_hash = tc_hash_create(
					TC_ERR_HASH_TABLE, 
					tc_err_hash,
					tc_err_hash_get,
					tc_err_hash_destroy);
	if (global_err_handle.err_hash == (TC_HASH_ERR))
		return TC_ERR;

	tc_err_add(TC_PARAM_ERROR, "parameter error");
	tc_err_add(TC_NOT_ENOUGH_MEMORY, "not enough memory");
	tc_err_add(TC_NOT_REGISTER_CMD, "the cmd is not registered");
	tc_err_add(TC_GETOPT_LONG_ERR, "getopt execute error");
	tc_err_add(TC_HASH_LIST_DELETED, "deleted the hash list");
	tc_err_add(TC_NOT_REGISTER_CONFIG, "not call the tc_config_oper_register function");
	tc_err_add(TC_GET_CONFIG_FILE_ERR, "get config file error");
	tc_err_add(TC_EMPTY_FILE, "the file does not exist or its size is 0");

	
	return TC_OK;
}


int
tc_max_errno_get()
{
	return TC_MAX;
}

int
tc_cur_errno_get()
{
	int err = 0;

	return tc_global_errno;
	/*pthread_mutex_lock(&global_err_handle);
	err = global_err_handle.errno;
	pthread_mutex_unlock(&global_err_handle);*/
}

int 
tc_err_add(
	int errorno,
	char *errmsg
)
{
	int ret = 0;
	int msg_len = 0;
	struct tc_err_msg_node *err_msg = NULL;

	err_msg = (struct tc_err_msg_node*)calloc(sizeof(*err_msg), 1);
	if (!err_msg) {
		return TC_NOT_ENOUGH_MEMORY;
	}
	err_msg->err = errorno;
	if (errmsg) {
		msg_len = strlen(errmsg);
		err_msg->msg = (char*)calloc(sizeof(char), msg_len + 1);
		if (!err_msg->msg) {
			free(err_msg);
			return TC_NOT_ENOUGH_MEMORY;
		}
		memcpy(err_msg->msg, errmsg, msg_len);
	}
	ret = tc_hash_add(
			global_err_handle.err_hash, 
			&err_msg->node, 
			0);
	if (ret != TC_OK) {
		return ret;
	}

	return TC_OK;
}

void
tc_errno_set(
	int err
)
{
	tc_global_errno = err;
}

static char *
tc_comm_msg_get(
	int err
)
{
	switch (err) {
	case TC_ERR:
		return "internal error";
	case TC_OK:
		return "success";
	case TC_PARAM_ERROR:
		return "parameter error";
	case TC_NOT_ENOUGH_MEMORY:
		return "system memory is not enough";
	default: 
		return NULL;
	}
}

char *
tc_errmsg_get(
	int err
)
{
	char *msg = NULL;
	char tmp_msg[32] = { 0 };
	struct hlist_node *hnode = NULL;
	struct tc_err_msg_node *err_msg = NULL;
	
	msg = tc_comm_msg_get(err);
	if (msg) {
		return msg;
	}

	hnode = tc_hash_get(global_err_handle.err_hash, err, err);
	if (!hnode) {
		return "non defined error code";
	}
	err_msg = tc_list_entry(hnode, struct tc_err_msg_node, node);

	return err_msg->msg;
}


int
tc_error_uninit()
{
	tc_hash_destroy(global_err_handle.err_hash);

	return TC_OK;
}


int
tc_error_init()
{
	int ret = 0;
	
	ret = tc_error_setup();
//	ret = tc_init_register(tc_error_setup);
	if (ret != TC_OK) 
		return ret;
	return tc_uninit_register(tc_error_uninit);
}

/**
 * declare tc_error_init as constructor 
 */
TC_MOD_INIT(tc_error_init);

