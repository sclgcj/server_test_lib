#include "tc_comm.h"
#include "tc_init.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_create.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
#include "tc_socket_private.h"
#include "tc_rendezvous_private.h"
#include "tc_recv_check_private.h"


/*
 * exist just one
 */

enum {
	TC_LINK_TCP_CLIENT,
	TC_LINK_TCP_SERVER,
	TC_LINK_UDP_CLIENT,
	TC_LINK_UDP_SERVER,
	TC_LINK_HTTP_CLIENT,
	TC_LINK_MAX
};

struct tc_create_data {
	struct in_addr addr;
	unsigned short port;
	unsigned long user_data;
	struct list_head node;
};

struct tc_create{
	int create_hash_num;
	int create_link_count;
	int create_id;
	int create_link_id;
	pthread_mutex_t count_mutex;
	tc_hash_handle_t create_hash;
	struct tc_recv_check_handle *recv_check;
	struct tc_create_link_oper oper;
	struct tc_create_config config;
};

#define TC_TIMEOUT_HASH_NUM  2
#define TC_CREATE_BASE  100
static struct tc_create global_create_link;

static int
tc_link_timeout_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	char name = 0;
	struct tc_link_timeout_node *lt_node = NULL;
	
	if (!hnode) {
		if (!user_data)
			return 0;
		else 
			name = ((char*)user_data)[0];
	}
	else {
		lt_node = tc_list_entry(hnode, struct tc_link_timeout_node, node);
		if (!lt_node->name)
			return 0;
		name = lt_node->name[0];
	}

	return (name % TC_TIMEOUT_HASH_NUM);
}

static int
tc_link_timeout_hash_get(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	struct tc_link_timeout_node *lt_node = NULL;

	lt_node = tc_list_entry(hnode, struct tc_link_timeout_node, node);
	if (!user_data && lt_node->name)
		return TC_OK;
	if (!user_data || !lt_node->name)
		return TC_ERR;
	if (strcmp((char*)user_data, lt_node->name))
		return TC_ERR;

	return TC_OK;
}

static int
tc_link_timeout_hash_destroy(
	struct hlist_node	*hnode
)
{
	struct tc_link_timeout_node *lt_node = NULL;

	lt_node = tc_list_entry(hnode, struct tc_link_timeout_node, node);
	TC_FREE(lt_node->name);
	TC_FREE(lt_node);

	return TC_OK;
}

