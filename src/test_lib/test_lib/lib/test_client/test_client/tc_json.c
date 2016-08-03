#include "tc_json.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_interface_private.h"

static int
tc_walk_json(
	int depth,
	cJSON *root,
	unsigned long out_data,
	node_handle_func node_handle,
	json_node_handle_func json_node_handle,
	unsigned long user_data
);

static int
tc_walk_object(
	int depth,
	cJSON *root,
	unsigned long out_data,
	node_handle_func node_handle,
	json_node_handle_func json_node_handle,
	unsigned long user_data
);

int 
tc_json_to_param(
	int depth,
	cJSON *input_node,
	unsigned long *out,
	json_node_handle_func json_node_handle,
	unsigned long user_data
)
{
	char val_pair[256] = { 0 };
	char *param = (char*)(*out);

	if (!input_node || !out) 
		return TC_ERR;

	if (input_node->type == cJSON_NULL)
		return TC_OK;

	if (json_node_handle) 
		json_node_handle(input_node, (unsigned long)param, user_data);
	else {
		sprintf(val_pair, "&%s=%s", input_node->string, input_node->valuestring);
		strcat(param, val_pair);
	}

	return TC_OK;
}

int
tc_json_node_default_param(
	cJSON *input_node,
	unsigned long out_data,
	unsigned long user_data
)
{
	cJSON *out = (cJSON *)out_data;
	cJSON *node = NULL;

	switch (input_node->type) {
	case cJSON_True:
	case cJSON_False:
	case cJSON_Number:
		if (input_node->valuedouble)
			node = cJSON_CreateNumber(input_node->valuedouble);
		else 
			node = cJSON_CreateNumber(input_node->valueint);
		break;
	case cJSON_String:
		node = cJSON_CreateString(input_node->valuestring);
		break;
	default:
		return TC_OK;
	}
	if (node) {
		if (out->type == cJSON_Object)
			cJSON_AddItemToObject(out, input_node->string, node);
		else if (out->type == cJSON_Array)
			cJSON_AddItemToArray(out, node);
	}

	return TC_OK;
}

int
tc_json_to_json_param(
	int depth,
	cJSON *input_node,
	unsigned long *out_data,
	json_node_handle_func json_node_handle,
	unsigned long user_data
)
{
	int ret = TC_OK;
	cJSON *node = NULL;
	cJSON *data = (cJSON*)(*out_data);

	switch(input_node->type) {
	case cJSON_NULL:
		break;
	case cJSON_Array:
		node = cJSON_CreateArray();
		if (!node) {
			TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
			return TC_ERR;
		}
		ret = tc_walk_object(
				0,
				input_node, 
				(unsigned long)node, 
				tc_json_to_json_param, 
				json_node_handle, 
				user_data);
		if (ret == TC_OK) {
			if (data->type == cJSON_Array)
				cJSON_AddItemToArray(data, node);
			else 
				cJSON_AddItemToObject(data, input_node->string, node);
		}
		break;
	case cJSON_Object:
		node = cJSON_CreateObject();
		if (!node) {
			TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
			return TC_ERR;
		}
		ret = tc_walk_object(
				0, 
				input_node, 
				(unsigned long)node,
				tc_json_to_json_param,
				json_node_handle,
				user_data);
		if (ret == TC_OK) {
			if (data->type == cJSON_Array)
				cJSON_AddItemToArray(data, node);
			else
				cJSON_AddItemToObject(data, input_node->string, node);
		}
		break;
	default:
		if (json_node_handle) 
			ret = json_node_handle(input_node, (*out_data), user_data);
		else
			ret = tc_json_node_default_param(
						input_node,
						(*out_data),
						user_data);
	}
out:
	return ret;
}

static int
tc_json_array_check(
	int depth,
	cJSON *input,
	cJSON *resp,
	json_node_handle_func json_node_handle,
	unsigned long user_data
)
{
	int i = 0;
	int ret = 0;
	int array_size = 0;
	unsigned long resp_node = 0;
	cJSON *input_node = NULL;

	array_size = cJSON_GetArraySize(resp);
	for (; i < array_size; i++) {
		resp_node = (unsigned long)cJSON_GetArrayItem(resp, i);
		input_node = cJSON_GetArrayItem(input, i);
		ret = tc_walk_json(
				1, 
				input_node, 
				resp_node, 
				tc_json_node_check, 
				json_node_handle, 
				user_data);
		if (ret != TC_OK)
			return ret;
	}

	return TC_OK;
}

static int
tc_json_check(
	cJSON *input,
	cJSON *req,
	unsigned long data
)
{
	switch (req->type & 255) {
	case cJSON_String:
		if (!strcmp(input->valuestring, req->valuestring)) 
			return TC_OK;
		else 
			PRINT("%s value is not the same, req = %s, input = %s\n", 
					input->string, req->valuestring, input->valuestring);
		break;
	case cJSON_Number:
	case cJSON_False:
	case cJSON_True:
		if (req->valueint == atoi(input->valuestring) ||
			(int)req->valuedouble == atoi(input->valuestring))
			return TC_OK;
		else 
			PRINT("%s value is not the same, req = %d,%f, input = %s\n", 
					input->string, req->valuestring, 
					req->valuedouble, input->valuestring);
		break;
	default:
		break;
	}

	TC_ERRNO_SET(TC_WRONG_JSON_DATA);
	return TC_ERR;
}

