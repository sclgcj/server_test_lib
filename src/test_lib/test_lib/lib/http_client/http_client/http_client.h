#ifndef HC_HTTP_CLIENT_2016_09_26_17_09_47_H
#define HC_HTTP_CLIENT_2016_09_26_17_09_47_H

#include "hc_comm.h"
#include "hc_init.h"
/*
 * 2016-9-9 record:
 *	This archeture is not good. I have used it yestoday, but the 
 * effect is bad. I think it's a little hard to use when we decide to 
 * bind struct hc_interface_op to every api. However, if we use our 
 * default json function, we can give a common structure to every api.
 * We need more test to check if this is suitable.
 */

enum {
	HC_INTERFACE_PARAM_NORMAL,
	HC_INTERFACE_PARAM_JSON,
	HC_INTERFACE_PARAM_MAX
};

struct hc_oper {
	/*
	 * err_handle() - user defined err handle function when something wrong in a link
	 * @reason:	the error resason
	 * @user_data:	user used data
	 *
	 * Return: 0 - nothing to do; -1 sometion wrong; TC_LINK_DEL - del link from epoll;
	 *	   TC_LINK_DESTROY - destroy the link, this will destroy all the data of 
	 *	   this link containing downstream data and upstream data.
	 */
	int (*err_handle)(int reason, unsigned long user_data);

	/*
	 * prepare_data_get() - get prepare data from upstream
	 * @port_map_cnt:	 the port number used at present	
	 * @data:		 a pointer to the upstream data
	 *
	 * We provide port_map_cnt parameter for upstreams to allow them to allocate data 
	 * for different port. Sometimes the first link is client, but the second or other
	 * link may server. Upstream may allocate different data types because of different
	 * situations.
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*prepare_data_get)(int port_map_cnt, unsigned long data);
};

struct hc_interface_oper {
	/*
	 * interface_param() - set the interface parameters
	 * api_name:	the api's name
	 * user_data:	upstream data
	 * param:	pointer to store the parameters. It's a double pointer of char(char**).
	 *		*param will point to the real address. Just look like this:
	 *			char **tmp = (char**)param;
	 *
	 * Please store the final send string in the param, and allocate new space for the parameter
	 *
	 * Return: 0 if success, -1 if not
	 */
	int  (*interface_param)(char *api_name, unsigned long user_data, unsigned long *param);
	/*
	 * curlopt_set() - set the curl options.
	 * curl:	a curl handle pointer
	 * user_data:   upstream user data
	 *
	 * We will set some default curl option, but this may not satisfy
	 * other situations. Thus, we provide this callback for upstreams 
	 * to set their own curl options. default curl opt below:
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
	 * Return: 0 if successful, -1 if not
	 */
	int (*curlopt_set)(char *api_name, CURL *curl, unsigned long user_data);
	/*
	 * before_send() - let user do something before sending http packet
	 * user_data:	upstream user data
	 *
	 * Before sending a packet, upstream may have something to do, so 
	 * we provide this callback. 
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*before_send)(char *api_name, unsigned long user_data);
	/*
	 * first_recv() - do something when first receiving peer data
	 * user_data:	upstream user data
	 *
	 * Sometimes we need do something when we first receiving the peer
	 * data, this may different from the interface_recv. Of course, we
	 * also can realize it in interface_recv callback. The relationship 
	 * of first_recv and interface_recv in realizing is below:
	 *	first_recv(user_data);
	 *	interface_recv(user_data);
	 * It doesn't do any reception operation. Please realize all reception
	 * operation in interface_recv callback
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*first_recv)(unsigned long user_data);
	/*
	 * interface_recv() - check if receiving all the http data
	 * ptr:		the http data that has been received
	 * size:	the size of each char
	 * nmemb:	the num of char
	 * user_data:	upstream data
	 *
	 * This function is used to determine if we have received all the 
	 * http data. In most time, we can determine this use content-length,
	 * but sometimes we can't get content-length value from the 
	 * reponse packet. In such situation, we need upstream to tell us 
	 * when the packet has been received over.
	 *
	 * Return: 0 if the data has reached the end, -1 if not
	 */
	int (*interface_recv)(char *ptr, 
			      size_t size, 
			      size_t nmemb, 
			      unsigned long user_data);
	/*
	 * interface_check() - check http data
	 * api_name:    this api interface name
	 * recv_cnt:	the count of the received data
	 * recv_data:	the received data
	 * user_data:	upstream data
	 *
	 * Check the http data
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*interface_check)(char *api_name, 
			       int recv_cnt, 
			       char *recv_data, 
			       unsigned long user_data);
};

/*
 * hc_interface_register() - register an api interface
 * @group_name:		the group of the api belonging to. For example, if there is a url 
 *			http://ip:port/hello/api_name, we call 'hello' as the group_name.
 *			It can be any length.
 * @api_name:		the interface's name
 * @oper:		the operation function of the api, to see struct hc_interface_oper	
 *			for details
 *
 * We just split the interface process into few parts. 
 * 1、 interface param set
 * 2、 curl opt set
 * 3、 before send disposition
 * 7、 sending http packet
 * 4、 first reception disposition
 * 5、 data reception
 * 6、 data check
 *
 * we try our best to abstract the interface test.
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
hc_interface_register(
	char *group_name,
	char *api_name,
	struct hc_interface_oper *oper
);

/*
 * hc_interface_url_encode() - use url code to encode the data
 * @data:	raw data that need url encode
 * 
 * Return: the string pointer if success, null if something wrong
 */
char*
hc_interface_url_encode(
	char *data
);

/*
 * hc_start() - start http_client
 * @user_data_size:	the data structure size of the upstream
 * @oper:		the set of http client callback
 *
 * It's used to start the http client process. 
 *
 * Return: 0 if successful, -1 if something wrong
 */
int
hc_start(
	int user_data_size,
	struct hc_oper *oper
);
#endif