struct tc_create_link_data *
tc_create_link_data_alloc(
	int sock, 
	int conn_timeout,
	int recv_timeout,
	unsigned long user_data,
	struct in_addr local_addr,
	struct in_addr peer_addr,
	unsigned short local_port,
	unsigned short peer_port
)
{
	struct tc_create_link_data *data = NULL;

	data = (struct tc_create_link_data*)calloc(1, sizeof(*data));
	if (!data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	data->user_data			= user_data;
	data->timeout_data.recv_timeout = recv_timeout;
	data->timeout_data.conn_timeout = conn_timeout;
	data->link_data.sock		= sock;
	data->link_data.peer_port	= peer_port;
	data->link_data.peer_addr	= peer_addr;
	data->link_data.local_addr	= local_addr;
	data->link_data.local_port	= local_port;
	data->link_data.parent		= (unsigned long)data;

	data->timeout_data.timeout_hash = tc_hash_create(
							TC_TIMEOUT_HASH_NUM,
							tc_link_timeout_hash,
							tc_link_timeout_hash_get,
							tc_link_timeout_hash_destroy);
	if (data->timeout_data.timeout_hash == TC_HASH_ERR) 
		TC_FREE(data);


	return data;
}

static void
tc_link_type_get(
	struct tc_create_config *config,
	int *type
)
{
	if (config->proto == TC_PROTO_TCP && 
			config->link_type == TC_DEV_CLIENT) 
		(*type) = TC_LINK_TCP_CLIENT;
	else if (config->proto == TC_PROTO_TCP && 
			config->link_type == TC_DEV_SERVER)
		(*type) = TC_LINK_TCP_SERVER;
	else if (config->proto == TC_PROTO_UDP && 
			config->link_type == TC_DEV_CLIENT)
		(*type) = TC_LINK_UDP_CLIENT;
	else if (config->proto == TC_PROTO_UDP && 
			config->link_type == TC_DEV_SERVER)
		(*type) = TC_LINK_UDP_SERVER;
	else if (config->proto == TC_PROTO_HTTP &&
			config->link_type == TC_DEV_CLIENT)
		(*type) = TC_LINK_HTTP_CLIENT;
	else
		(*type) = TC_LINK_MAX;
}

static int
tc_create_proto_link(
	int			sock,
	struct tc_create_data   *create_data,
	struct tc_create_config *config,
	int			*event,
	int			*status
)
{
	int ret = 0, type = 0;
	struct in_addr server_addr;
	/*
	 * we want to make it more flexible for upstreams, so that they can create 
	 * link all by themselves. However, we find many upstreams may use the same
	 * fucntions to create link, listen or do some other protocol command. At 
	 * this situation, we decide to integrate them in the downsteam, and upstreams
	 * can use configuration to decide what to do. Of course, we will also keep the 
	 * user defined interface.
	 */
	server_addr.s_addr = global_create_link.config.server_ip;
	tc_link_type_get(config, &type);
	switch (type) {
	case TC_LINK_TCP_CLIENT:
		ret = tc_tcp_connect(
				sock,
				server_addr,
				global_create_link.config.server_port);
		*event = EPOLLONESHOT | EPOLLOUT;
		(*status) = TC_STATUS_CONNECT;
		break;
	case TC_LINK_TCP_SERVER:
		listen(sock, 10);
		*event = EPOLLONESHOT | EPOLLIN;
		(*status) = TC_STATUS_LISTEN;
		//ret = tc_tcp_accept(sock);
		break;
	case TC_LINK_UDP_CLIENT:
		ret = tc_udp_connect(
				sock,
				server_addr,
				global_create_link.config.server_port);
		*event = EPOLLONESHOT | EPOLLOUT;
		(*status) = TC_STATUS_CONNECT;
		break;
	case TC_LINK_UDP_SERVER:
		*event = EPOLLONESHOT | EPOLLIN;
		(*status) = TC_STATUS_LISTEN;
		//ret = tc_udp_accept(sock);
		break;
	case TC_LINK_HTTP_CLIENT:
		break;
	default:
		if (global_create_link.oper.create_link) {
			ret = global_create_link.oper.create_link(
							sock,	
							create_data->addr,
							create_data->port, 
							create_data->user_data);
			if (ret != TC_OK){
				close(sock);
				return ret;
			}
			(*status) = TC_STATUS_CONNECT;
		}
	}

	return ret;
}

static int
tc_link_create_handle(
	struct list_head *node
)
{
	int ret = 0;
	int sock = 0;
	int event = 0;
	int status = 0;
	struct in_addr server_addr;
	struct tc_create_link_data *epoll_data = NULL;
	struct tc_create_data  *create_data = NULL;

	create_data = tc_list_entry(node, struct tc_create_data, node);

	//PRINT("--ip = %s:%d\n", inet_ntoa(create_data->addr), create_data->port);
	ret = tc_create_socket(
			global_create_link.config.proto, 
			global_create_link.config.linger,
			create_data->addr, 
			create_data->port,
			&sock);
	if (ret != TC_OK) 
		return ret;

	status = TC_STATUS_CONNECT;

	server_addr.s_addr = global_create_link.config.server_ip;
	epoll_data = tc_create_link_data_alloc(
				sock,	
				global_create_link.config.connect_timeout, 
				global_create_link.config.recv_timeout,
				create_data->user_data,
				create_data->addr,
				server_addr,
				create_data->port,
				global_create_link.config.server_port);
	if (!epoll_data)
		return TC_ERR;

/*	ret = tc_create_proto_link(
				sock, 
				create_data, 
				&global_create_link.config, 
				&event, 
				&status);
	if (ret != TC_OK) 
		TC_PANIC("create proto link error\n");*/

	epoll_data->link_data.status = status;
	epoll_data->epoll_oper = &global_create_link.oper;
	//start recv_check
	tc_recv_check_start("connect", &epoll_data->link_data);
	/*
	//add to epoll
	ret = tc_epoll_data_add(sock, event, (unsigned long)epoll_data);
	if (ret != TC_OK)
		return ret;
	//PRINT("add_check = %d, %p\n", global_create_link.config.add_check, epoll_data);
*/	
	//add recv check
	if (global_create_link.config.add_check)
		tc_recv_check_add(global_create_link.recv_check, epoll_data);

	TC_FREE(create_data);

	return TC_OK;
}

static void
tc_netmask_num_get(
	char *netmask,
	int  *netmask_num
)
{
	unsigned int ip = inet_addr(netmask);
	unsigned int cnt = 0;

	while(cnt < 32)
	{
		if( ip & 1 << cnt)
		{
			(*netmask_num)++;
		}
		cnt++;
	}
}

static void
tc_link_netcard_config(
	struct tc_create_config *create_config
)
{
	int i = 0;
	int netmask_num = 0;
	char cmd[128] = { 0 };
	unsigned int ip = create_config->start_ip;
	struct in_addr addr;

	if (!create_config->netmask[0])	
		memcpy(create_config->netmask, "255.255.255.0", 13);

	tc_netmask_num_get(create_config->netmask, &netmask_num);
	for (i = 1; i < create_config->ip_count - 1; i++) {
		ip += 1 << 24;
		addr.s_addr = ip;
		memset(cmd, 0, 128);
		sprintf(cmd, "ip addr add %s/%d dev %s", 
				inet_ntoa(addr), netmask_num, create_config->netcard);
		system(cmd);
	}
}

static void
tc_link_epoll_set(
	struct tc_create_config *create_config
)
{
	struct tc_epoll_oper oper;

	memset(&oper, 0, sizeof(oper));

	tc_epoll_config_set(create_config->duration, &oper);
}

static void
tc_link_environment_set(
	struct tc_create_config *create_config
)
{
	tc_link_netcard_config(create_config);
	tc_link_epoll_set(create_config);
}

static int
tc_create_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	int hash_data = 0;
	struct tc_create_link_data *data = NULL;

	if (!hnode) 
		hash_data = (int)user_data;
	else {
		data = tc_list_entry(hnode, struct tc_create_link_data, node);
		hash_data = data->link_data.link_id;
	}

	return (hash_data % global_create_link.create_hash_num);
}

