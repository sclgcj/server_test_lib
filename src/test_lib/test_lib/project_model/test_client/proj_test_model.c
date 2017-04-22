
/*
 * 这是一个模板程序，包含了所有库所需要的回调函数，
 * 同时增加了一些我们在使用过程中用到的函数分类，
 * 可以更加方便快速的找到对应的地方进行修改。为了
 * 方便我们使用tc_text为前缀，如果想修改，可以在
 * vim 的命令行模式下使用如下命令进行修改:
 * :1,$s/tc_test/想要更改的名字/g 
 * 即可.
 */
/*用户处理结构*/
struct test_structure {
	int test_data;
};

static int
tc_test_prepare_data_get(
	int port_map_cnt,
	unsigned long data
)
{
	/* 创建连接前准备连接数据 */
	return TC_OK;
}

static void
tc_test_create_flow_ctrl(
	int cur_count
)
{
	/* 连接创建速度控制 */
}

static int
tc_test_connected_func(
	unsigned long	user_data
)
{
	/* tcp连接建立成功或者udp开始客户端发送第一个数据时调用该函数 */
	return TC_OK;
}

static int
tc_test_err_handle(
	int reason,
	unsigned long user_data
)
{
	/* 当出现错误时调用的函数 */
}

static int
tc_test_send_data(
	unsigned long user_data
)
{
	/* 发送数据时调用的函数 */
	return TC_OK;
}

static int
tc_test_recv_data(
	char *ptr,
	int  size,
	unsigned long user_data
)
{
	/* 接收数据时调用该函数 */
	return TC_OK;
}

static int
tc_test_handle_data(
	unsigned long user_data
)
{
	/* 处理数据时，调用该函数 */
	return TC_OK;
}

static int
tc_test_harbor_func(
	unsigned long user_data
)
{
	/* 在需要发送心跳包的时候， 发送心跳的时候调用该函数 */
	return TC_OK;
}

static void
tc_test_interface_before_send(
	unsigned long user_data
)
{
	/* api接口发送之前做的处理 */
}

static int
tc_test_interface_recv(
	char *ptr,
	size_t size,
	size_t nmemb,
	unsigned long user_data
)
{
	/* 接收到api返回数据时做的处理 */
	return TC_OK;
}

/*int
tc_test_tcp_accept(
)*/

static int
tc_test_link_create_setup()
{
	struct tc_create_link_oper oper;

	/* 调用tc_link_create方法，为创建连接设置回调函数 */

	memset(&oper, 0, sizeof(oper));
	oper.connected_func = tc_test_connected_func;
	oper.prepare_data_get = tc_test_prepare_data_get;
	oper.create_flow_ctrl = tc_test_create_flow_ctrl;
	oper.err_handle = tc_test_err_handle;
	oper.recv_data = tc_test_recv_data;
	oper.handle_data = tc_test_handle_data;
	oper.harbor_func  = tc_test_harbor_func;

	return tc_link_create_start("test", sizeof(struct test_structure), &oper);
}

static int
tc_test_interface_setup()
{
	/*注册api接口*/
	return TC_OK;
}

static int
tc_test_setup()
{
	/*初始化数据*/
	return TC_OK;
}

static int
tc_test_cmd_add()
{
	/*注册命令行参数*/
	return TC_OK;
}

static int
tc_test_config_setup()
{
	/*初始化配置*/
	return TC_OK;
}

static int
tc_test_uninit()
{
	/*当程序结束时，处理该模块的一些数据*/
	return TC_OK;
}

/*
 * 该模块最开始运行的程序
 */
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

/*
 * 声明该模块在main函数启动之前运行
 */
TC_MOD_INIT(tc_test_init);

