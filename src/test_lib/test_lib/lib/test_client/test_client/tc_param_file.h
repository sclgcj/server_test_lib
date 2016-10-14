#ifndef TC_PARAM_FILE_H
#define TC_PARAM_FILE_H

// The command to control the file.
enum {
	TC_FILE_CMD_ROW_ADD,
	TC_FILE_CMD_ROW_DEL,
	TC_FILE_CMD_COLUMN_ADD,
	TC_FILE_CMD_COLUMN_DEL
};

// The configure of column select type
enum {
	TC_SELECT_COLUMN_BY_NAME,
	TC_SELECT_COLUMN_BY_NUMBER,
};
struct tc_select_column {
	int select_type;
	union {
		int number;
		char name[32];
	}un;
};

struct tc_file_format {
	char col_seperate[16];
	char first_data[8];
};

// The options when the unique value reach the end
enum {
	TC_FILE_OUT_VAL_ABORT_USER,			//abort the user 
	TC_FILE_OUT_VAL_CONTINUE_IN_A_CYCLIC_MANNER,	//continue in a cyclic manner
	TC_FILE_OUT_VAL_CONTINUE_WITH_LAST_VALUE	//continue with last value
};

struct tc_unique_value {
	int out_of_value_oper;
	union {
		int alloc_value_type;
		int alloc_value;
	}un;
};

// The type to choose next value
enum {
	TC_FILE_CHOOSE_TYPE_RANDOM,		//random
	TC_FILE_CHOOSE_TYPE_SEQUENTIAL,		//sequential
	TC_FILE_CHOOSE_TYPE_UNIQUE		//unique
};

// When to choose the next value
enum {
	TC_FILE_CHOOSE_TIME_EACH_ITERATION,	//each iteration
	TC_FILE_CHOOSE_TIME_EACH_OCCURENCE,	//each occurence
	TC_FILE_CHOOSE_TIME_ONCE		//just 	once
};

struct tc_next_value_choose {
	int choose_type;
	int choose_time;
	struct tc_unique_value unique_value;	
};

struct tc_param_file {
	char *file_map;
	char file_path[256];
	int  file_size;
	int row;
	int column;
	struct tc_file_format file_format;
	struct tc_select_column select_col;
	struct tc_next_value_choose value_choose;	
};

#endif
