#include "json_config_private.h"
#include "json_config_comm_func_private.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define JC_FILE_PATH 256
#define JC_PATH_OFFSET 10

char *
jc_escape_splash(
	char *str
)
{
	int len = 0;
	int cnt = 0;
	char *new_str = NULL;

	len = strlen(str);
	new_str = (char*)calloc(sizeof(char), len + 1);
	while(*str) {
		if (*str == '\\') 
			goto next;
		*(new_str + cnt) = *str;
		cnt++;
next:
		str++;
	}

	return new_str;
}

char *
jc_path_get(
	char *org_path
)
{
	char buf[JC_FILE_PATH] = { 0 };
	char *path = NULL;

	getcwd(buf, JC_FILE_PATH);
	path = (char*)calloc(sizeof(char), 
			strlen(buf) + strlen(org_path) + JC_PATH_OFFSET);
	if (!path) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				strlen(buf) + strlen(org_path) + JC_PATH_OFFSET, 
				strerror(errno));
		exit(0);
	}

	sprintf(path, "%s/%s", buf, org_path);

	fprintf(stderr, "lib_path = %s\n", path);

	return path;
}

int
jc_col_num_get(
	char separate,
	char *name,
	char *path
)
{
	int cnt = 1024, cur = 0;
	char *buf = NULL, *tmp = NULL;
	FILE *fp = NULL;

	fp = fopen(path, "r");
	if (!fp) {
		fprintf(stderr, "fopen %s error : %s\n", 
				path, strerror(errno));
		exit(0);
	}
	buf = (char*)calloc(sizeof(char), cnt);
	if (!buf) {
		fprintf(stderr, "calloc %d bytes error : %s\n", 
				cnt, strerror(errno));
		exit(0);
	}
	while (1) {
		fgets(buf + cur, cnt, fp);
		if (buf[cnt - 2] != 0)
			cnt *= 2;
		else 
			break;
		buf = (char*)calloc(1, cnt);
		cur += cnt - 1;
		memset(buf + cur, 0, cnt - cur);
	}
	cur = 0;
	tmp = buf;
	while (1) {
		tmp = strchr(tmp, separate);
		if (!tmp)
			break;
		tmp++;
		if (!strcmp(tmp, name))	
			break;
		cur++;
	}
	if (!tmp) {
		fprintf(stderr, "no column named %s\n", name);
		return JC_ERR;
	}

	free(buf);

	return cur;
}

void *
jc_file_mmap(
	char *path,
	int  *size
)
{
	int fd = 0;
	void *ptr = NULL;
	struct stat sbuf;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "open file %s error : %s\n", 
				path, strerror(errno));
		exit(0);
	}
	memset(&sbuf, 0, sizeof(sbuf));
	stat(path, &sbuf);
	if (size)
		(*size) = sbuf.st_size;
	ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "mmap error : %s\n", strerror(errno));
		exit(0);
	}
	close(fd);
	return ptr;
}

static char *
jc_file_str_get(
	int num,
	char separate,
	char *ptr,
	void **new_ptr
)
{
	int i = 0;
	char *line_str = NULL, *col_str = NULL;
	char *end = NULL, *start = (char*)ptr;

	for (; i <= num; i++) {
		start = strchr(start, separate);
		if (!start) {
			fprintf(stderr, "no more value for %d\n", num);
			return NULL;
		}
		start++;
	}
	end = strchr(start, separate);
	if (new_ptr) 
		(*new_ptr) = (void*)start;
	if (end) 
		i = end - start;
	else
		i = strlen(start);
	line_str = (char*)calloc(1, i + 1);
	memcpy(line_str, start, i);

	return line_str;
}

char *
jc_file_line_get(
	int line,
	void *map_ptr,
	void **new_ptr
)
{
	return jc_file_str_get(line, '\n', (char*)map_ptr, new_ptr);
}

char *
jc_file_col_get(
	int col,
	char separate,
	char *line
)
{
	void *next = NULL;

	return jc_file_str_get(col, separate, line, &next);
}

char *
jc_file_val_get(
	int  col, 
	int  line,
	char separate,
	void *map_ptr,
	void **new_ptr
)
{
	char *line_str = NULL, *col_str = NULL;

	line_str = jc_file_line_get(line, map_ptr, new_ptr);
	if (!line_str)
		return NULL;
	col_str = jc_file_col_get(col, separate, line_str);

	free(line_str);
	line_str = NULL;

	return col_str;
}

int
jc_file_munmap(
	int size,
	void *ptr
)
{
	munmap(ptr, size);

	return JC_OK;
}

int
jc_file_line_num_get(
	char *path
)
{
	int num = 0;
	int size = 0;
	char *ptr = NULL;
	char *map = NULL;

	map = (char*)jc_file_mmap(path, &size);
	if (!map)
		return JC_ERR;

	ptr = map;
	while (ptr) {
		ptr = strchr(ptr, '\n');
		ptr++;
		num++;
	}

	jc_file_munmap(size, map);

	return num;
}


