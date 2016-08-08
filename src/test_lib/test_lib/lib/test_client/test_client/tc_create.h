#ifndef TC_CREATE_H
#define TC_CREATE_H

#include "tc_epoll.h"
#include <net/if.h>


enum {
	TC_LINK_DEL = 1,
	TC_LINK_DESTROY
};

struct tc_create_link_oper{
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
	/*
	 * create_flow_ctrl() - flow control
	 * @cur_count: current link count
	 */
	void (*create_flow_ctrl)(int cur_count);
	/*
	 * extra_data_set() - set downstream data to the upstream
	 * @extra_data:		downstream data
	 * @user_data:		user data
	 *
	 * Sometimes upstreams will call the downstream function, and some functions 
	 * may need some data that only operated by downstream. In order to hide these
	 * details to the upstreams, we provide this interface for upstream to store 
	 * this kind of data.
	 */
	void (*extra_data_set)(unsigned long extra_data, unsigned long user_data);
	/*
	 * create_link() - create a link
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*create_link)(
			int sock, 
			struct in_addr addr, 
			unsigned short port, 
			unsigned long data);
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
	 * send_data() - user defined send function
	 * @sock:	socket
	 * @user_data： user data
	 * @send_buf:	the buffer storing the sending data
	 * @send_len:	the length of the sending data
	 * @addr:	udp used peer address
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*send_data)(
			 unsigned long user_data
			 );
	/*
	 * recv_data() - user define recv fucntion
	 * @sock:	socket
	 * @user_data:  user data
	 * @recv_buf:	the buffer storing the received data, its space is allocated by user
	 * @recv_len:	the length of the received data
	 * @addr:	udp used peer address
	 *
	 * Return: 0 if there is more data to be received, -1 means something wrong, >0 means
	 *	   receiving over.
	 */
	int (*recv_data)(char *ptr, int size, unsigned long user_data);
	/*
	 * handle_data() - user defined function to dispose recvd data
	 * @user_data:	user data
	 *
	 * Return 0 if successful, -1 if not
	 */
	int (*handle_data)(unsigned long data);
	/*
	 * connected_func() - user defined connect function
	 * @user_data:  user_data,
	 * @event:	the event after connected func, user defined
	 * @addr:	udp used peer address
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*connected_func)(
			unsigned long user_data);

	/*
	 * accept_func() - user defined accept function
	 * @sock:	socket
	 * @user_data:  user defined data for a connection
	 * @addr:	peer addr
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*accept_func)(
			unsigned long extra_data,
			struct sockaddr_in *addr, 
			int *event, 
			unsigned long user_data);
	/*
	 * udp_accept_func() - user defined accept function
	 * @sock:	socket
	 * @user_data:  user defined data for a connection
	 * @addr:	peer addr
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*udp_accept_func)(
			unsigned long extra_data,
			struct sockaddr_in *addr, 
			int *event, 
			unsigned long user_data);
	/*
	 * harbor_func() - user defined function to orgnize harbor data
	 * @sock:	socket
	 * @user_data:  user data
	 * @peer_addr:  peer address
	 *
	 * This function can just be used to orgnize harbor data and set event to TC_EVENT_WRITE
	 * to send this packet by downstream. Of course, it can send this packet itself.
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*harbor_func)(int sock, unsigned long user_data, struct sockaddr_in *peer_addr);
	/*
	 * Next interface* functions are used by api test.
	 */
	/*
	 * interface_before_send() - used to do some tasks before sending api packet
	 * @user_data:		user data
	 *
	 * In tests, we need to store some information before sending packet, so that we can 
	 * have a more detailed information when orgnize the packet.
	 *
	 */
	void (*interface_before_send)(unsigned long user_data);
	/*
	 * interface_recv() - recv the interface response data
	 * @ptr:	data received by libcurl
	 * @size:	the size of each member
	 * @nmemb:	the number of members
	 * @user_data:  user_data
	 *
	 * When using libcurl to receive chunk data, we usually don't know when to end and 
	 * how to end the packet is always up to the designers. So we need upstream to give
	 * us this function to determine if it has received all packets. if upstream doesn't
	 * provide this function, we will call interface handle function each time when libcurl
	 * call writeback function.Note, the data should be stored by upstream, and we don't 
	 * keep any data.
	 *
	 * Return: 0 if receive over, -1 if not
	 */
	int (*interface_recv)(char *ptr, size_t size, size_t nmemb, unsigned long user_data);
	/*
	 * result_func() - dispose the link result
	 * @user_data:	user_data
	 *
	 * In test,we need this to handle each link's result.
	 */
	void (*result_func)(unsigned long user_data);
	/*
	 * data_destroy() - destroy user_data
	 * @user_data:	user_data
	 */
	void (*data_destroy)(unsigned long user_data);
	/*
	 * Functions below is used for server transferring, at present we don't want to pay 
	 * much attention on them, and they may change in the future, so we don't write their
	 * comment. Left them here is to reserve them in case of forgetting them.
	 */
	int (*transfer_send)(
			int sock, 
			unsigned long user_data, 
			struct sockaddr *addr);
	int (*transfer_recv)(
			int sock, 
			unsigned long user_data, 
			struct sockaddr *addr);
	int (*transfer_handle)(
			int sock,
			unsigned long user_data,
			struct sockaddr *addr);
};

