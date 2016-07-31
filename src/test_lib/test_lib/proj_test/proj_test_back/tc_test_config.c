#include "tc_test.h"


static cJSON *global_param = NULL;


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


int
tc_test_config_setup()
{
	tc_config_oper_register(&global_test_conf_oper);

	tc_config_add("hello", TC_CONFIG_TOML_NORMAL, 0, tc_test_config_a);
	tc_config_add("bye", TC_CONFIG_TOML_NORMAL, 0, tc_test_config_a);
	tc_config_add("interface", TC_CONFIG_TOML_TABLE, 0, tc_test_config_a);
	tc_config_add("dada", TC_CONFIG_TOML_NORMAL, 0, tc_test_config_a);

}
