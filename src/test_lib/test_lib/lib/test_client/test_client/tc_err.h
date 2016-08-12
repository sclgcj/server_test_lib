#ifndef TC_ERR_H
#define TC_ERR_H



//err number
enum{
	TC_ERR = -1,
	TC_OK = 0,
	TC_TIMEOUT,
	TC_EMPTY_FILE,
	TC_PEER_RESET,
	TC_PEER_CLOSED,
	TC_OPEN_FILE_ERR,
	TC_PARAM_ERROR,
	TC_NOT_ENOUGH_MEMORY,
	TC_NOT_REGISTER_CMD,
	TC_NOT_REGITSTER_CONFIG_OPT,
	TC_GETOPT_LONG_ERR,
	TC_HASH_LIST_DELETED,
	TC_NOT_REGISTER_CONFIG,
	TC_CREATE_THREAD_ERR,
	TC_ADDR_ALREADY_INUSE,
	TC_WOULDBLOCK,
	TC_CTL_EPOLL_ERR,
	TC_CURL_INIT_ERR,
	TC_CURL_PERFORM_ERR,
	TC_WRONG_JSON_DATA,
	TC_RECV_ERR,
	TC_SEND_ERR,
	TC_PORT_MAP_FULL,
	TC_NO_INTERFACE_PATH,
	TC_NO_INTERFACE_SET,
	TC_WRONG_JSON_FILE,
	TC_WRONG_JSON_DATA_TYPE,
	TC_EPOLL_ERR,
	TC_WRONG_RECV_RETURN_VALUE,
	TC_CONNECT_ERR,
	TC_NO_HEAP_DATA,
	TC_MAX
};

/**
 * tc_error_uninit() - uninit error module
 *
 * Return: 0 if successful, -1 if failed
 */
int
tc_error_uninit();


/**
 * tc_max_errno_get() - return the currently max errno
 *
 * When upstream wants to add its own errno, we advise 
 * to call this function first, so the errno added by 
 * upstream will no cover the one used in downstream.
 *
 * Return: the currently mac errno
 */ 
int 
tc_max_errno_get();
#define TC_MAX_ERRNO_GET() TC_MAX

/** 
 * get_cur_errno_get() - return the current set errno
 *
 * If upstreams want to get the current errno, they can
 * call this function
 *
 * Return : the currently set errno value
 */
int
tc_cur_errno_get();

/**
 * tc_err_add() - add new error num to the err module
 * @errno:	added errno	
 * @errmsg:	the man-reading msg to the errno
 *
 * This function used to add a new errno and it msg 
 * to the downstream'a error handle structure, so that
 * we can add the errcode at anytime we want.
 *
 * Return: 0 if success, otherwise return an errcode
 */
int
tc_err_add(
	int errorno, 
	char *errmsg 
);

/**
 * tc_errno_set() - set errno for every error
 * @errno:  the error number of the current error
 *
 * This function usually used for the downstring,
 * but in order to have the same error msg output
 * with the upstream, we decide to provide it out,
 * so that upstream function can also call it to 
 * set its own errno. Of cource, if upstream wants
 * get such effect, it should call tc_err_add fucntion
 * first to register its own err msg.
 *
 * Return: no
 */

void
tc_errno_set(
	const char *file,
	const char *func,
	int  line,
	int err,
	int system_err
);
#define TC_ERRNO_SET(err) tc_errno_set(__FILE__, __FUNCTION__, __LINE__, err, errno)

/**
 * tc_errmsg_get() - get the errmsg of current set errno
 *
 * This can return the related err man-reading msg of current
 * errno set by an error. We are sure that it will return 
 * the ringt msg in downstring. Before calling this function,
 * upstream should call tc_errno_set to set the right errno 
 * to current error.
 *
 * Retrun: the errmsg of the current errno if the errno is ok;
 *	   otherwise return "no related errmsg for errno current 
 *	   xxx"
 */
char *
tc_errmsg_get( 
	int errorno
);
#define TC_CUR_ERRMSG_GET() tc_errmsg_get(tc_cur_errno_get())

#endif
