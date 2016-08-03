
#include "tc_test.h"
#include "tc_config.h"
#include "tc_create.h"
#include "tc_hub.h"
#include "tc_interface.h"
#include "tc_json.h"

struct tc_test_config{
	int		duration;
	int		stack_size;
	int		link_type;	//server or client
	int		link_thread_num;
	int		recv_thread_num;
	int		handle_thread_num;
	int		timer_thread_num;
	int		send_thread_num;
	int		hub_thread_num;
	unsigned int    server_ip;
	unsigned short  server_port;
	unsigned short  res;
	struct tc_create_config create_config;
	/*int		linger;
	int		port_map;
	int		ip_count;	
	int		total_link;
	unsigned int	start_ip;
	unsigned short  end_port;
	unsigned short  start_port;*/
};

static struct tc_test_config global_config;
static unsigned long global_extra_data = 0;

static int
tc_cmd_d_handle(
	char *val,
	unsigned long user_data
)
{
	PRINT("user_data = %d\n", (int)user_data);
	daemon(1, 1);

	return TC_OK;
}

static int
tc_cmd_f_handle(
	char *val,
	unsigned long user_data 
)
{
	PRINT("user_data = %d\n", (int)user_data);
	PRINT("val = %s\n", val);

	return TC_OK;
}

static int
tc_cmd_haha_handle(
	char *val,
	unsigned long user_data
)
{
	PRINT("user_data = %d\n", (int)user_data);

	return TC_OK;
}

static int
tc_cmd_lalala_handle(
	char *val,
	unsigned long user_data
)
{
	PRINT("user_data = %d\n", (int)user_data);

	return TC_OK;
}

static int
tc_cmd_a_handle(
	char *val,
	unsigned long user_data
)
{
	PRINT("user_data = %d\n", (int)user_data);

	return TC_OK;
}

static int
tc_cmd_b_handle(
	char *val,
	unsigned long user_data
)
{
	PRINT("user_data = %d\n", (int)user_data);

	return TC_OK;
}

static int
tc_test_prepare_data_get(
	int port_map_cnt,
	unsigned long *data
)
{
	PRINT("-------------\n");

	return TC_OK;
}

static void
tc_test_create_flow_ctrl(
	int cur_count
)
{
	PRINT("cur_count = %d\n", cur_count);
}

static int
tc_test_connected_func(
	unsigned long	user_data,
	int		*event,
	struct sockaddr_in *addr
)
{
	PRINT("---\n");
	tc_hub_add(global_extra_data);

	return TC_OK;
}

static int
tc_test_create_link(
	int sock,
	struct in_addr addr,
	unsigned short port,
	unsigned long  data
)
{
	int ret = 0;

	return TC_OK;
}

static void
tc_test_err_handle(
	int reason,
	unsigned long user_data,
	struct tc_link_data *link_data
)
{
	PRINT(" %d, errllllll\n");
	link_data->err_flag = 1;
}

static int
tc_test_send_data(
	int sock,
	unsigned long user_data,
	struct sockaddr_in *addr
)
{
	PRINT("\n");

	return TC_OK;
}

static int
tc_test_recv_data(
	int sock,
	unsigned long user_data,
	struct sockaddr_in *addr
)
{
	PRINT("\n");

	return TC_OK;
}

static int
tc_test_handle_data(
	int sock,
	unsigned long user_data,
	struct tc_link_data *link_data,
	struct sockaddr_in *addr
)
{
	PRINT("\n");
	return TC_OK;
}

static int
tc_test_harbor(
	int sock,
	unsigned long user_data,
	struct sockaddr_in *peer_addr
)
{
	PRINT("HARBOR=========\n");

	return TC_OK;
}

static void
tc_test_extra_data_set(
	unsigned long extra_data, 
	unsigned long user_data
)
{
	PRINT("\n");
	global_extra_data = extra_data;
}

static int
tc_test_harbor_func(
	int sock,
	unsigned long user_data,
	struct sockaddr_in *peer_addr
)
{
	PRINT("\n");
	return TC_OK;
}

static void
tc_test_interface_before_send(
	unsigned long user_data
)
{
	PRINT("\n");
}

static int
tc_test_interface_recv(
	char *ptr,
	size_t size,
	size_t nmemb,
	unsigned long user_data
)
{
	PRINT("\n");
	return TC_OK;
}

