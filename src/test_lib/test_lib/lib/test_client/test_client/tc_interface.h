#ifndef TC_INTERFACE_H
#define TC_INTERFACE_H 1

#include "tc_comm.h"
/*
 * tc_interface_register() - register an api interface
 * @interface_name:	the name of interface
 * @interface_func:	the interface function
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_interface_register(
	char *interface_name,
	int (*interface_func)(unsigned long user_data)
);


/*
 * tc_mobile_data_send() - use libcurl to send api data
 * @url:	http or https url
 * @param:	the data to send
 * @param_size: the param size
 * @user_data:  user_data 
 * @curlopt_set: used to set the curl option, we have the 
 *		 default curl opt below:
 *			CURLOPT_POST		-----> 1
 *			CURLOPT_SSL_VERIFYPEER	-----> 0
 *			CURLOPT_SSL_VERIFYHOST  -----> 0
 *			CURLOPT_WRITEDATA	-----> write_callback data
 *			CURLOPT_WRITEFUNCTION   -----> write_callback function
 *			CURLOPT_POSTFIELDSIZE	-----> param_size
 *			CURLOPT_POSTFIELDS	-----> param
 *			CURLOPT_URL		-----> url
 *			CURLOPT_NOSIGNAL	-----> 1
 *			CURLOPT_TIMEOUT		-----> recv_timeout
 *			CURLOPT_CONNECTTIMEOUT  -----> connect_timeout
 *			CURLOPT_SSL_SESSIONID_CACHE -> 0
 *			CURLOPT_CAINFO		-----> ""
 *			CURLOPT_CAPATH		-----> ""
 *
 * @write_callback: curl response callback
 */
int
tc_mobile_data_send(
	char *url,
	char *param,
	int  param_size,
	unsigned long extra_data,
	void (*curlopt_set)(unsigned long user_data, CURL *curl),
	int (*write_callback)(char *ptr, size_t size, size_t nmemb, void *user_data)
);

/*
 * tc_interface_url_encode() - use url code to encode the data
 * @data:	raw data that need url encode
 * @encode_len: the length of encode_str, if the real length is larger than it, it will be set 
 *		to the real length, and the function will return -1
 * @encode_str: the buffer to store the url encoded string
 * 
 * Return: 0 if successful, -1 if something wrong or the real length is larger than the given one
 */
int
tc_interface_url_encode(
	char *data,
	int  *encode_len,
	char *encode_str
);

#endif
