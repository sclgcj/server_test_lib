#include "tc_socket_private.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_config.h"


int
tc_nonblock_fd_set(
	int fd
)
{
	int flag = 0;

	flag = fcntl(fd, F_GETFL);
	if (flag < 0) 
		TC_PANIC("fcntl F_GETFL error:%s\n", strerror(errno));
	flag |= O_NONBLOCK;
	flag = fcntl(fd, F_SETFL, flag);
	if (flag < 0)
		TC_PANIC("fcntl F_SETFL error:%s\n", strerror(errno));

	return TC_OK;
}


static int
tc_set_socket_opt(
	int sock,
	int link_linger
)
{
	int reuse = 1;	
	struct linger linger;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int)) < 0) 
		TC_PANIC("set socket SO_REUSEADDR error :%s\n", strerror(errno));

	if (link_linger) {
		linger.l_onoff = 1;
		linger.l_linger = 0;
		if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void*)&linger, sizeof(linger)) < 0)
			TC_PANIC("set socket SO_LINGER error: %s\n", strerror(errno));
	}
	tc_nonblock_fd_set(sock);

	return TC_OK;
}

int
tc_create_socket(
	int proto,
	int link_linger,
	struct in_addr addr,
	unsigned short port,
	int *sock
)
{
	struct sockaddr_in bind_addr;

	if (proto == TC_PROTO_TCP)
		(*sock) = socket(AF_INET, SOCK_STREAM, 0);
	else
		(*sock) = socket(AF_INET, SOCK_DGRAM, 0);
	if (*sock < 0) 
		TC_PANIC("create socket erro: %s\n", strerror(errno));

	tc_set_socket_opt(*sock, link_linger);

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_addr.s_addr = addr.s_addr;
	bind_addr.sin_port = htons(port);
	bind_addr.sin_family = AF_INET;
	if (bind((*sock), (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
		if (errno == EADDRINUSE) {
			TC_ERRNO_SET(TC_ADDR_ALREADY_INUSE);
			return TC_ERR;
		}
		TC_PANIC("bind error: %s\n", strerror(errno));
	}

	return TC_OK;
}

int
tc_tcp_connect(
	int		sock,
	struct in_addr	server_ip,
	unsigned short	server_port
)
{
	int ret = 0;
	int addr_size = sizeof(struct sockaddr_in);
	struct sockaddr_in server_addr;
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family	= AF_INET;
	server_addr.sin_port	= htons(server_port);
	server_addr.sin_addr	= server_ip;	

	while (1) {
		ret = connect(sock, (struct sockaddr*)&server_addr, addr_size);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			else if (errno == EISCONN)
				return TC_OK;
			else if (errno != EINPROGRESS && 
					errno != EALREADY && errno != EWOULDBLOCK) 
				TC_PANIC("connect error :%s\n", strerror(errno));
			return TC_WOULDBLOCK;	
		}
		break;
	}

	return TC_OK;
}

int
tc_tcp_accept(
	int sock
)
{
	return TC_OK;
}

int
tc_udp_connect(
	int sock,	
	struct in_addr addr,
	unsigned short port
)
{
	return TC_OK;
}

int
tc_udp_accept(
	int sock
)
{
	return TC_OK;
}
