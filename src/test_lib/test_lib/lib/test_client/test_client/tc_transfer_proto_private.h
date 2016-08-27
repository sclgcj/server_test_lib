#ifndef TC_TRANSFER_PROTO_H
#define TC_TRANSFER_PROTO_H 1

/*
 * We want our transfer layer to be more flexible : no matter how 
 * we change our special protocol,there will no be any effect to 
 * other module. We regard accept as recv, so we can just use same
 * interface to dispose server and client
 */
struct tc_create_link_data;
struct tc_transfer_proto_oper {
	int (*proto_connect)(struct tc_create_link_data *cl_data);
	int (*proto_recv)(struct tc_create_link_data *cl_data);
	int (*proto_send)(struct tc_create_link_data *cl_data);
	int (*proto_handle)(struct tc_create_link_data *cl_data);
	int (*proto_event_add)(struct tc_create_link_data *cl_data);
	int (*proto_recv_check_start)(struct tc_create_link_data *cl_data);
	int (*is_proto_server)();
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

struct tc_transfer_proto_oper*
tc_transfer_proto_oper_get_by_name(
	char *proto_name
);

struct tc_transfer_proto_oper*
tc_transfer_proto_oper_get_by_type(
	int proto_type
);

struct tc_transfer_proto_oper*
tc_transfer_proto_oper_get();

int
tc_transfer_proto_add(
	struct tc_transfer_proto_oper *oper,
	int			      *proto_id
);

int
tc_transfer_proto_config_add(
	char *name,
	int  proto
);

#endif
