#include "http_client.h"
#include "hc_init_private.h"
#include "hc_config_private.h"
#include "hc_interface_private.h"

/*用户处理结构*/
struct hc_data {
	struct hc_oper oper;
	struct hc_config config;
};

static struct hc_data global_hc_data;

static int
hc_prepare_data_get(
	int port_map_cnt,
	unsigned long data
)
{
	struct hc_create_link_data *cl_data = NULL;

	if (!data)
		return TC_ERR;

	cl_data = (struct hc_create_link_data *)data;
	cl_data->first_recv = 0;
	cl_data->circle_cnt = 0;
	pthread_cond_init(&cl_data->interface_cond, NULL);
	pthread_mutex_init(&cl_data->interface_mutex, NULL);
	pthread_mutex_init(&cl_data->data_mutex, NULL);
	cl_data->oper = &global_hc_data.oper;

	if (cl_data->oper->prepare_data_get)
		cl_data->oper->prepare_data_get(port_map_cnt, 
					(unsigned long)cl_data->data);
        
	return TC_OK;
}
                                                                               
static int
hc_err_handle(
	int reason,
	unsigned long user_data
)
{
	int ret = 0;
	struct hc_create_link_data *cl_data = NULL;

	cl_data = (struct hc_create_link_data*)user_data;
	if (cl_data->oper && cl_data->oper->err_handle)
		ret = cl_data->oper->err_handle(reason, 
				(unsigned long)cl_data->data);
	return ret;
}

static int
tc_create_link(
	unsigned long user_data
)
{
	int ret = 0;
	struct hc_create_link_data *cl_data = NULL;

	cl_data = (struct hc_create_link_data *)user_data;

	return hc_interface_func_execute(cl_data);
}

static int
hc_config_setup()
{
	cJSON *obj = NULL;

	// 获取通用配置
	obj = tc_config_read_get("general");
	CR_INT(obj, "total_link", global_hc_data.config.total_link);
	CR_INT(obj, "recv_timeout", global_hc_data.config.recv_timeout);
	CR_INT(obj, "connect_timeout", global_hc_data.config.conn_timeout);

	obj = tc_config_read_get("http_client");
	CR_INT(obj, "rendevous_enable", global_hc_data.config.rendevous_enable);
	CR_INT(obj, "circle_run", global_hc_data.config.circle_run);
	CR_USHORT(obj, "http_server_port", global_hc_data.config.http_server_port);
	CR_STR(obj, "http_server_ip", global_hc_data.config.http_server_ip);

	hc_interface_init(&global_hc_data.config);

	hc_init();

	return TC_OK;
}

int
hc_start(
	int user_data_size,
	struct hc_oper *oper
)
{
	struct tc_create_link_oper cl_oper;

	memcpy(&global_hc_data.oper, oper, sizeof(*oper));

	memset(&cl_oper, 0, sizeof(cl_oper));
	cl_oper.prepare_data_get = hc_prepare_data_get;
	cl_oper.err_handle = hc_err_handle;
	
	return tc_link_create_start(
			"http_client",
			sizeof(struct hc_create_link_data) + user_data_size, 
			&cl_oper);
}

static int
hc_destroy()
{
	/*当程序结束时，处理该模块的一些数据*/
	hc_interface_destroy();
	
	hc_uninit();

	return TC_OK;
}


/*
 * 该模块最开始运行的程序
 */
int
hc_private_init()
{
	int ret = 0;

/*	ret = tc_user_cmd_add(hc_config_oper_set);
	if (ret != TC_OK)
		TC_PANIC("add config setup: %s\n", TC_CUR_ERRMSG_GET());*/

	ret = tc_init_register(hc_config_setup);
	if (ret != TC_OK)
		TC_PANIC("err_msg = %s\n", TC_CUR_ERRMSG_GET());

	ret = tc_uninit_register(hc_destroy);
	if (ret != TC_OK) {
		TC_PANIC("unint_register = %s\n", TC_CUR_ERRMSG_GET());
		return ret;
	}

	return TC_OK;
}

/*
 * 声明该模块在main函数启动之前运行
 */
TC_MOD_INIT(hc_private_init);

