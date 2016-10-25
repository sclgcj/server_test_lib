#include "tc_param_file.h"
#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_init_private.h"
#include "tc_param_manage_private.h"
#include "tc_print.h"

#include <sys/mman.h>

static int
tc_param_file_oper(
	int oper_cmd, 
	struct tc_param *param
)
{
	return TC_OK;
}

static void
tc_param_file_set(
	struct tc_param *new_param,
	struct tc_param *old_param
)
{
	struct tc_param_file *new_file = NULL;
	struct tc_param_file *old_file = NULL;

	if (!new_param || !old_param)
		return;

	new_file = (struct tc_param_file *)new_param->data;
	old_file = (struct tc_param_file *)old_param->data;

	memset(old_file, 0, sizeof(*old_file));
	memcpy(old_file, new_file, sizeof(*old_file));
}

static void
tc_param_file_destroy(
	struct tc_param *param
)
{
	return;
}

static char *
tc_param_file_value_get(
	struct tc_create_link_data *cl_data,
	struct tc_param *param
)
{
	int start_line = 0, end_line = 0;
	char *start = NULL, *end = NULL, *cur = NULL;
	FILE *fp = NULL;
	struct tc_param_file *fparam = NULL;

	if (!param) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return NULL;
	}
	if (fparam->file_map == MAP_FAILED || 
			fparam->file_map == NULL) {
		TC_ERRNO_SET(TC_FILE_MAP_FAILED);
		return NULL;
	}

	cur = start = fparam->file_map;
	
	return NULL;
}

static int
tc_param_file_map_set(
	struct tc_param_file *param
)
{
	int fd = 0;
	struct stat sbuf;

	memset(&sbuf, 0, sizeof(sbuf));
	fd = open(param->file_path, O_RDONLY);
	if (fd == -1) {
		TC_ERRNO_SET(TC_OPEN_FILE_ERR);
		PRINT("open error: %s\n", strerror(errno));
		return TC_ERR;
	}
	fstat(fd, &sbuf);
	param->file_size = sbuf.st_size;
	param->file_map = (char*)mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (param->file_map == MAP_FAILED) {
		TC_ERRNO_SET(TC_FILE_MAP_FAILED);
		PRINT("mmap error: %s\n", strerror(errno));
		close(fd);
		return TC_ERR;
	}
out:
	close(fd);	
	return TC_OK;
}

static struct tc_param *
tc_param_file_copy(
	struct tc_param *param
)
{
	int ret = 0;
	struct tc_param *nparam = NULL;
	struct tc_param_file *param_file = NULL;
	struct tc_param_file *new_param = NULL;

	nparam = (struct tc_param *)calloc(1, sizeof(*param_file) + sizeof(*nparam));
	if (!nparam)
		TC_PANIC("Not enough memory for %d bytes\n", 
				sizeof(*param_file) + sizeof(*nparam));
	new_param = (struct tc_param_file *)param->data;
	param_file = (struct tc_param_file*)nparam->data;
	memcpy(param_file, new_param, sizeof(*new_param));

	ret = tc_param_file_map_set(new_param);
	if (ret != TC_OK) {
		TC_FREE(nparam);
		nparam = NULL;
	}

	return nparam;
}

static int
tc_param_file_setup()
{
	struct tc_param_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.param_oper = tc_param_file_oper;
	oper.param_value_get = tc_param_file_value_get;
	oper.param_copy = tc_param_file_copy;
	oper.param_destroy = tc_param_file_destroy;
	oper.param_set = tc_param_file_set;

	return tc_param_type_add("file", &oper);
}

int
tc_param_file_uninit()
{
}

int
tc_param_file_init()
{
	int ret = 0;	
	
	ret = tc_local_init_register(tc_param_file_setup);
	if (ret != TC_OK) 
		TC_PANIC("tc_local_init_register: register tc_param_file_setup error");

	return tc_local_uninit_register(tc_param_file_uninit);
}

TC_MOD_INIT(tc_param_file_init);
