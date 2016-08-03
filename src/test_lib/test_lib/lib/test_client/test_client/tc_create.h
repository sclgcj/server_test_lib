#ifndef TC_CREATE_H
#define TC_CREATE_H

#include "tc_epoll.h"
#include <net/if.h>


struct tc_link_data {
	int		err_flag;		//错误标记，由用户设置该标志，如果为1, 则只会将
						//该连接移除epoll，如果为2，则会将和该连接有关的
						//所有数据均移除。这么做的目的是因为服务器和客户端
						//对连接失败的处理是不同的。为0，则不作任何操作
	char		*unix_path;		//unix socket path
	struct in_addr	peer_addr;		//对端地址
	struct in_addr 	local_addr;		//本地地址
	unsigned short 	peer_port;		//对端端口
	unsigned short 	local_port;		//本地端口
};

struct tc_create_link_oper{
	/*
	 * prepare_data_get() - get prepare data from upstream
	 * @data - a pointer to the upstream data
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*prepare_data_get)(int port_map_cnt, unsigned long *data);
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
	 * Return: no
	 */
	void (*err_handle)(int reason, unsigned long user_data, struct tc_link_data *link_data);
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
	int (*send_data)(int sock, 
			 unsigned long user_data, 
			 struct sockaddr_in *addr);
	/*
	 * recv_data() - user define recv fucntion
	 * @sock:	socket
	 * @user_data:  user data
	 * @recv_buf:	the buffer storing the received data, its space is allocated by user
	 * @recv_len:	the length of the received data
	 * @addr:	udp used peer address
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*recv_data)(int sock, 
			 unsigned long user_data,
			 struct sockaddr_in *addr);
	/*
	 * handle_data() - user defined function to dispose recvd data
	 * @sock:	socket
	 * @user_data:	user data
	 * @addr:	peer address
	 *
	 * Return 0 if successful, -1 if not
	 */
	int (*handle_data)(
			int sock,
			unsigned long user_data,
			struct tc_link_data *link_data,
			struct sockaddr_in *addr);
	/*
	 * connected_func() - user defined connect function
	 * @user_data:  user_data,
	 * @event:	the event after connected func, user defined
	 * @addr:	udp used peer address
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*connected_func)(unsigned long user_data, int *event, struct sockaddr_in *addr);

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
			unsigned long *user_data);
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
	void (*result_func)(unsigned long user_data, struct tc_link_data *link_data);
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
			struct tc_link_data *link_data,
			struct sockaddr *addr);
};

struct tc_transfer_link {
	int enable;
	int proto;
	int addr;
	int port;
	char unix_path[108];

};

struct tc_create_config {
	char		netcard[IFNAMSIZ];	//网卡名
	char		netmask[16];	//子网掩码
	int		duration;	//程序持续运行时间
	int		port_map;	//针对一个连接管理其他多个连接情况，
					//可以为其他链接预留足够的端口
	int		enable_transfer; //开启数据转发, 1 - 开启， 0 - 不开启
	int		total_link;	//总连接数
	int		ip_count;	//ip个数
	int		connect_timeout; //连接超时
	int		recv_timeout;	//接收超时
	int		add_check;	//添加超时检测
	int		link_type;	//设备类型,server or client
	int		proto;		//协议
	int		linger;		//是否使用reset包断开连接 1 使用，0 不使用
	
	int		open_push;	//是否开启消息推送, 1 开启， 0 不开启(未实现)
	int		stack_size;	//线程栈大小
	int		hub_interval;	//心跳间隔
	int		hub_enable;	//是否开启心跳, 1 开启， 0 不开启
	int		rendevous_enable; //是否开启集合点
	unsigned int	server_ip;	//服务器ip
	unsigned int	start_ip;	//起始ip
	unsigned short  hub_num;	//心跳模块线程数
	unsigned short  link_num;	//连接创建模块线程数
	unsigned short  recv_num;	//接收数据模块线程数
	unsigned short  send_num;	//发送数据模块线程数
	unsigned short  timer_num;	//定时器线程数
	unsigned short	handle_num;	//处理数据模块线程数据
	unsigned short  end_port;	//结束端口
	unsigned short  start_port;	//起始端口
	unsigned short  server_port;	//服务器端口
	char		res[2];
	struct tc_transfer_link	transfer_server;	//数据转发服务器配置
	struct tc_transfer_link	transfer_client;	//数据转发客户端配置
};

/*
 * tc_link_create() - create the link module
 * @thread_num:		the number of threads this module needs
 * @stack_size:		the size of the thread's  stack
 * @thread_name:	thread name
 * @oper:		create operations, set struct tc_create_link_oper
 * @config:		some configure of this module, set struct tc_create_config
 *
 * Return: 0 if successful, -1 if not
 */
int
tc_link_create(
	struct tc_create_link_oper *oper
);

/*
 * tc_create_data_add() - add a new socket
 * @port_map_cnt:  the port count of the link
 */
int
tc_create_data_add(
	int transfer_flag,
	int port_map_cnt,
	struct in_addr ip,
	unsigned short port
);

#endif
