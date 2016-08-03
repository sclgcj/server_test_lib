#ifndef TC_CREATE_PRIVATE_H
#define TC_CREATE_PRIVATE_H 1

#include "tc_hash.h"
#include "tc_epoll.h"
#include "tc_create.h"

struct tc_timeout_data {
	int check_flag;				//是否开启了检测，1 为开启， 0为未开启
	int conn_timeout;			//连接超时时间
	int recv_timeout;			//接收超时时间
//	struct timespec send_time;		//发送包的时间
	tc_hash_handle_t timeout_hash;		//超时hash表
};

enum {
	TC_STATUS_CONNECT,
	TC_STATUS_SEND_DATA,
	TC_STATUS_LISTEN,
	TC_STATUS_DISCONNECT,
	TC_STATUS_MAX
};

enum {
	TC_LINK_TCP_CLIENT,
	TC_LINK_TCP_SERVER,
	TC_LINK_UDP_CLIENT,
	TC_LINK_UDP_SERVER,
	TC_LINK_HTTP_CLIENT,
	TC_LINK_UNIX_TCP_CLIENT,
	TC_LINK_UNIX_TCP_SERVER,
	TC_LINK_UNIX_UDP_CLIENT,
	TC_LINK_UNIX_UDP_SERVER,
	TC_LINK_TRASVERSAL_TCP_CLIENT,
	TC_LINK_TRASVERSAL_TCP_SERVER,
	TC_LINK_TRASVERSAL_UDP_CLIENT,
	TC_LINK_TRASVERSAL_UDP_SERVER,
	TC_LINK_MAX
};

struct tc_link_timeout_node {
	char *name;
	struct timespec send_time;
	struct hlist_node node;
};

struct tc_link_private_data {
	int		sock;			//套接字
	int		status;			//内部维护的状态，主要用于区分是否是创建连接
	int		link_id;		//用于标识每一个连接,其实是做hash的时候使用

	int		link_type;		//连接类型, client or server
					
	int		hub_interval;		//心跳间隔，由于有些服务器的心跳间隔会在运行过程中
						//发生变化，因此，提供此参数，当心跳变化时，需要对
						//其重新赋值
	int		port_num;		//用于标识在port_map中，该连接使用的是第几个端口
};


struct tc_create_link_data {
	unsigned long		user_data;	//用户数据
	unsigned long		timer_data;     //超时节点链表对应的结构指针
	unsigned long		hub_data;	//心跳包数据, 当连接断开时，删除该数据
	struct tc_link_data	link_data;	//连接数据
	struct tc_link_private_data private_link_data; //连接私有数据
	struct tc_timeout_data	timeout_data;	//超时数据
	struct tc_create_link_oper *epoll_oper;	//epoll的操作

	pthread_mutex_t		data_mutex;	//数据锁
	pthread_mutex_t		*hlist_mutex;	//hash链表对应的锁指针
	struct hlist_node	node;
	struct list_head	rc_node;	//超时检测节点
};

struct tc_create_link_data *
tc_create_link_data_alloc(
	int sock, 
	char *path,
	unsigned long user_data,
	struct in_addr local_addr,
	struct in_addr peer_addr,
	unsigned short local_port,
	unsigned short peer_port
);

int
tc_sock_event_add(
	int sock,
	int event,
	struct tc_create_link_data *epoll_data
);

/*
 * tc_link_create_start() - start the link create thread
 *
 * Return: 0 if successful, -1 if not
 */
int tc_link_create_start();

int
tc_create_link_data_traversal(
	unsigned long user_data,
	int (*traversal_func)(unsigned long user_data, struct hlist_node *hnode, int *flag)
);

int
tc_create_link_data_traversal_del(
	unsigned long user_data,
	int (*traversal_func)(unsigned long user_data, struct hlist_node *node, int *flag)
);

int
tc_create_link_data_del(
	struct tc_create_link_data  *data
);

int
tc_create_check_duration();

#endif
