#include "tc_comm.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_init.h"
#include "tc_print.h"

/*
 * This module is an internal module, but we also want to make it modifiable.
 * A more portable module can be easier to maitain. What will import thread- 
 * safety problem in this module is tc_global_errno. Of course, this is based
 * on the basic modul hash which must be designed as thread-safety
 */

struct tc_err_msg_node{
	int err;
	int start_len;
	int end_len;
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

	tc_err_add(TC_TIMEOUT, "timeout");
	tc_err_add(TC_PARAM_ERROR, "parameter error");
	tc_err_add(TC_NOT_ENOUGH_MEMORY, "not enough memory");
	tc_err_add(TC_NOT_REGISTER_CMD, "the cmd is not registered");
	tc_err_add(TC_GETOPT_LONG_ERR, "getopt execute error");
	tc_err_add(TC_HASH_LIST_DELETED, "deleted the hash list");
	tc_err_add(TC_NOT_REGISTER_CONFIG, "not call the tc_config_oper_register function");
	tc_err_add(TC_OPEN_FILE_ERR, "open file error");
	tc_err_add(TC_EMPTY_FILE, "the file does not exist or its size is 0");
	tc_err_add(TC_CREATE_THREAD_ERR, "create thread error");
	tc_err_add(TC_ADDR_ALREADY_INUSE, "address is already in use");
	tc_err_add(TC_WOULDBLOCK, "the address would block");
	tc_err_add(TC_CTL_EPOLL_ERR, "control socket to epoll err");
	tc_err_add(TC_CURL_INIT_ERR, "curl init error");
	tc_err_add(TC_CURL_PERFORM_ERR, "curl perform error");
	tc_err_add(TC_WRONG_JSON_DATA, "wrong json data");
	tc_err_add(TC_PEER_CLOSED, "peer closed the link");
	tc_err_add(TC_PEER_RESET, "peer reset the link");
	tc_err_add(TC_RECV_ERR, "receive error");
	tc_err_add(TC_SEND_ERR, "send error");
	tc_err_add(TC_PORT_MAP_FULL, "port map full");
	tc_err_add(TC_NO_INTERFACE_PATH, "don't set the interface input or check path");
	tc_err_add(TC_NO_INTERFACE_SET, "dont't set such interface in configure file");
	tc_err_add(TC_WRONG_JSON_FILE, "The json data in file is not right");
	tc_err_add(TC_WRONG_JSON_DATA_TYPE, "The reponse data's type is not the "
					    "same as the one in check file");
	tc_err_add(TC_EPOLL_ERR, "epoll err\n");
	tc_err_add(TC_WRONG_RECV_RETURN_VALUE, "The return value from recv callback is wrong");
	tc_err_add(TC_CONNECT_ERR, "connect to server error\n");
	tc_err_add(TC_NO_HEAP_DATA, "don't add heap data\n");
	tc_err_add(TC_PEER_REFUSED, "The peer refused to connect");
	tc_err_add(TC_NOT_CONNECT, "Don't build the connect");
	tc_err_add(TC_NOT_REGISTER_ADDR, "Don't register this kind of address");

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
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	err_msg->err = errorno;
	if (errmsg) {
		msg_len = strlen(errmsg);
		err_msg->start_len = msg_len;
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
	const char *file,
	const char *func,
	int  line,
	int err,
	int system_err
)
{
	int len = 0, old_len = 0, new_len = 0;
	char *msg = NULL;
	struct hlist_node *hnode = NULL;
	struct tc_err_msg_node *msg_node;
	
	tc_global_errno = err;
	if (system_err == 0)
		return;
	
	hnode = tc_hash_get(global_err_handle.err_hash, err, err);
	if (!hnode)
		return;
	msg_node = tc_list_entry(hnode, struct tc_err_msg_node, node);

	msg = strerror(system_err);
	//PRINT("err: %s -> %s\n", msg_node->msg, msg);
	old_len = strlen(msg_node->msg);
	new_len = strlen(msg) + msg_node->start_len + 
		  strlen(file) + strlen(func) + 64;
	if (msg_node->end_len > new_len) {
		memset(msg_node->msg + msg_node->start_len, 
			0, 
			old_len - msg_node->start_len);
		sprintf(msg_node->msg + msg_node->start_len, 
			"->%s[%s:%d]=>%s", 
			file, func, line, msg);
		return;
	}
	if (msg_node->end_len == new_len)
		new_len += 1;
	
	msg_node->end_len = new_len;
	msg_node->msg = (char*)realloc(msg_node->msg, new_len);
	if (!msg_node->msg)  
		TC_PANIC("Not enough memory for %d bytes\n", len + 3);
	memset(msg_node->msg + msg_node->start_len, 0, new_len - msg_node->start_len);
	sprintf(msg_node->msg + msg_node->start_len, "->%s[%s:%d]=>%s", file, func, line, msg);

	errno = 0;
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
	default: 
		return NULL;
	}
}

char *
tc_errmsg_get(
	int err
)
{
	int len = 0;
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

