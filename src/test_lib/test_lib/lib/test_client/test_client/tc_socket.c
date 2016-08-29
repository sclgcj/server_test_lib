#include "tc_socket_private.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_config.h"

int
tc_block_fd_set(
	int fd
)
{
	int flag = 0;

	flag = fcntl(fd, F_GETFL);
	if (flag < 0) 
		return TC_ERR;
	flag &= ~O_NONBLOCK;
	flag = fcntl(fd, F_SETFL, flag);
	if (flag < 0)
		return TC_ERR;

	return TC_OK;
}

int
tc_nonblock_fd_set(
	int fd
)
{
	int flag = 0;

	flag = fcntl(fd, F_GETFL);
	if (flag < 0) 
		return TC_ERR;
	flag |= O_NONBLOCK;
	flag = fcntl(fd, F_SETFL, flag);
	if (flag < 0)
		return TC_ERR;

	return TC_OK;
}

static int
tc_set_socket_opt(
	int sock,
	struct tc_create_socket_option *option
)
{
	int reuse = 1;	
	int real_buf = 0;
	int def_buf = 4 * 1024;
	struct linger linger;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int)) < 0) 
		TC_PANIC("set socket SO_REUSEADDR error :%s\n", strerror(errno));

	if (option->linger) {
		linger.l_onoff = 1;
		linger.l_linger = 0;
		if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void*)&linger, sizeof(linger)) < 0)
			TC_PANIC("set socket SO_LINGER error: %s\n", strerror(errno));
	}
	if (option->recv_buf > def_buf) {
		real_buf = option->recv_buf;
		if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&real_buf, sizeof(int)) < 0)
			TC_PANIC("set socket SO_RCVBUF error: %s\n", strerror(errno));
	}
	if (option->send_buf > def_buf) {
		real_buf = option->send_buf;
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&real_buf, sizeof(int)) < 0)
			TC_PANIC("set socket SO_SNDBUF error: %s\n", strerror(errno));
	}
	tc_nonblock_fd_set(sock);

	return TC_OK;
}

int
tc_create_socket(
	int proto,
	char *unix_path,
	struct in_addr addr,
	unsigned short port,
	struct tc_create_socket_option *option,
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

	PRINT("sock = %d\n", sock);
	tc_set_socket_opt(*sock, option);

	PRINT("port = %d, addr = %s\n", port, inet_ntoa(addr));
	if (proto == TC_PROTO_TCP || proto == TC_PROTO_UDP) {
		memset(&inet_addr, 0, sizeof(inet_addr));
		inet_addr.sin_family = AF_INET;
		inet_addr.sin_port = htons(port);
		inet_addr.sin_addr.s_addr = addr.s_addr;
		bind_addr = (struct sockaddr*)&inet_addr;
		addr_size = sizeof(struct sockaddr_in);
	} else if (unix_path){
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