int
tc_json_node_check(
	int depth,
	cJSON *input_data,
	unsigned long *param,
	json_node_handle_func json_node_handle,
	unsigned long user_data
)
{
	int ret = TC_OK;
	cJSON *root = (cJSON *)(*param);
	cJSON *node = NULL;

	if (depth == 0 || input_data->string == NULL)
		node = root;
	else {
		if (root->type != cJSON_Object)
			node = (cJSON*)(*param);
		else {
			node = cJSON_GetObjectItem(root, input_data->string);
			if (!node) {
				TC_ERRNO_SET(TC_WRONG_JSON_DATA);
				return TC_ERR;
			}
		}
	}
	if (json_node_handle) {
		ret = json_node_handle(input_data, (unsigned long)node, user_data);
		if (ret != TC_OK)
			return ret;
	}
	if (input_data->type == cJSON_NULL)
		return TC_OK;
	switch (input_data->type & 255) {
	case cJSON_Array:
		if (input_data->type != node->type) {
			TC_ERRNO_SET(TC_WRONG_JSON_DATA);
			return TC_ERR;
		}
		ret = tc_json_array_check(
					depth, 
					input_data, 
					node, 
					json_node_handle, 
					user_data);
		break;
	case cJSON_Object:
		if (input_data->type != node->type) {
			TC_ERRNO_SET(TC_WRONG_JSON_DATA);
			return TC_ERR;
		}
		ret = tc_walk_object(
				0, 
				input_data, 
				(unsigned long)node, 
				tc_json_node_check, 
				json_node_handle, 
				user_data);
		break;
	default:
		ret = tc_json_check(input_data, node, user_data);
	}

	return ret;
}

static int
tc_walk_object(
	int depth,
	cJSON *root,
	unsigned long out_data,
	node_handle_func node_handle,
	json_node_handle_func json_node_handle,
	unsigned long user_data
)
{
	int ret = 0;
	int numentries = 0;
	cJSON *child = root->child;

	while (child) numentries++, child = child->next;

	if (!numentries)
		return TC_OK;
	child = root->child;
	depth++;
	while (child) {
		ret = tc_walk_json(
				depth, 
				child, 
				out_data, 
				node_handle, 
				json_node_handle, 
				user_data);
		if (ret != TC_OK)
			return ret;

		child = child->next;
	}

	return TC_OK;
}

static int 
tc_walk_json(
	int depth,
	cJSON *root,
	unsigned long out_data,
	node_handle_func node_handle,
	json_node_handle_func json_node_handle,
	unsigned long user_data
)
{
	int ret = 0;
	unsigned long cur_data = out_data;

	if (!root) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	switch (root->type & 255) {
	case cJSON_Array:
		if (node_handle) 
			ret = node_handle(depth, root, &cur_data, json_node_handle, user_data);
		break;
	case cJSON_Object:
		if (node_handle && depth != 0) 
			ret = node_handle(depth, root, &cur_data, json_node_handle, user_data);
		else if (node_handle && depth == 0)
			ret = tc_walk_object(
					depth, 
					root, 
					cur_data, 
					node_handle, 
					json_node_handle, 
					user_data);
		break;
	default:	
		if (node_handle && depth != 0) 
			ret = node_handle(depth, root, &cur_data, json_node_handle, user_data);
		break;
	}

	return ret;
}

int
tc_get_file_json(
	char *file,
	unsigned long user_data,
	cJSON **json
)
{
	char *buf = NULL;
	FILE *fp = NULL;
	struct stat st_buf;
	cJSON *root = NULL;

	stat(file, &st_buf);
	if (st_buf.st_size == 0) {
		TC_ERRNO_SET(TC_WRONG_JSON_DATA);
		return TC_ERR;
	}
	buf = (char*)calloc(1, st_buf.st_size + 1);
	if (!buf) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	fp = fopen(file, "r");
	if (!fp) {
		TC_FREE(buf);
		TC_PANIC("open file %s error: %s\n", file, strerror(errno));
	}
	fread(buf, 1, st_buf.st_size, fp);
	fclose(fp);

	(*json) = cJSON_Parse(buf);
	if (!(*json)) {
		TC_ERRNO_SET(TC_WRONG_JSON_DATA);
		TC_FREE(buf);
		return TC_ERR;
	}
	TC_FREE(buf);
	return TC_OK;
}
 
int
tc_interface_json_walk(
	char *interface,
	char *interface_path,
	unsigned long *param, 
	node_handle_func node_handle,
	json_node_handle_func json_node_handle,
	unsigned long user_data
)
{
	int ret = 0;
	char *input_path = NULL;
	cJSON *input_data = NULL;
	cJSON *input_param = NULL;

	input_param = tc_interface_param_get(interface);
	if (input_param) {
		input_path = cJSON_Print(input_param);
		TC_FREE(input_path);
	}
	input_data = cJSON_GetObjectItem(input_param, interface_path);
	if (!input_data) {
		TC_ERRNO_SET(TC_WRONG_JSON_DATA);
		return TC_ERR;
	}
	input_path = input_data->valuestring;
	if (!input_path) {
		TC_ERRNO_SET(TC_WRONG_JSON_DATA);
		return TC_ERR;
	}
	ret = tc_get_file_json(input_path, user_data, &input_data);
	if (ret != TC_OK || !input_data)
		return TC_ERR;

	if (!(*param)) {
		if (input_data->type == cJSON_Array)
			(*param) = (unsigned long)cJSON_CreateArray();
		else 
			(*param) = (unsigned long)cJSON_CreateObject();
	}

	ret = tc_walk_json(
			0,
			input_data, 
			*param, 
			node_handle,
			json_node_handle,
			user_data);

	cJSON_Delete(input_data);
	return ret;
}