static int
tc_create_hash_get(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	struct tc_create_link_data *data = NULL;	

	data = tc_list_entry(hnode, struct tc_create_link_data, node);
	if (data->link_data.link_id == (int)user_data)
		return TC_OK;

	return TC_ERR;
}

static int
tc_create_hash_destroy(
	struct hlist_node	*hnode
)
{
	struct tc_create_link_data *data = NULL;

	data = tc_list_entry(hnode, struct tc_create_link_data, node);

	if (data->timeout_data.timeout_hash != TC_HASH_ERR)
		tc_hash_destroy(data->timeout_data.timeout_hash);

	close(data->link_data.sock);
	pthread_mutex_destroy(&data->data_mutex);
	TC_FREE(data);

	return TC_OK;
}

int
tc_create_link_data_del(
	struct tc_create_link_data  *data
)
{
	int ret = 0;

	ret = tc_hash_del(
			global_create_link.create_hash,
			&data->node,
			data->link_data.link_id);

	tc_create_hash_destroy(&data->node);

	return TC_OK;
}

int
tc_create_link_data_traversal(
	unsigned long user_data,
	int (*traversal_func)(unsigned long user_data, struct hlist_node *hnode)
)
{
	TC_HASH_WALK(global_create_link.create_hash, user_data, traversal_func);
	return TC_OK;
}

int
tc_create_link_data_traversal_del(
	unsigned long user_data,
	int (*traversal_func)(unsigned long user_data, struct hlist_node *node)
)
{
	TC_HASH_WALK_DEL(global_create_link.create_hash, user_data, traversal_func);
	return TC_OK;
}

