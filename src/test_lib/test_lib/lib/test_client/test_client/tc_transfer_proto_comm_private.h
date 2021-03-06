#ifndef TC_TRANSFER_PROTO_COMM_PRIVATE_H
#define TC_TRANSFER_PROTO_COMM_PRIVATE_H

struct tc_address;
struct tc_create_link_data;
int
tc_transfer_proto_comm_accept(
	struct tc_create_link_data *cl_data
);

int	
tc_transfer_proto_comm_data_send(
	struct tc_create_link_data *cl_data
);

int
tc_transfer_proto_comm_data_recv(
	struct tc_create_link_data *cl_data
);

int
tc_transfer_proto_comm_data_handle(
	struct tc_create_link_data *cl_data
);

int
tc_transfer_proto_accept_handle(
	struct tc_create_link_data *cl_data
);

int
tc_transfer_proto_comm_udp_data_recv(
	struct tc_create_link_data *cl_data
);

int	
tc_transfer_proto_comm_udp_data_send(
	struct tc_create_link_data *cl_data
);

int
tc_transfer_proto_server();

int
tc_transfer_proto_client();

#endif