static int
tc_test_link_create_setup()
{
	struct tc_create_link_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.connected_func = tc_test_connected_func;
	oper.prepare_data_get = tc_test_prepare_data_get;
	oper.create_link = tc_test_create_link;
	oper.create_flow_ctrl = tc_test_create_flow_ctrl;
	oper.err_handle = tc_test_err_handle;
	oper.send_data = tc_test_send_data;
	oper.recv_data = tc_test_recv_data;
	oper.handle_data = tc_test_handle_data;
	oper.harbor_func = tc_test_harbor;
	oper.extra_data_set = tc_test_extra_data_set;
	oper.harbor_func  = tc_test_harbor_func;
	oper.interface_before_send = tc_test_interface_before_send;
	oper.interface_recv	   = tc_test_interface_recv;

	return tc_link_create(&oper);
}

static void
tc_curopt_set(
	unsigned long user_data, 
	CURL *curl
)
{
	PRINT("\n");
}

static int
tc_write_callback(
	char *ptr, 
	size_t size,
	size_t nmemb,
	void *user_data
)
{
	int ret = 0;
	cJSON *param = NULL;
	char *tmp = NULL;

	PRINT("size = %d, ptr = %s\n", size * nmemb, ptr);
	ret = tc_interface_json_walk(
				"test_interface",
				"input_path", 
				(unsigned long*)&param,
				tc_json_to_json_param,
				NULL, 
				(unsigned long)user_data);
	if (ret != TC_OK){
		PRINT("hhhh: %s\n", TC_CUR_ERRMSG_GET());

		return ret;
	}

	PRINT("param = %s\n", cJSON_Print((cJSON*)param));
	ret = tc_interface_json_walk(
				"test_interface",
				"input_check",
				(unsigned long*)&param, 
				tc_json_node_check, 
				NULL, 
				(unsigned long)user_data);
	if (ret != TC_OK) {
		PRINT("EEEEEE : %s\n", TC_CUR_ERRMSG_GET());
		return ret;
	}


	return size * nmemb;
}

static int
tc_test_interface(
	unsigned long data	
)
{
	int ret = 0;
	int encode_size = 128; 
	char encode_str[128] = { 0 };
	char *param = "hahahahhahahah";

	tc_interface_url_encode(param, &encode_size, encode_str);
	PRINT("\n");

	ret = tc_mobile_data_send(
				"http://192.168.30.227:10004/Interfaces/AppInterface/test", 
				encode_str,	
				strlen(encode_str),
				global_extra_data, 
				tc_curopt_set, 
				tc_write_callback);
	if (ret != TC_OK)
		return ret;
}

static int
tc_test_interface_setup()
{
//	int ret = 0;
	
	tc_interface_register("test_interface", tc_test_interface);
	return TC_OK;
}

static int
tc_test_setup()
{
	tc_err_add(TC_TEST_HARD_ERR, "hard err");
	tc_err_add(TC_TEST_SOFT_ERR, "soft err");

	tc_test_link_create_setup();

	PRINT("err = %d, %s\n", TC_TEST_HARD_ERR, tc_errmsg_get(TC_TEST_HARD_ERR));
	PRINT("err2 = %d, %s\n", TC_TEST_SOFT_ERR, tc_errmsg_get(TC_TEST_SOFT_ERR));
	return TC_OK;
}

static int
tc_test_cmd_add()
{
	tc_cmd_add("d", TC_CMD_SHORT, tc_cmd_d_handle, 1);
	tc_cmd_add("f", TC_CMD_SHORT_PARAM, tc_cmd_f_handle, 2);
	tc_cmd_add("haha", TC_CMD_LONG, tc_cmd_haha_handle, 3);
	tc_cmd_add("lalala", TC_CMD_LONG_PARAM, tc_cmd_lalala_handle, 4);
	tc_cmd_add("a", TC_CMD_SHORT, tc_cmd_a_handle, 5);
	tc_cmd_add("b", TC_CMD_SHORT, tc_cmd_b_handle, 6);

	return TC_OK;
}

static int
tc_test_uninit()
{
	PRINT("\n");

	return TC_OK;
}

static int
tc_test_get_config_file(
	int file_path_len,
	char *file_path,
	int *real_len
)
{
	char *file = "./hello.toml";
	PRINT("\n");

	memcpy(file_path, file, strlen(file));
	return TC_OK;
}

static void
tc_test_config_start()
{
	PRINT("\n");
}

static void
tc_test_config_end()
{
	PRINT("\n");
}

