#include "tc_comm.h"
#include "tc_init.h"
#include "tc_cmd.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_create.h"
#include "tc_recv_check.h"
#include "tc_hub_private.h"
#include "tc_recv_private.h"
#include "tc_send_private.h"
#include "tc_epoll_private.h"
#include "tc_timer_private.h"
#include "tc_create_private.h"
#include "tc_socket_private.h"
#include "tc_handle_private.h"
#include "tc_timer_list_private.h"
#include "tc_rendezvous_private.h"
#include "tc_recv_check_private.h"
#include "tc_interface_private.h"


/*
 * exist just one
 */

struct tc_create_user_data {
	char *data;
	int realloc;
	int data_cnt;
	int data_size;
	pthread_mutex_t mutex;
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
	struct tc_create_user_data user_data;
};

/*
 * tc_create_data_add() - add a new socket
 * @port_map_cnt:  the port count of the link
 */
static int
tc_create_data_add(
	int proto,
	int link_type,
	int transfer_flag,
	int port_map_cnt,
	struct in_addr ip,
	struct in_addr server_ip,
	unsigned short server_port,
	unsigned short port
);

static int
tc_create_hash_destroy(
	struct hlist_node	*hnode
);

#define TC_TIMEOUT_HASH_NUM  2
#define TC_CREATE_BASE  100
static struct tc_create global_create_link;

void
tc_create_user_data_get(
	int id,	
	unsigned long *cl_user_data
)
{
	int data_size = 0;
	int old_size = 0, new_size = 0;
	struct tc_create_user_data *user_data = &global_create_link.user_data;

	if (id >= user_data->data_cnt) {
		PRINT("lind num larger the one set starting\n");
		return;
	}

//	pthread_mutex_lock(&user_data->mutex);
	data_size = user_data->data_size;
	//PRINT("id = %d, %d\n", id, user_data->data_cnt);
/*	if (id >= user_data->data_cnt) {
		old_size = user_data->data_cnt * data_size;
		user_data->data_cnt *= 2;
		new_size = user_data->data_cnt * data_size;
		user_data->data = (char *)realloc(
						user_data->data, 
						new_size);
		if (!user_data->data) 
			TC_PANIC("Not enough data When create num =%d, "
					"total_num = %d, size = %d", 
					id,
					user_data->data_cnt, 
					user_data->data_cnt * data_size);
		PRINT("old_size = %d, new_size = %d\n", new_size, old_size);
		memset(user_data->data + old_size, 0, new_size - old_size);
	}  */
	//pthread_mutex_unlock(&user_data->mutex);

	PRINT("id = %d, data_size =%d\n", id, data_size);
	*cl_user_data = (unsigned long)&user_data->data[id * data_size];
}

int
tc_create_link_err_handle(
	struct tc_create_link_data *cl_data 
)
{
	int ret = 0;

	if (cl_data->private_link_data.err_flag == 1) {
		tc_create_link_data_del(cl_data);
		ret = TC_ERR;
	} else if (cl_data->private_link_data.err_flag == 2){
		tc_create_link_data_destroy(cl_data);
		ret = TC_ERR;
	} /*else if (traversal_data.count == 0)
		ret = TC_ERR;*/

	return ret;
}

int
tc_create_check_duration()
{
	int tick = 0;

	tick = tc_timer_tick_get();
	if (tick + 5 >= global_create_link.config.duration) 
		return TC_OK;

	return TC_ERR;
}

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
	if (!user_data && !lt_node->name)
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
tc_create_link_data_get(
	unsigned long link_data
)
{
	int id = 0;
	struct hlist_node *hnode = NULL;
	struct tc_create_link_data *data = NULL;
	struct tc_create_user_data *user_data = NULL;

	user_data = &global_create_link.user_data;

	pthread_mutex_lock(&user_data->mutex);
	id = labs(link_data - (unsigned long)user_data->data) / user_data->data_size;
//	PRINT("link_data = %p, data = %p, id = %d, data_size = %d\n", 
//			(char*)link_data, user_data->data, id, user_data->data_size);
	pthread_mutex_unlock(&user_data->mutex);
	hnode = tc_hash_get(global_create_link.create_hash, id, id);
	if (!hnode) 
		TC_PANIC("Wrong link id = %d\n", id);

	data = tc_list_entry(hnode, struct tc_create_link_data, node);

	return data;
}


