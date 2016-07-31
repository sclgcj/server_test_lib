#ifndef TC_CREATE_H
#define TC_CREATE_H

#include "tc_epoll.h"
#include <net/if.h>

struct tc_link_data {
	int		sock;			//套接字
	int		status;			//内部维护的状态，主要用于区分是否是创建连接
	int		link_id;		//用于标识每一个连接,其实是做hash的时候使用

	int		err_flag;		//错误标记，由用户设置该标志，如果为1, 则只会将
						//该连接移除epoll，如果为2，则会将和该连接有关的
						//所有数据均移除。这么做的目的是因为服务器和客户端
						//对连接失败的处理是不同的。为0，则不作任何操作
						
	int		port_num;		//用于标识在port_map中，该连接使用的是第几个端口
	struct in_addr	peer_addr;		//对端地址
	struct in_addr 	local_addr;		//本地地址
	unsigned short 	peer_port;		//对端端口
	unsigned short 	local_port;		//本地端口
	unsigned long   parent;			//上级结构
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
			 char *send_buf, 
			 int send_len, 
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
			 char **recv_buf, 
			 int *recv_len, 
			 struct sockaddr_in *addr);
	/*
	 * connected_func() - user defined connect function
	 * @sock:	socket
	 * @user_data:  user_data,
	 * @addr:	udp used peer address
	 *
	 * Return: 0 if successful, -1 if not
	 */
	int (*connected_func)(int sock, unsigned long user_data, struct sockaddr_in *addr);
};



struct tc_create_config {
	char		netcard[IFNAMSIZ];
	char		netmask[16];
	int		duration;	//程序持续运行时间
	int		port_map;	//针对一个连接管理其他多个连接情况，
					//可以为其他链接预留足够的端口
	int		total_link;	//总连接数
	int		ip_count;	//ip个数
	int		connect_timeout; //连接超时
	int		recv_timeout;	//接收超时
	int		add_check;	//添加超时检测
	int		link_type;	//设备类型,server or client
	int		proto;		//协议
	int		linger;		//是否使用reset包断开连接 1 使用，0不使用
	unsigned int	server_ip;
	unsigned int	start_ip;	//起始ip
	unsigned short  end_port;	//结束端口
	unsigned short  start_port;	//起始端口
	unsigned short  server_port;
	char		res[2];
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
	int thread_num,
	int stack_size,
	char *thread_name,
	struct tc_create_link_oper *oper,
	struct tc_create_config *config
);

/*
 * tc_create_data_add() - add a new socket
 * @port_map_cnt:  the port count of the link
 */
int
tc_create_data_add(
	int port_map_cnt,
	struct in_addr ip,
	unsigned short port
);

#endif