static struct tc_config_oper global_test_conf_oper = 
{
	.get_config_file = tc_test_get_config_file,
	.config_start	 = tc_test_config_start,
	.config_end	 = tc_test_config_end
};

static void
tc_test_config_a(
	int toml_type,
	char *name,
	char *val,
	unsigned long user_data
)
{
	PRINT("toml_type = %d\n", toml_type);
	PRINT("name = %s\n", name);
	PRINT("val = %s\n", val);
	PRINT("user_data = %ld\n", user_data);
}

/*#define TC_CONFIG_ADD(name, vp, func) \
	tc_config_add(name, TC_CONFIG_TOML_NORMAL, (unsigned long)vp, func)

#define FUNC_NAME(func) tc_test_config_##func

#define CONFIG_FUNC(func) \
	 void FUNC_NAME(func)( \
			int toml_type,  \
			char *name, \
			char *val, \
			unsigned long user_data)

CONFIG_FUNC(INT)
{
	*((int*)user_data) = atoi(val);
	PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(STR)
{
	memcpy((char*)user_data, val, strlen(val));
	PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(SHORT)
{
	*((short*)user_data) = (short)atoi(val);
	PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(USHORT)
{
	*((unsigned short*)user_data) = (unsigned short)atoi(val);
	PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(IP)
{
	*((unsigned int*)user_data) = inet_addr(val);
	PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(TABLE)
{
	PRINT("table\n");
}

CONFIG_FUNC(DURATION)
{
	int day = 0, hour = 0, min = 0, sec = 0;

	sscanf(val, "%d:%d:%d:%d", &day, &hour, &min, &sec);
	if (hour >=24 || min >= 60 || sec >= 60) 
		PRINT("Wrong duration time set : %s\n", val);

	*((int*)user_data) = day * 3600 * 24 + hour * 3600 + min * 60 + sec;
	PRINT("duration = %d\n", *((int*)user_data));
}

CONFIG_FUNC(PROTO)
{
	if (!strcmp(val, "tcp")) 
		*((int*)user_data) = TC_PROTO_TCP;
	else if (!strcmp(val, "udp"))
		*((int*)user_data) = TC_PROTO_UDP;
	else if (!strcmp(val, "http"))
		*((int*)user_data) = TC_PROTO_HTTP;
	else if (!strcmp(val, "unix_tcp"))
		*((int*)user_data) = TC_PROTO_UNIX_TCP;
	else if (!strcmp(val, "unix_udp"))
		*((int*)user_data) = TC_PROTO_UNIX_UDP;
}

CONFIG_FUNC(DEV)
{
	if (!strcmp(val, "server"))
		*((int*)user_data) = TC_DEV_SERVER;
	if (!strcmp(val, "client"))
		*((int*)user_data) = TC_DEV_CLIENT;
}*/

/*
struct tc_test_config{
	int		linger;
	int		port_map;
	int		ip_count;	
	int		total_link;
	unsigned int	start_ip;
	unsigned int    server_ip;
	unsigned short  server_port;
	unsigned short  res;
	unsigned short  end_port;
	unsigned short  start_port;
};

static struct tc_test_config global_config;
*/

static int
tc_test_config_setup()
{
	tc_config_oper_register(&global_test_conf_oper);

	return TC_OK;
}

int
tc_test_init()
{
	int ret = 0;

	ret = tc_user_cmd_add(tc_test_cmd_add);
	if (ret != TC_OK)
		TC_PANIC("err_msg = %s\n", TC_CUR_ERRMSG_GET());

	ret = tc_user_cmd_add(tc_test_interface_setup);
	if (ret != TC_OK)
		TC_PANIC("cmd_add =%s\n", TC_CUR_ERRMSG_GET());

	ret = tc_user_cmd_add(tc_test_config_setup);
	if (ret != TC_OK)
		TC_PANIC("add config setup: %s\n", TC_CUR_ERRMSG_GET());

	ret = tc_init_register(tc_test_setup);
	if (ret != TC_OK)
		TC_PANIC("err_msg = %s\n", TC_CUR_ERRMSG_GET());

	ret = tc_uninit_register(tc_test_uninit);
	if (ret != TC_OK) {
		TC_PANIC("unint_register = %s\n", TC_CUR_ERRMSG_GET());
		return ret;
	}

	return TC_OK;
}

TC_MOD_INIT(tc_test_init);

