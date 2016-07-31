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
	TC_STATUS_MAX
};

struct tc_link_timeout_node {
	char *name;
	struct timespec send_time;
	struct hlist_node node;
};


struct tc_create_link_data {
	unsigned long		user_data;	//用户数据
	unsigned long		timer_data;     //超时节点链表对应的结构指针
	struct tc_link_data	link_data;	//连接数据
	struct tc_timeout_data	timeout_data;	//超时数据
	struct tc_create_link_oper *epoll_oper;	//epoll的操作

	pthread_mutex_t		data_mutex;	//数据锁
	pthread_mutex_t		*hlist_mutex;	//hash链表对应的锁指针
	struct hlist_node	node;
	struct list_head	rc_node;	//超时检测节点
};

struct tc_epoll_data *
tc_epoll_data_alloc(
	int sock, 
	int conn_timeout,
	int recv_timeout,
	unsigned long user_data,
	struct in_addr local_addr,
	struct in_addr peer_addr,
	unsigned short local_port,
	unsigned short peer_port
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
	int (*traversal_func)(unsigned long user_data, struct hlist_node *hnode)
);

int
tc_create_link_data_traversal_del(
	unsigned long user_data,
	int (*traversal_func)(unsigned long user_data, struct hlist_node *node)
);

int
tc_create_link_data_del(
	struct tc_create_link_data  *data
);

#endif
