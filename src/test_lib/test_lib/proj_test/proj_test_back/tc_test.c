
#include "tc_test.h"
#include "tc_config.h"

static int
tc_cmd_d_handle(
	char *val,
	unsigned long user_data
)
{
	PRINT("user_data = %d\n", (int)user_data);

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
tc_test_setup()
{
	tc_err_add(TC_TEST_HARD_ERR, "hard err");
	tc_err_add(TC_TEST_SOFT_ERR, "soft err");

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
	.config_end		 = tc_test_config_end
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

static int
tc_test_config_setup()
{
	tc_config_oper_register(&global_test_conf_oper);

	tc_config_add("hello", TC_CONFIG_TOML_NORMAL, 0, tc_test_config_a);
	tc_config_add("bye", TC_CONFIG_TOML_NORMAL, 0, tc_test_config_a);
	tc_config_add("interface", TC_CONFIG_TOML_TABLE, 0, tc_test_config_a);
	tc_config_add("dada", TC_CONFIG_TOML_NORMAL, 0, tc_test_config_a);
}

int
tc_test_init()
{
	int ret = 0;

	ret = tc_user_cmd_add(tc_test_cmd_add);
	if (ret != TC_OK)
		TC_PANIC("err_msg = %s\n", TC_CUR_ERRMSG_GET());

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