int
tc_link_create(
	int thread_num,
	int stack_size,
	char *thread_name,
	struct tc_create_link_oper *oper,
	struct tc_create_config *create_config
)
{
	int ret = 0;
	int hash_num = 0;

	memcpy(&global_create_link.config, create_config, sizeof(*create_config));
	memcpy(&global_create_link.oper, oper, sizeof(*oper));

	global_create_link.recv_check = tc_recv_check_create(
						global_create_link.config.recv_timeout);
	if (!global_create_link.recv_check)
		return TC_ERR;

	ret = tc_thread_pool_create(
			thread_num, 
			stack_size, 
			thread_name?:"create_link2", 
			NULL,
			NULL,
			tc_link_create_handle, 
			&global_create_link.create_id);
	if (ret != TC_OK)
		return ret;

	if (global_create_link.config.total_link > TC_CREATE_BASE)
		global_create_link.create_hash_num = global_create_link.config.total_link / TC_CREATE_BASE + 1;
	else
		global_create_link.create_hash_num = 1;
	global_create_link.create_hash = tc_hash_create(
						global_create_link.create_hash_num,
						tc_create_hash,
						tc_create_hash_get,
						tc_create_hash_destroy);

	tc_link_environment_set(create_config);
	return TC_OK;
}

static int
tc_create_uninit()
{
	return TC_OK;
}

static struct tc_create_data *
tc_create_data_calloc(
	struct in_addr  ip,
	unsigned short	port,
	unsigned long   user_data
)
{
	struct tc_create_data *data = NULL;

	data = (struct tc_create_data*)calloc(1, sizeof(*data));
	if (!data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	data->port = port;
	data->user_data = user_data;
	data->addr.s_addr = ip.s_addr;

	return data;
}

int
tc_create_data_add(
	int port_map_cnt,
	struct in_addr ip,
	unsigned short port
)
{
	unsigned long user_data;
	struct tc_create_data *data = NULL;

	if (global_create_link.oper.prepare_data_get)
		global_create_link.oper.prepare_data_get(port_map_cnt, &user_data);	
	data = tc_create_data_calloc(ip, port, user_data);
	if (!data) {
		return TC_ERR;
	}
	tc_thread_pool_node_add(global_create_link.create_id, &data->node);

	return TC_OK;
}

static int
tc_create_link(
	struct list_head *node
)
{
	int i = 0, j = 0;
	int ret = 0, count = 0;
	int port_offset = 0;
	unsigned short end_port = 0;
	unsigned long user_data = 0;
	struct in_addr ip;
	struct tc_create_data *data = NULL;
	struct tc_create_config *config = NULL;

	config = &global_create_link.config;
	port_offset = config->port_map;
	count = config->end_port - config->start_port;
	PRINT("port_offset = %d, %d\n", port_offset, config->port_map);
	end_port = (count - count % port_offset) + config->start_port;
	count = 0;

	ip.s_addr = config->start_ip;
	for (i = 0; i < config->ip_count; i++) {
		if (i != 0)
			ip.s_addr += 1 << 24;
		for (j = config->start_port; j < end_port; j++) {
			//PRINT("ip = %s, port = %d\n", inet_ntoa(ip), j);
			ret = tc_thread_test_exit();
			if (ret == TC_OK) {
				i = config->ip_count;
				j = config->end_port;
				continue;
			}
			ret = tc_create_data_add(0, ip, j);
			if (ret != TC_OK) {
				i = config->ip_count;
				j = config->end_port;
				continue;
			}
			count++;
			if (count >= config->total_link) {
				i = config->ip_count;
				break;
			}
			if (global_create_link.oper.create_flow_ctrl)
				global_create_link.oper.create_flow_ctrl(count);

		}
	}
	return TC_OK;
}

static int
tc_create_setup()
{
	int ret = 0;	

	ret = tc_thread_pool_create(
				0, 
				32 * 1024, 
				"create_link", 
				NULL,
				tc_create_link,
				NULL,
				&global_create_link.create_link_id);
	if (ret != TC_OK)
		return ret;

	return TC_OK;
}

int 
tc_link_create_start()
{
	struct in_addr addr;
	struct tc_create_data *thread_data = NULL;

	addr.s_addr = 0;
	thread_data = tc_create_data_calloc(addr, 0, 0);
	if (!thread_data) 
		return TC_ERR;

	PRINT("create_link_id = %d, create_id = %d\n", global_create_link.create_link_id, global_create_link.create_id);
	return tc_thread_pool_node_add(global_create_link.create_link_id, &thread_data->node); 
} 

int tc_create_init() 
{ 
	int ret = 0; 
	
	memset(&global_create_link, 0, sizeof(global_create_link));

	ret = tc_init_register(tc_create_setup); 
	if (ret != TC_OK)
		return ret; 
	return tc_uninit_register(tc_create_uninit);
}

TC_MOD_INIT(tc_create_init);
