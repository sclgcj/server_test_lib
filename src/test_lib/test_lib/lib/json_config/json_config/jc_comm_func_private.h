#ifndef JC_COMM_FUNC_PRIVATE_H
#define JC_COMM_FUNC_PRIVATE_H

char *
jc_escape_splash(
	char *str
);

char *
jc_path_get(char *org_path);

int
jc_col_num_get(
	char separate,
	char *name,
	char *path
);

void *
jc_file_mmap(
	char *path,
	int  *size
);

char *
jc_file_val_get(
	int  col, 
	int  line,
	char separate,
	void *map_ptr,
	void **new_ptr
);

int
jc_file_munmap(
	int size,
	void *ptr
);

int
jc_file_line_num_get(
	char *path
);

#endif