/*
 * tc_link_create() - create the link module
 * @user_data_size:	user defined structure data size
 * @oper:		create operations, set struct tc_create_link_oper
 *
 * We decide to hide all the details of the downstreams. This means upstreams 
 * can not get any downstream infomation directly. Upstream gives downstream 
 * its data size and downstream will create the its data memory, so the 
 * upstream needn't care about its structure memory. What upstream need to 
 * do is to manage the data its structure has.We recommend that server and 
 * client use the same structure. If not we recommend to set user_data_size 
 * the bigger one. 
 * 
 * Return: 0 if successful, -1 if not
 */
int
tc_link_create(
	int user_data_size,
	struct tc_create_link_oper *oper
);

/*
 * tc_create_link_new() - create a new link, can be a client connecting to server or a server 
 *			  to accept client links
 * @proto:	link protocol
 * @link_type:  server or client
 * @server_ip： server address, if 0 will use the server ip configured in configure file
 * @server_port: server port, if 0 will use the server port configured in configure file
 * @extra_data: data set by extra_data_set function 
 *
 * We provide this functions because some protocals may create a new link when the first 
 * link created(such as ftp).  
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int 
tc_create_link_new(
	int proto,
	int link_type,
	struct in_addr server_ip,
	unsigned short server_port,
	unsigned long extra_data	
);

/*
 * tc_create_link_recreate() - recreate a link
 * @flag:	if use the same port to create a new link, 1 - use the same port, 0 - not
 * @close_link: if close the link：	
 *		0 - close, but not destroy the structure data
 *		1 - close and destroy the  structure data
 * @server_ip:	new connections' server address, if 0, the configured server ip will be used
 * @server_port:new connections' server port, if 0, the configured server pot will be used
 * @extra_data:	data set by extra_data_set function
 *
 * In fact, we don't want to provide this kind of function, because similar function can use
 * tc_create_link_new to implement. However, tc_create_link_new will create a new data structure,
 * and sometimes we may hope to use current structure for a new connection. Of course, we imagine
 * the recreated link using the same link type of the old one. Upstreams should provide the new 
 * link's server_ip and server_port, if they are 0, we will use the configured values.upstream
 * should tell us if we should use the port_map port to create a new link or just use the same
 * port to create a new link. if flag == 1, close_link == 2 is forbidding.
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_create_link_recreate(
	int flag,
	int close_link,
	struct in_addr server_ip,
	unsigned short server_port,
	unsigned long extra_data
);


#endif