struct tc_create_link_data *
tc_create_link_data_alloc(
	int sock, 
	char *path,
	struct in_addr peer_addr,
	unsigned short peer_port,
	struct tc_create_data *create_data
)
{
	int path_len = 0;
	struct tc_create_link_data *data = NULL;

	data = (struct tc_create_link_data*)calloc(1, sizeof(*data));
	if (!data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	data->private_link_data.link_id = create_data->link_id;
	data->user_data			= create_data->user_data;
	data->timeout_data.check_flag	= global_create_link.config.add_check;
	data->timeout_data.recv_timeout = global_create_link.config.recv_timeout;
	data->timeout_data.conn_timeout = global_create_link.config.connect_timeout;
	data->private_link_data.sock		= sock;
	data->link_data.peer_port	= peer_port;
	data->link_data.peer_addr	= peer_addr;
	data->link_data.local_addr	= create_data->addr;
	data->link_data.local_port	= create_data->port;
	data->private_link_data.hub_interval    = global_create_link.config.hub_interval;
	data->private_link_data.port_num++;
	data->private_link_data.recv_cnt = TC_DEFAULT_RECV_BUF;
	data->epoll_oper		= &global_create_link.oper;
	data->config			= &global_create_link.config;
	INIT_LIST_HEAD(&data->private_link_data.send_list);
	pthread_mutex_init(&data->private_link_data.mutex, NULL);
	pthread_mutex_init(&data->private_link_data.send_mutex, NULL);
	pthread_mutex_init(&data->data_mutex, NULL);
	pthread_mutex_init(&data->recv_mutex, NULL);
	if (path && path[0] != '\0') {
		path_len = strlen(data->link_data.unix_path);
		data->link_data.unix_path = (char*)calloc(1, path_len + 1);
		if (!data->link_data.unix_path) 
			TC_PANIC("not enough memory\n");
		memcpy(data->link_data.unix_path, path, path_len);
	}

	data->timeout_data.timeout_hash = tc_hash_create(
							TC_TIMEOUT_HASH_NUM,
							tc_link_timeout_hash,
							tc_link_timeout_hash_get,
							tc_link_timeout_hash_destroy);
	if (data->timeout_data.timeout_hash == TC_HASH_ERR) 
		TC_FREE(data);

	tc_hash_add(
		global_create_link.create_hash, 
		&data->node, 
		data->private_link_data.link_id);

/*	if (data->epoll_oper->extra_data_set)
		data->epoll_oper->extra_data_set(
					(unsigned long)data,
					create_data->user_data);*/

	return data;
}

static void
tc_link_type_get(
	int transfer_flag,
	struct tc_create_config *config,
	int *type
)
{
	if (config->proto == TC_PROTO_TCP && 
			config->link_type == TC_DEV_CLIENT) {
		if (transfer_flag == 0)
			(*type) = TC_LINK_TCP_CLIENT;
		else
			(*type) = TC_LINK_TRASVERSAL_TCP_CLIENT;
	}
	else if (config->proto == TC_PROTO_TCP && 
			config->link_type == TC_DEV_SERVER) {
		if (transfer_flag == 0)
			(*type) = TC_LINK_TCP_SERVER;
		else
			(*type) = TC_LINK_TRASVERSAL_TCP_SERVER;
	}
	else if (config->proto == TC_PROTO_UDP && 
			config->link_type == TC_DEV_CLIENT) {
		if (transfer_flag == 0)
			(*type) = TC_LINK_UDP_CLIENT;
		else
			(*type) = TC_LINK_TRASVERSAL_UDP_CLIENT;
	}
	else if (config->proto == TC_PROTO_UDP && 
			config->link_type == TC_DEV_SERVER) {
		if (transfer_flag == 1)
			(*type) = TC_LINK_UDP_SERVER;
		else
			(*type) = TC_LINK_TRASVERSAL_UDP_SERVER;
	}
	else if (config->proto == TC_PROTO_HTTP &&
			config->link_type == TC_DEV_CLIENT)
		(*type) = TC_LINK_HTTP_CLIENT;
	else if (config->proto == TC_PROTO_UNIX_TCP && 
			config->link_type == TC_DEV_CLIENT)
		(*type) = TC_LINK_UNIX_TCP_CLIENT;
	else if (config->proto == TC_PROTO_UNIX_TCP && 
			config->link_type == TC_DEV_SERVER)
		(*type) = TC_LINK_UNIX_TCP_SERVER;
	else if (config->proto == TC_PROTO_UNIX_UDP && 
			config->link_type == TC_DEV_CLIENT)
		(*type) = TC_LINK_UNIX_UDP_CLIENT;
	else if (config->proto == TC_PROTO_UNIX_UDP && 
			config->link_type == TC_DEV_SERVER)
		(*type) = TC_LINK_UNIX_UDP_SERVER;
	else
		(*type) = TC_LINK_MAX;
}

static int
tc_create_proto_link(
	int			sock,
	struct tc_create_data   *create_data,
	struct tc_create_config *config,
	int			*event,
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	int *status = &cl_data->private_link_data.status;
	struct in_addr server_addr;
	/*
	 * we want to make it more flexible for upstreams, so that they can create 
	 * link all by themselves. However, we find many upstreams may use the same
	 * fucntions to create link, listen or do some other protocol command. At 
	 * this situation, we decide to integrate them in the downsteam, and upstreams
	 * can use configuration to decide what to do. Of course, we will also keep the 
	 * user defined interface.
	 */
//	server_addr.s_addr = global_create_link.config.server_ip;
	server_addr.s_addr = create_data->server_ip.s_addr;
	tc_link_type_get(
			create_data->transfer_flag, 
			config, 
			&cl_data->private_link_data.link_type);
	switch (cl_data->private_link_data.link_type) {
	case TC_LINK_TCP_CLIENT:
		ret = tc_tcp_connect(
				sock,
				server_addr,
				create_data->server_port);
		*event = TC_EVENT_WRITE;
		(*status) = TC_STATUS_CONNECT;
		break;
	case TC_LINK_TCP_SERVER:
		listen(sock, 10);
		*event = TC_EVENT_READ;
		(*status) = TC_STATUS_LISTEN;
		//ret = tc_tcp_accept(sock);
		break;
	case TC_LINK_UDP_CLIENT:
		ret = tc_udp_connect(
				sock,
				server_addr,
				create_data->server_port);
		*event = TC_EVENT_WRITE;
		(*status) = TC_STATUS_CONNECT;
		break;
	case TC_LINK_UDP_SERVER:
		*event = TC_EVENT_READ;
		(*status) = TC_STATUS_LISTEN;
		//ret = tc_udp_accept(sock);
		break;
	case TC_LINK_HTTP_CLIENT:
		tc_interface_func_execute(cl_data->user_data);
		break;
	case TC_LINK_UNIX_TCP_CLIENT:
		break;
	case TC_LINK_UNIX_TCP_SERVER:
		break;
	case TC_LINK_UNIX_UDP_CLIENT:
		break;
	case TC_LINK_UNIX_UDP_SERVER:
		break;
	default:
		if (global_create_link.oper.create_link) {
			ret = global_create_link.oper.create_link(
							sock,	
							create_data->server_ip,
							create_data->server_port, 
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

int
tc_sock_event_add(
	int sock,
	int event,
	struct tc_create_link_data *epoll_data
)
{
	int ret = 0;

	
	//add to epoll
	ret = tc_epoll_data_add(sock, event, (unsigned long)epoll_data);
	if (ret != TC_OK)
		return ret;
	//PRINT("add_check = %d, %p\n", global_create_link.config.add_check, epoll_data);

	//add recv check
	if (global_create_link.config.add_check) {
		if (epoll_data->private_link_data.link_type == TC_LINK_TCP_CLIENT)
			tc_recv_check_start("connect", epoll_data->user_data);
		tc_recv_check_add(global_create_link.recv_check, epoll_data);
	}

	return TC_OK;
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
	char *path = NULL;
	struct in_addr server_addr;
	struct tc_create_link_data *epoll_data = NULL;
	struct tc_create_data  *create_data = NULL;

	create_data = tc_list_entry(node, struct tc_create_data, node);

	if (global_create_link.config.transfer_server.enable)
		path = global_create_link.config.transfer_server.unix_path;
	else 
		path = global_create_link.config.transfer_client.unix_path;
	//PRINT("--ip = %s:%d\n", inet_ntoa(create_data->addr), create_data->port);
	ret = tc_create_socket(
			global_create_link.config.proto, 
			path,
			global_create_link.config.linger,
			create_data->addr, 
			create_data->port,
			&sock);
	if (ret != TC_OK) 
		return ret;

	server_addr.s_addr = global_create_link.config.server_ip;
	epoll_data = tc_create_link_data_alloc(
				sock,	
				path,
				server_addr,
				global_create_link.config.server_port, 
				create_data);
	if (!epoll_data)
		return TC_ERR;

	ret = tc_create_proto_link(
				sock, 
				create_data, 
				&global_create_link.config, 
				&event, 
				epoll_data);
	if (ret != TC_OK) 
		TC_PANIC("create proto link error\n");

	if (epoll_data->private_link_data.link_type != TC_LINK_HTTP_CLIENT) {
		status = TC_STATUS_CONNECT;
		epoll_data->private_link_data.status = status;

		tc_sock_event_add(sock, event, epoll_data);
	}
	TC_FREE(create_data);

	return TC_OK;
}

int 
tc_create_link_new(
	int proto,
	int link_type,
	struct in_addr server_ip,
	unsigned short server_port,
	unsigned long extra_data	
)
{
	int ret = 0;
	struct tc_create_link_data *link_data = NULL;	

	if (!extra_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	if (server_ip.s_addr == 0) 
		server_ip.s_addr == global_create_link.config.server_ip;
	if (server_port == 0)
		server_port = global_create_link.config.server_port;

	link_data = (struct tc_create_link_data *)extra_data;
	pthread_mutex_lock(&link_data->data_mutex);
	if (link_data->private_link_data.port_num >= global_create_link.config.port_map) {
		TC_ERRNO_SET(TC_PORT_MAP_FULL);
		pthread_mutex_unlock(&link_data->data_mutex);
		return TC_ERR;
	}
	
	PRINT("=============================================================\n");
	ret =  tc_create_data_add(
			proto,
			link_type,
			0, 
			link_data->private_link_data.port_num, 
			link_data->link_data.local_addr,
			server_ip,
			server_port,
			link_data->link_data.local_port + link_data->private_link_data.port_num);
	link_data->private_link_data.port_num++;
	pthread_mutex_unlock(&link_data->data_mutex);	

	return ret;
}

static int
tc_create_same_link(
	struct in_addr ip,
	struct in_addr server_ip,
	unsigned short server_port,
	struct tc_create_link_data *link_data
)
{
	int ret = 0;
	int sock = 0;
	int event = 0;
	char *path = NULL;
	struct tc_create_data create_data;

	PRINT("ip = %s, server_ip = %s, port = %d, server_port = %d\n", 
			inet_ntoa(ip), inet_ntoa(server_ip), 
			link_data->link_data.local_port, server_port);
	memset(&create_data, 0, sizeof(create_data));
	/*create_data = tc_create_data_calloc(
					global_create_link.config.proto,
					global_create_link.config.link_type,
					0,
					link_data->private_link_data.port_num,
					ip,
					server_ip,
					server_port,
					link_data->link_data.local_port, 
					link_data->user_data);*/
	create_data.addr = link_data->link_data.local_addr;
	create_data.port = link_data->link_data.local_port;
	create_data.user_data = link_data->user_data;
	create_data.link_type = global_create_link.config.link_type;
	create_data.server_ip = server_ip;
	create_data.server_port = server_port;

	if (global_create_link.config.transfer_server.enable)
		path = global_create_link.config.transfer_server.unix_path;
	else 
		path = global_create_link.config.transfer_client.unix_path;	
	link_data->private_link_data.sock = -1;
	ret = tc_create_socket(
			global_create_link.config.proto, 
			path,
			global_create_link.config.linger,
			create_data.addr, 
			create_data.port,
			&link_data->private_link_data.sock);
	if (ret != TC_OK) 
		goto out;

	ret = tc_create_proto_link(
				link_data->private_link_data.sock, 
				&create_data,
				&global_create_link.config, 
				&event, 
				link_data);
	if (ret != TC_OK)
		goto out;

	if (link_data->private_link_data.link_type != TC_LINK_HTTP_CLIENT){
		link_data->private_link_data.status = TC_STATUS_CONNECT;
		tc_sock_event_add(link_data->private_link_data.sock, event, link_data);
	}
out:	
	//TC_FREE(create_data);
	return ret;
}

int
tc_create_link_recreate(
	int flag,
	int close_link,
	struct in_addr server_ip,
	unsigned short server_port,
	unsigned long user_data
)
{
	int ret = 0;
	int port_map = 0;
	struct in_addr ip;
	struct tc_create_link_data *link_data = NULL;

	if (!user_data || (flag == 1 && close_link == 2)){
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	//link_data = (struct tc_create_link_data *)extra_data;
	link_data = tc_create_link_data_get(user_data);
	PRINT("close_link = %d, flag = %d\n", close_link, flag);
	switch (close_link) {
	case 0:
		tc_block_fd_set(link_data->private_link_data.sock);
		tc_epoll_data_del(link_data->private_link_data.sock);
		close(link_data->private_link_data.sock);
		break;
	case 1: 
		tc_create_hash_destroy(&link_data->node);
		break;
	}

	if (server_ip.s_addr == 0) 
		server_ip.s_addr = global_create_link.config.server_ip;
	if (server_port == 0)
		server_port = global_create_link.config.server_port;
	ip = link_data->link_data.local_addr;
	if(flag == 0) {
		pthread_mutex_lock(&link_data->data_mutex);
		port_map = link_data->private_link_data.port_num;
		link_data->private_link_data.port_num++;
		pthread_mutex_unlock(&link_data->data_mutex);
		PRINT("=============================================================\n");
		ret = tc_create_data_add(
				global_create_link.config.proto, 
				global_create_link.config.link_type, 
				0,
				port_map++, 
				ip,
				server_ip,
				server_port, 
				link_data->link_data.local_port + port_map);
		return ret;
	}
	PRINT("\n");
	ret = tc_create_same_link(ip, server_ip, server_port, link_data);
	if (ret != TC_OK)
		goto out;

	PRINT("\n");
	pthread_mutex_lock(&link_data->data_mutex);
	link_data->link_data.peer_addr = server_ip;
	link_data->link_data.peer_port = server_port;
	pthread_mutex_unlock(&link_data->data_mutex);
out:
	return ret;
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

static int
tc_create_link_epoll_err(
	int reason, 
	unsigned long data
)
{
	int ret = 0;
	struct tc_create_link_data *cl_data = NULL;

	PRINT("epoll_error :%s\n", strerror(errno));
	cl_data = (struct tc_create_link_data *)data;
	if (cl_data->epoll_oper->err_handle)
		ret = cl_data->epoll_oper->err_handle(
						TC_EPOLL_ERR, 
						cl_data->user_data);
	cl_data->private_link_data.err_flag = ret;

	tc_create_link_err_handle(cl_data);
	
	return TC_OK;
}

static void
tc_link_epoll_set(
	struct tc_create_config *create_config
)
{
	struct tc_epoll_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.epoll_recv = tc_recv_node_add;
	oper.epoll_send = tc_send_node_add;
	oper.epoll_err  = tc_create_link_epoll_err;

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
		hash_data = data->private_link_data.link_id;
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
	if (data->private_link_data.link_id == (int)user_data)
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

	tc_block_fd_set(data->private_link_data.sock);
	tc_epoll_data_del(data->private_link_data.sock);
	close(data->private_link_data.sock);

	if (data->timeout_data.timeout_hash != TC_HASH_ERR)
		tc_hash_destroy(data->timeout_data.timeout_hash);

	pthread_mutex_destroy(&data->data_mutex);
	TC_FREE(data);

	return TC_OK;
}

int
tc_create_link_data_destroy(
	struct tc_create_link_data  *data
)
{
	int ret = 0;

	ret = tc_hash_del(
			global_create_link.create_hash,
			&data->node,
			data->private_link_data.link_id);

	tc_hub_link_del(data->hub_data);
	tc_timer_list_del(data->timer_data);

	tc_create_hash_destroy(&data->node); 
	return TC_OK;
}

int
tc_create_link_data_del(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;

	pthread_mutex_lock(&cl_data->private_link_data.mutex);
	if (cl_data->private_link_data.status == TC_STATUS_DISCONNECT) {
		pthread_mutex_unlock(&cl_data->private_link_data.mutex);
		return TC_OK;
	}
	cl_data->private_link_data.status = TC_STATUS_DISCONNECT;
	pthread_mutex_unlock(&cl_data->private_link_data.mutex);

	tc_epoll_data_del(cl_data->private_link_data.sock);
	tc_block_fd_set(cl_data->private_link_data.sock);
	close(cl_data->private_link_data.sock);
//	pthread_mutex_lock(&cl_data->data_mutex);
	tc_hub_link_del(cl_data->hub_data);
	tc_timer_list_del(cl_data->timer_data);
//	pthread_mutex_unlock(&cl_data->data_mutex);	
}

int
tc_create_link_data_traversal(
	unsigned long user_data,
	int (*traversal_func)(unsigned long user_data, struct hlist_node *hnode, int *flag)
)
{
	TC_HASH_WALK(global_create_link.create_hash, user_data, traversal_func);
	return TC_OK;
}

int
tc_create_link_data_traversal_del(
	unsigned long user_data,
	int (*traversal_func)(unsigned long user_data, struct hlist_node *node, int *flag)
)
{
	TC_HASH_WALK_DEL(global_create_link.create_hash, user_data, traversal_func);
	return TC_OK;
}

int
tc_link_create(
	int user_data_size,
	struct tc_create_link_oper *oper
)
{
	int ret = 0;
	int hash_num = 0;

//	memcpy(&global_create_link.config, create_config, sizeof(*create_config));
	memcpy(&global_create_link.oper, oper, sizeof(*oper));
	global_create_link.user_data.data_size = user_data_size;
	global_create_link.recv_check = tc_recv_check_create(
						global_create_link.config.recv_timeout);
	if (!global_create_link.recv_check)
		return TC_ERR;

	if (global_create_link.config.link_num <= 1) 
		ret = tc_thread_pool_create(
			global_create_link.config.link_num, 
			global_create_link.config.stack_size, 
			"create_link2", 
			NULL,
			tc_link_create_handle, 
			NULL,
			&global_create_link.create_id);
	else
		ret = tc_thread_pool_create(
			global_create_link.config.link_num, 
			global_create_link.config.stack_size, 
			"create_link2", 
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
	pthread_mutex_init(&global_create_link.count_mutex, NULL);
	pthread_mutex_init(&global_create_link.user_data.mutex, NULL);

	/*if (global_create_link.config.enable_transfer){
		struct in_addr addr;
		addr.s_addr = 0;
		tc_create_data_add(1, -1, addr, 0);
	}*/

	tc_link_environment_set(&global_create_link.config);
	return TC_OK;
}

static int
tc_create_uninit()
{
	return TC_OK;
}

struct tc_create_data *
tc_create_data_calloc(
	int		proto,
	int		link_type,
	int		transfer_flag,
	int		port_map_cnt,
	struct in_addr  ip,
	struct in_addr  server_ip,
	unsigned short  server_port,
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
	data->port_num = port_map_cnt;
	data->proto = proto;
	data->link_type = link_type;
	data->transfer_flag = transfer_flag;
	data->port = port;
	data->user_data = user_data;
	data->addr.s_addr = ip.s_addr;
	data->server_ip = server_ip;
	data->server_port = server_port;
	pthread_mutex_lock(&global_create_link.count_mutex);
	data->link_id = global_create_link.create_link_count++;
	PRINT("link = %d\n", data->link_id);
	pthread_mutex_unlock(&global_create_link.count_mutex);

	return data;
}

static int
tc_create_data_add(
	int proto,
	int link_type,
	int transfer_flag,
	int port_map_cnt,
	struct in_addr ip,
	struct in_addr server_ip,
	unsigned short server_port,
	unsigned short port
)
{
	unsigned long user_data;
	struct in_addr server_addr;
	struct tc_create_data *data = NULL;

	server_addr.s_addr = global_create_link.config.server_ip;
	data = tc_create_data_calloc(
				proto, 
				link_type, 
				transfer_flag, 
				port_map_cnt,
				ip, 
				server_ip,
				server_port,
				port, 
				user_data);
	if (!data) {
		return TC_ERR;
	}

	tc_create_user_data_get(data->link_id, &data->user_data);

	if (global_create_link.oper.prepare_data_get)
		global_create_link.oper.prepare_data_get(port_map_cnt, data->user_data);	



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
	int total_link = 0;
	int port_offset = 0;
	unsigned short end_port = 0;
	unsigned long user_data = 0;
	struct in_addr ip, server;
	struct tc_create_data *data = NULL;
	struct tc_create_config *config = NULL;

	config = &global_create_link.config;
	port_offset = config->port_map;
	count = config->end_port - config->start_port;
	PRINT("port_offset = %d, %d\n", port_offset, config->port_map);
	end_port = (count - count % port_offset) + config->start_port;
	count = 0;
	if (global_create_link.config.link_type == TC_DEV_SERVER)
		total_link = 1;
	else 
		total_link = global_create_link.config.total_link;

	//create user data array
	global_create_link.user_data.data_cnt = global_create_link.config.total_link;
	PRINT("data_cont = %d, total_link = %d\n", global_create_link.user_data.data_cnt, 
			global_create_link.config.total_link);
	global_create_link.user_data.data = (char*)calloc(
				global_create_link.user_data.data_cnt, 
				global_create_link.user_data.data_size);
	if (!global_create_link.user_data.data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	PRINT("data = %p, %p, cnt = %d\n", global_create_link.user_data.data, 
				&global_create_link.user_data.data[0], 
				global_create_link.user_data.data_cnt);

	ip.s_addr = config->start_ip;
	server.s_addr = global_create_link.config.server_ip;
	for (i = 0; i < config->ip_count; i++) {
		if (i != 0)
			ip.s_addr += 1 << 24;
		for (j = config->start_port; j < end_port; j++) {
			PRINT("count = %d, ip = %s, port = %d\n", count, inet_ntoa(ip), j);
			ret = tc_thread_test_exit();
			if (ret == TC_OK) {
				i = config->ip_count;
				j = config->end_port;
				continue;
			}
			if (count >= total_link) {
				i = config->ip_count;
				break;
			}
			count++;
			ret = tc_create_data_add(
					global_create_link.config.proto, 
					global_create_link.config.link_type, 
					0, 
					0, 
					ip, 
					server,
					global_create_link.config.server_port,
					j);
			if (ret != TC_OK) {
				i = config->ip_count;
				j = config->end_port;
				continue;
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
	thread_data = tc_create_data_calloc(0, 0, 0, 0, addr, addr, 0, 0, 0);
	if (!thread_data) 
		return TC_ERR;
	pthread_mutex_lock(&global_create_link.count_mutex);
	global_create_link.create_link_count--;
	pthread_mutex_unlock(&global_create_link.count_mutex);

	PRINT("create_link_id = %d, create_id = %d\n", global_create_link.create_link_id, global_create_link.create_id);
	return tc_thread_pool_node_add(global_create_link.create_link_id, &thread_data->node); 
} 

static void
tc_create_config_default_set(
	struct tc_create_config *config
)
{
	config->link_num = TC_THREAD_DEFAULT_NUM;
	config->stack_size = TC_THREAD_DEFALUT_STACK;
	config->total_link = 1;
	config->port_map = 1;
	config->end_port = 1;
	config->ip_count = 1;
}

static int
tc_create_config_setup()
{
	struct tc_create_config *config = NULL;
	config = &global_create_link.config;
	
	tc_create_config_default_set(config);

	TC_CONFIG_ADD("linger", &config->linger, FUNC_NAME(INT));
	TC_CONFIG_ADD("total_link", &config->total_link, FUNC_NAME(INT));
	TC_CONFIG_ADD("port_map", &config->port_map, FUNC_NAME(INT));
	TC_CONFIG_ADD("ip_count", &config->ip_count, FUNC_NAME(INT));
	TC_CONFIG_ADD("start_ip", &config->start_ip, FUNC_NAME(IP));
	TC_CONFIG_ADD("server_ip", &config->server_ip, FUNC_NAME(IP));
	TC_CONFIG_ADD("server_port", &config->server_port, FUNC_NAME(USHORT));
	TC_CONFIG_ADD("end_port", &config->end_port, FUNC_NAME(USHORT));
	TC_CONFIG_ADD("start_port", &config->start_port, FUNC_NAME(USHORT));
	TC_CONFIG_ADD("duration", &config->duration, FUNC_NAME(DURATION));
	TC_CONFIG_ADD("recv_timeout", &config->recv_timeout, FUNC_NAME(INT));
	TC_CONFIG_ADD("connect_timeout", &config->connect_timeout, FUNC_NAME(INT));
	TC_CONFIG_ADD("add_check", &config->add_check, FUNC_NAME(INT));
	TC_CONFIG_ADD("netcard", &config->netcard, FUNC_NAME(STR));
	TC_CONFIG_ADD("proto", &config->proto, FUNC_NAME(PROTO));
	TC_CONFIG_ADD("device", &config->link_type, FUNC_NAME(DEV));
	TC_CONFIG_ADD("stack_size", &config->stack_size, FUNC_NAME(INT));
	TC_CONFIG_ADD("link_thread_num", &config->link_num, FUNC_NAME(INT));
	TC_CONFIG_ADD("rendevous_enable", &config->rendevous_enable, FUNC_NAME(INT));
	TC_CONFIG_ADD("hub_interval", &config->hub_interval, FUNC_NAME(INT));
	TC_CONFIG_ADD("recv_buf", &config->recv_buf, FUNC_NAME(SIZE));
	TC_CONFIG_ADD("send_buf", &config->send_buf, FUNC_NAME(SIZE));
	TC_CONFIG_ADD(
		"trans_server_ip", 
		&config->transfer_server.addr, 
		FUNC_NAME(IP));
	TC_CONFIG_ADD(
		"trans_server_port", 
		&config->transfer_server.port, 
		FUNC_NAME(USHORT));
	TC_CONFIG_ADD(
		"trans_server_porto",
		&config->transfer_server.proto, 
		FUNC_NAME(PROTO));
	TC_CONFIG_ADD(
		"trans_server_path", 
		config->transfer_server.unix_path, 
		FUNC_NAME(STR));
	TC_CONFIG_ADD(
		"trans_client_path", 
		config->transfer_server.unix_path, 
		FUNC_NAME(STR));
	TC_CONFIG_ADD(
		"trans_client_ip", 
		&config->transfer_client.addr,
		FUNC_NAME(IP));
	TC_CONFIG_ADD(
		"trans_client_proto", 
		&config->transfer_client.proto,
		FUNC_NAME(PROTO));
	TC_CONFIG_ADD(
		"trans_client_port",
		&config->transfer_client.port,
		FUNC_NAME(USHORT));
}

int tc_create_init() 
{ 
	int ret = 0; 
	
	memset(&global_create_link, 0, sizeof(global_create_link));

	ret = tc_user_cmd_add(tc_create_config_setup);
	if (ret != TC_OK)
		return ret;

	ret = tc_init_register(tc_create_setup); 
	if (ret != TC_OK)
		return ret; 
	return tc_uninit_register(tc_create_uninit);
}

TC_MOD_INIT(tc_create_init);
