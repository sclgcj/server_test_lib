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
	char *unix_path,
	int link_linger,
	struct in_addr addr,
	unsigned short port,
	int *sock
)
{
	int addr_size = 0;
	struct sockaddr_in inet_addr;
	struct sockaddr_un unix_addr;
	struct sockaddr *bind_addr;

	if (proto == TC_PROTO_TCP)
		(*sock) = socket(AF_INET, SOCK_STREAM, 0);
	else if (proto == TC_PROTO_UDP)
		(*sock) = socket(AF_INET, SOCK_DGRAM, 0);
	else if (proto == TC_PROTO_UNIX_TCP)
		(*sock) = socket(AF_UNIX, SOCK_STREAM, 0);
	else if (proto == TC_PROTO_UNIX_UDP)
		(*sock) = socket(AF_UNIX, SOCK_DGRAM, 0);
	else 
		return TC_OK;
	if (*sock < 0) 
		TC_PANIC("create socket erro: %s\n", strerror(errno));

	tc_set_socket_opt(*sock, link_linger);

	if (proto == TC_PROTO_TCP || proto == TC_PROTO_UDP) {
		memset(&inet_addr, 0, sizeof(inet_addr));
		inet_addr.sin_family = AF_INET;
		inet_addr.sin_port = htons(port);
		inet_addr.sin_addr.s_addr = addr.s_addr;
		bind_addr = (struct sockaddr*)&inet_addr;
		addr_size = sizeof(struct sockaddr_in);
	} else {
		memset(&unix_addr, 0, sizeof(unix_addr));
		unix_addr.sun_family = AF_UNIX;
		memcpy(unix_addr.sun_path, unix_path, strlen(unix_path));
		bind_addr = (struct sockaddr*)&unix_addr;
		addr_size = sizeof(struct sockaddr_un);
	}

	if (bind((*sock), bind_addr, addr_size) < 0) {
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

	PRINT("sever_ip = %s, server_port = %d\n", inet_ntoa(server_ip), server_port);
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
			return TC_OK;	
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

int
tc_unix_tcp_connect(
	int	sock,
	char	*path
)
{
	return TC_OK;
}

int
tc_unix_tcp_accept(
	int	sock
)
{
	return TC_OK;
}

int
tc_unix_udp_connect(
	int	sock,
	char	*path
)
{
	return TC_OK;
}

int
tc_unix_udp_accept(
	int	sock
)
{
	return TC_OK;
}
