#include "toml.h"
#include "toml_private.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <unicode/ustring.h>

struct toml_stack_item {
	struct list_node list;
	enum toml_type list_type;
	struct toml_node *node;
};

static const char *
toml_type_to_str(enum toml_type type)
{
#define CASE_ENUM_TO_STR(x) case(x): return #x
	switch (type) {
	CASE_ENUM_TO_STR(TOML_ROOT);
	CASE_ENUM_TO_STR(TOML_TABLE);
	CASE_ENUM_TO_STR(TOML_LIST);
	CASE_ENUM_TO_STR(TOML_INT);
	CASE_ENUM_TO_STR(TOML_FLOAT);
	CASE_ENUM_TO_STR(TOML_STRING);
	CASE_ENUM_TO_STR(TOML_DATE);
	CASE_ENUM_TO_STR(TOML_BOOLEAN);
	CASE_ENUM_TO_STR(TOML_TABLE_ARRAY);
	default:
		return "unknown toml type";
	}
#undef CASE_ENUM_TO_STR
}

static size_t
utf32ToUTF8(char* dst, int len, uint32_t utf32)
{
	if (utf32 < 0x80) {
		if (len < 1)
			return 0;

		*dst = (uint8_t)utf32;
		return 1;
	}

	if (utf32 < 0x000800)
	{
		if (len < 2)
			return 0;

		dst[0] = (uint8_t)(0xc0 | (utf32 >> 6));
		dst[1] = (uint8_t)(0x80 | (utf32 & 0x3f));
		return 2;
	}

	if (utf32 < 0x10000)
	{
		if (len < 3)
			return 0;

		dst[0] = (uint8_t)(0xE0 | (utf32 >> 12));
		dst[1] = (uint8_t)(0x80 | ((utf32 & 0x0FC0) >> 6));
		dst[2] = (uint8_t)(0x80 | (utf32 & 0x003F));
		return 3;
	}

	if (len < 4)
		return 0;

	dst[0] = (uint8_t)(0xF0 | (utf32 >> 18));
	dst[1] = (uint8_t)(0x80 | ((utf32 & 0x03F000) >> 12));
	dst[2] = (uint8_t)(0x80 | ((utf32 & 0x000FC0) >> 6));
	dst[3] = (uint8_t)(0x80 | (utf32 & 0x00003F));

	return 4;
}

bool add_node_to_tree(struct list_head* list_stack, struct toml_node* cur_table, struct toml_node* node, char* name, char** parse_error, int* malloc_error, int cur_line)
{
	struct toml_stack_item *cur_list =
					list_tail(list_stack, struct toml_stack_item, list);

	if (cur_list) {
		if (cur_list->list_type && cur_list->list_type != node->type) {
			asprintf(parse_error,
					"incompatible types list %s this %s line %d\n",
					toml_type_to_str(cur_list->list_type),
					toml_type_to_str(node->type), cur_line);
			return false;
		}
		cur_list->list_type = node->type;

		struct toml_list_item *item = malloc(sizeof(*item));
		if (!item) {
			*malloc_error = 1;
			return false;
		}

		memcpy(&item->node, node, sizeof(*node));
		item->node.name = NULL;
		list_add_tail(&cur_list->node->value.list, &item->list);
	} else {
		struct toml_table_item *item = malloc(sizeof(*item));
		if (!item) {
			*malloc_error = 1;
			return false;
		}
		memcpy(&item->node, node, sizeof(*node));
		item->node.name = name;
		list_add_tail(&cur_table->value.map, &item->map);
	}

	return true;
}

%%{
	machine toml;

	whitespace = [\t ]*;

	name = (print - ('#'|'='|'"'|whitespace))+					>{ts = p;};
	name_in_double_quotes = (print - '"')+						>{ts = p;};
	name_in_single_quotes = (print - "'")+						>{ts = p;};
	tablename =  (print - ('#'|']'|'['|'"'|whitespace))+		>{ts = p;};
	tablename_in_double_quotes =  (print - '"')+				>{ts = p;};
	tablename_in_single_quotes =  (print - "'")+				>{ts = p;};

	action saw_key {
		struct toml_table_item* item = NULL;
		list_for_each(&cur_table->value.map, item, map) {
			if (strncmp(item->node.name, ts, namelen + 1) != 0)
				continue;

			asprintf(&parse_error,
					"duplicate key %s line %d\n",
					item->node.name, cur_line);
			fbreak;
		}

		while (ts[namelen] == ' ' || ts[namelen] == '\t')
			namelen--;

		name = strndup(ts, namelen + 1);
	}

	action saw_bool {
		struct toml_node node;

		node.type = TOML_BOOLEAN;
		node.value.integer = number;

		if (!add_node_to_tree(&list_stack, cur_table, &node, name, &parse_error, &malloc_error, cur_line))
			fbreak;

		struct toml_stack_item *cur_list =
					list_tail(&list_stack, struct toml_stack_item, list);
		if (cur_list)
			fnext list;
		else if (inline_table)
			fnext inline_table;
	}

	action saw_int {
		char*				te = p;
		struct toml_node	node;

		node.type = TOML_INT;
		node.value.integer = negative ? -number : number;

		if (!add_node_to_tree(&list_stack, cur_table, &node, name, &parse_error, &malloc_error, cur_line))
			fbreak;

		struct toml_stack_item *cur_list =
					list_tail(&list_stack, struct toml_stack_item, list);
		if (cur_list)
			fnext list;
		else if (inline_table)
			fnext inline_table;
	}

	action saw_float {
		char*				te = p;
		struct toml_node	node;

		floating = strtod(ts, &te);

		node.type = TOML_FLOAT;
		node.value.floating.value = floating;
		node.value.floating.precision = precision;

		if (!node.value.floating.precision && !exponent) {
			asprintf(&parse_error, "bad float\n");
			fbreak;
		}

		exponent = 0;

		if (!add_node_to_tree(&list_stack, cur_table, &node, name, &parse_error, &malloc_error, cur_line))
			fbreak;

		struct toml_stack_item *cur_list =
					list_tail(&list_stack, struct toml_stack_item, list);
		if (cur_list)
			fnext list;
		else if (inline_table)
			fnext inline_table;
	}

	action saw_string {
		int					len = strp - string + 1;
		struct toml_node	node;

		*strp = 0;

		node.type = TOML_STRING;
		node.value.string = malloc(len);
		if (!node.value.string) {
			malloc_error = 1;
			fbreak;
		}
		memcpy(node.value.string, string, len);

		if (!add_node_to_tree(&list_stack, cur_table, &node, name, &parse_error, &malloc_error, cur_line))
			fbreak;

		struct toml_stack_item *cur_list =
					list_tail(&list_stack, struct toml_stack_item, list);
		if (cur_list)
			fnext list;
		else if (inline_table)
			fnext inline_table;
	}

	action saw_date {
		char*	te = p;
		struct	toml_node node;

		node.type = TOML_DATE;
		node.value.rfc3339_time.epoch = timegm(&tm);
		node.value.rfc3339_time.offset_sign_negative = time_offset_is_negative;
		node.value.rfc3339_time.offset = time_offset;
		node.value.rfc3339_time.offset_is_zulu = time_offset_is_zulu;
		if (secfrac_ptr)
			node.value.rfc3339_time.sec_frac = strtol(secfrac_ptr, &te, 10);
		else
			node.value.rfc3339_time.sec_frac = -1;

		if (!add_node_to_tree(&list_stack, cur_table, &node, name, &parse_error, &malloc_error, cur_line))
			fbreak;

		struct toml_stack_item *cur_list =
					list_tail(&list_stack, struct toml_stack_item, list);
		if (cur_list)
			fnext list;
		else if (inline_table)
			fnext inline_table;
	}

	action start_list {
		struct toml_node *node;

		/*
		 * if the list stack is empty add this list to the table
		 * otherwise it should be added the to list on top of the stack
		 */
		if (list_empty(&list_stack)) {
			struct toml_table_item *item = malloc(sizeof(*item));
			if (!item) {
				malloc_error = 1;
				fbreak;
			}

			/* first add it to the table */
			item->node.type = TOML_LIST;
			list_head_init(&item->node.value.list);
			list_add_tail(&cur_table->value.map, &item->map);
			item->node.name = name;
			node = &item->node;
		} else {
			struct toml_stack_item* tail =
						list_tail(&list_stack, struct toml_stack_item, list);

			if (tail->list_type && tail->list_type != TOML_LIST) {
				asprintf(&parse_error,
						"incompatible types list %s this %s line %d\n",
						toml_type_to_str(tail->list_type),
						toml_type_to_str(TOML_BOOLEAN), cur_line);
				fbreak;
			}

			struct toml_list_item *item = malloc(sizeof(*item));
			if (!item) {
				malloc_error = 1;
				fbreak;
			}

			tail->list_type = TOML_LIST;
			item->node.type = TOML_LIST;
			item->node.name = NULL;
			list_head_init(&item->node.value.list);

			list_add_tail(&tail->node->value.list, &item->list);
			node = &item->node;
		}

		/* push this list onto the stack */
		struct toml_stack_item *stack_item = malloc(sizeof(*stack_item));
		if (!stack_item) {
			malloc_error = 1;
			fbreak;
		}
		stack_item->node = node;
		stack_item->list_type = 0;
		list_add_tail(&list_stack, &stack_item->list);
	}

	action end_list {
		struct toml_stack_item *tail =
						list_tail(&list_stack, struct toml_stack_item, list);

		list_del(&tail->list);
		free(tail);

		if (!list_empty(&list_stack))
			fnext list;
		else if (inline_table)
			fnext inline_table;
	}

	action saw_inline_table {
		char*					tablename = name;
		struct toml_table_item*	item;
		struct toml_node*		place = cur_table ? cur_table : toml_root;
		bool					found = false;

		inline_table = true;

		list_for_each(&place->value.map, item, map) {
			if (strcmp(item->node.name, tablename) != 0)
				continue;

			found = true;
			break;
		}

		if (found)
		{
			asprintf(&parse_error, "duplicate entry %s\n", tablename);
			fbreak;
		}

		item = malloc(sizeof(*item));
		if (!item) {
			malloc_error = 1;
			fbreak;
		}

		item->node.name = strdup(tablename);
		item->node.type = TOML_TABLE;
		list_head_init(&item->node.value.map);
		list_add_tail(&place->value.map, &item->map);

		cur_table = &item->node;

		free(tablename);
	}

	action saw_table {
		char *ancestor, *tofree = NULL, *tablename;
		int item_added = 0;
		int len = (int)(p-ts);

		struct toml_node *place = toml_root;

		tofree = tablename = strndup(ts, len);

		while ((ancestor = strsep(&tablename, "."))) {
			struct toml_table_item *item = NULL;
			int found = 0;

			if (strcmp(ancestor, "") == 0) {
				asprintf(&parse_error, "empty implicit table");
				fbreak;
			}

			list_for_each(&place->value.map, item, map) {
				if (strcmp(item->node.name, ancestor) == 0) {
					place = &item->node;
					found = 1;
					break;
				}
			}

			if (found)
				continue;

			/* this is the auto-vivification */
			item = malloc(sizeof(*item));
			if (!item) {
				malloc_error = 1;
				fbreak;
			}
			item->node.name = strdup(ancestor);
			item->node.type = TOML_TABLE;
			list_head_init(&item->node.value.map);
			list_add_tail(&place->value.map, &item->map);

			place = &item->node;
			item_added = 1;
		}

		if (!item_added) {
			asprintf(&parse_error, "Duplicate item %.*s", len, ts);
			fbreak;
		}

		if (place->type != TOML_TABLE) {
			asprintf(&parse_error, "Attempt to overwrite table %.*s", len, ts);
			fbreak;
		}

		free(tofree);

		cur_table = place;
	}

	action saw_table_array {
		char *ancestor, *tofree, *tablename;

		struct toml_node *place = toml_root;
		struct toml_list_item *new_table_entry = NULL;

		tofree = tablename = strndup(ts, (int)(p-ts-1));

		while ((ancestor = strsep(&tablename, "."))) {
			struct toml_table_item *item = NULL;
			int found = 0;

			list_for_each(&place->value.map, item, map) {
				if (!item->node.name)
					continue;
				
				if (strcmp(item->node.name, ancestor) == 0) {
					place = &item->node;
					found = 1;
					break;
				}
			}

			if (found)
				continue;

			/* this is the auto-vivification */

			/*
			 * Create a table array node and insert it into the heirarchy
			 */
			item = malloc(sizeof(*item));
			if (!item) {
				malloc_error = 1;
				fbreak;
			}
			item->node.name = strdup(ancestor);
			item->node.type = TOML_TABLE_ARRAY;
			list_head_init(&item->node.value.list);
			list_add_tail(&place->value.map, &item->map);

			place = &item->node;
		}

		if (place->type != TOML_TABLE_ARRAY) {
			asprintf(&parse_error, "Attempt to overwrite table %.*s",
															(int)(p-ts), ts);
			fbreak;
		}

		free(tofree);

		/*
		 * Create a table which becomes the last element in the list
		 * of maps (table array is a list of maps)
		 */
		new_table_entry = malloc(sizeof(*new_table_entry));
		if (!new_table_entry) {
			malloc_error = 1;
			fbreak;
		}

		new_table_entry->node.type = TOML_TABLE;
		new_table_entry->node.name = NULL;
		list_head_init(&new_table_entry->node.value.map);
		list_add_tail(&place->value.list, &new_table_entry->list);

		cur_table = &new_table_entry->node;
	}

	action saw_utf16 {
		UChar		utf16[2] = { 0 };
		int32_t		len = sizeof(string) - (strp - string);
		int32_t		outLen;
		UErrorCode	err = U_ZERO_ERROR;
		char		utf16_str[5] = { 0 };

		memcpy(utf16_str, utf_start, 4);
		*utf16 = strtoul(utf16_str, NULL, 16);

		u_strToUTF8(strp, len, &outLen, utf16, 1, &err);
		strp += outLen;
		fret;
	}

	action saw_utf32 {
		uint32_t	utf32;
		int32_t		len = sizeof(string) - (strp - string);
		int32_t		outLen;
		char		utf32_str[9] = { 0 };

		memcpy(utf32_str, utf_start, 8);
		utf32 = strtoul(utf32_str, NULL, 16);

		outLen = utf32ToUTF8(strp, len, utf32);
		strp += outLen;
		fret;
	}

	action bad_escape {
		asprintf(&parse_error, "bad escape \\%c", *p);
		fbreak;
	}

	lines = (
		start: (
			# count the indentation to know where the tables end
			'#'			>{in_text = 0; fcall comment;}				->start	|
			[\t ]+		>{in_text = 0; indent = 0;} @{indent++;}	->start	|
			[\n]		>{in_text = 0; cur_line++;}					->start	|
			[\0]		>{in_text = 0; fbreak;}						->start	|
			[^\t \n]	@{fhold;} %{in_text = 1;}					->text
		),

		# just discard everything until newline
		comment: ( [^\n]*[\n] ${cur_line++; fret;} ),

		# a table
		table: (
			tablename ']' @saw_table										->start	|
			'"' tablename_in_double_quotes '"' @saw_table ']'				->start	|
			"'" tablename_in_single_quotes "'" @saw_table ']'				->start	|
			'[' tablename ']' ']' @saw_table_array							->start	|
			'[' '"' tablename_in_double_quotes '"' ']' @saw_table_array ']'	->start	|
			'[' "'" tablename_in_single_quotes "'" ']' @saw_table_array ']'	->start
		),

		# the boolean data type
		true:	( any	>{fhold;} $saw_bool	->start ),
		false:	( any	>{fhold;} $saw_bool	->start ),

		basic_string: (
			["]				-> basic_empty_or_multi_line	|
			[^"] ${fhold;}	-> basic_string_contents
		),

		basic_empty_or_multi_line: (
			["]							-> basic_multi_line_start	|
			[^"] $saw_string ${fhold;}	-> start
		),

		basic_multi_line_start: (
			'\n' ${cur_line++;}	-> basic_multi_line |
			[^\n] ${fhold;}		-> basic_multi_line
		),

		basic_multi_line: (
			["]										-> basic_multi_line_quote	|
			[\n]		${cur_line++;*strp++=fc;}	-> basic_multi_line			|
			[\\]									-> basic_multi_line_escape	|
			[^"\n\\]	${*strp++=fc;}				-> basic_multi_line
		),

		basic_multi_line_escape: (
			[\n]	${cur_line++;}			-> basic_multi_line_rm_ws	|
			[^\n]	${fcall str_escape;}	-> basic_multi_line
		),

		basic_multi_line_rm_ws: (
			[\n]	${cur_line++;}	-> basic_multi_line_rm_ws |
			[ \t]					-> basic_multi_line_rm_ws |
			[^ \t\n]	${fhold;}	-> basic_multi_line
		),

		basic_multi_line_quote: (
			["]								-> basic_multi_line_quote_2	|
			[^"]	${fhold;*strp++='"';}	-> basic_multi_line
		),

		basic_multi_line_quote_2: (
			["]		$saw_string							-> start |
			[^"]	${fhold;*strp++='"';*strp++='"';}	-> basic_multi_line
		),

		# String, we have to escape \0, \t, \n, \r, everything else can
		# be prefixed with a slash and the slash just gets dropped
		basic_string_contents: (
			'"'			$saw_string					-> start					|
			[\n]		${cur_line++; *strp++=fc;}	-> basic_string_contents	|
			[\\]		${fcall str_escape;}		-> basic_string_contents	|
			[^"\n\\]	${*strp++=fc;}				-> basic_string_contents
		),

		str_escape: (
			'b'	${*strp++=0x8;}			${fret;}	|
			't'	${*strp++='\t';}		${fret;}	|
			'n'	${*strp++='\n';}		${fret;}	|
			'f'	${*strp++=0xc;}			${fret;}	|
			'r'	${*strp++='\r';}		${fret;}	|
			'0'	${*strp++=0;}			${fret;}	|
			'"'	${*strp++='"';}			${fret;}	|
			'/'	${*strp++='/';}			${fret;}	|
			'\\'	${*strp++='\\';}	${fret;}	|
			'u'							-> unicode4	|
			'U'							-> unicode8	|
			[^btnfr0uU"/\\] $bad_escape
		),

		unicode4: (
			xdigit{4} >{utf_start=p;} @saw_utf16
		),

		unicode8: (
			xdigit{8} >{utf_start=p;} @saw_utf32
		),

		# When we don't know yet if this is going to be a date or a number
		# this is the state
		number_or_date: (
			'_' ? digit ${number *= 10; number += fc-'0';}	->number_or_date	|
			'-'	${tm.tm_year = number - 1900;}				->date				|
			[eE]											->exponent_part		|
			[.]	>{precision = 0;}							->fractional_part	|
			[\t ,\]\n\0] $saw_int ${fhold;}					->start
		),

		# Fractional part of a double
		fractional_part: (
			[0-9]	${precision++;}				->fractional_part	|
			[eE]								->exponent_part		|
			[^0-9eE]	$saw_float ${fhold;}	->start
		),

		exponent_part: (
			[\-\+0-9]	${exponent=true;}		->exponent_part	|
			[^\-\+0-9]	$saw_float ${fhold;}	->start
		),

		# Zulu date, we've already picked up the first four digits and the '-'
		# when figuring this was a date and not a number
		date: ( '' >{secfrac_ptr = NULL; time_offset = 0; time_offset_is_negative = 0; time_offset_is_zulu = 0;}
			digit{2} @{tm.tm_mon = atoi(fpc-1) - 1;}
			'-'
			digit{2} @{tm.tm_mday = atoi(fpc-1);}
			'T'
			digit{2} @{tm.tm_hour = atoi(fpc-1);}
			':'
			digit{2} @{tm.tm_min = atoi(fpc-1);}
			':'
			digit{2} @{tm.tm_sec = atoi(fpc-1);} -> fractional_second_or_offset
		),

		fractional_second_or_offset: (
			'.' digit* >{secfrac_ptr=p;} >{fhold;}	-> time_offset	|
			[^.] @{fhold;}							-> time_offset
		),

		time_offset: (
			('-' @{time_offset_is_negative=1;}|'+')
				digit{2} @{time_offset = atoi(fpc-1) * 60;}
				':'
				digit{2} @{time_offset += atoi(fpc-1);}
				@saw_date								-> start |
			'Z' >{time_offset_is_zulu = 1;} @saw_date	-> start
		),

		literal_string: (
			[']					-> literal_empty_or_multi_line		|
			[^']	${fhold;}	-> literal_string_contents
		),

		literal_string_contents: (
			[']		$saw_string		->start						|
			[^']	${*strp++=fc;}	->literal_string_contents
		),

		literal_empty_or_multi_line: (
			[']								-> literal_multi_line_start	|
			[^']	${fhold;} $saw_string	-> start
		),

		literal_multi_line_start: (
			'\n' ${cur_line++;}	-> literal_multi_line	|
			[^\n] ${fhold;}		-> literal_multi_line
		),

		literal_multi_line: (
			[']									-> literal_multi_line_quote	|
			[\n]	${cur_line++;*strp++=fc;}	-> literal_multi_line		|
			[^'\n]	${*strp++=fc;}				-> literal_multi_line
		),

		# saw 1 quote, if there's not another one go back to literalMultiLine
		literal_multi_line_quote: (
			[']									-> literal_multi_line_second_quote |
			[^']	${fhold;*strp++='\'';}		-> literal_multi_line
		),

		# saw 2 quotes, if there's not another one go back to literalMultiLine
		# if there is another then terminate the string
		literal_multi_line_second_quote: (
			[']		$saw_string							-> start				|
			[^']	${fhold;*strp++='\'';*strp++='\'';}	-> literal_multi_line
		),

		# Non-list value
		singular: (
			'true'		@{number = 1;}									-> true				|
			'false'		@{number = 0;}									-> false			|
			'"'			${strp = string;}								-> basic_string		|
			[']			${strp = string;}								-> literal_string	|
			('-'|'+')	${negative = fc == '-'; number = 0; ts = p;}	-> number_or_date	|
			digit		${negative = false; ts = p; number = fc-'0';}	-> number_or_date
		),

		# A list of values
		list: (
			'#'		>{fcall comment;}	->list	|
			'\n'	${cur_line++;}		->list	|
			[\t ]						->list	|
			','							->list	|
			']'	$end_list				->start	|
			[^#\t, \n\]] ${fhold;}		->val
		),

		inline_table: (
			','								-> inline_table |
			[\t ]							-> inline_table |
			'}'	@{inline_table = false;}	-> start		|
			[^\t ,}] @{fhold;}				-> key
		),

		# A val can be either a list or a singular value
		val: (
			'#'	>{fcall comment;}	->val			|
			'\n' ${ cur_line++; }	->val			|
			[\t ]					->val			|
			'['	$start_list			->list			|
			']'	$end_list			->start			|
			'{'	$saw_inline_table	->inline_table	|
			[^#\t \n[\]{] ${fhold;}	->singular
		)

		# A regular key
		key: (
			name @{namelen = (int)(p-ts);} whitespace '=' $saw_key								->val	|
			'"' name_in_double_quotes '"' @{namelen = (int)(p-ts-1);} whitespace '=' $saw_key	->val	|
			"'" name_in_single_quotes "'" @{namelen = (int)(p-ts-1);} whitespace '=' $saw_key	->val
		),

		# Text stripped of leading whitespace
		text: (
			'#'	>{fcall comment;}	->text	|
			'['						->table	|
			[\t ]					->text	|
			'\n' ${cur_line++;}		->start	|
			[^#[\t \n]	${fhold;}	->key
		)
	);

	main := lines?;
}%%

%%write data;

int
toml_parse(struct toml_node* toml_root, char* buf, int buflen)
{
	int indent = 0, cs, cur_line = 1;
	char *p, *pe;
	char *ts;
	char string[1024], *strp;
	int precision;
	int namelen;
	int64_t number;
	bool negative;
	struct tm tm;
	double floating;
	char *name;
	char *parse_error = NULL;
	int malloc_error = 0;
	char* utf_start;
	int top = 0, stack[1024];
	int in_text = 0;
	int time_offset = 0;
	char* secfrac_ptr;
	bool time_offset_is_negative = 0;
	bool time_offset_is_zulu = 0;
	bool inline_table = false;
	int exponent = 0;

	struct toml_node *cur_table = toml_root;

	struct list_head list_stack;
	list_head_init(&list_stack);

	assert(toml_root->type == TOML_ROOT);

	%% write init;

	p = buf;
	pe = buf + buflen + 1;

	%% write exec;

	if (malloc_error) {
		fprintf(stderr, "malloc failed, line %d\n", cur_line);
		return 1;
	}

	if (parse_error) {
		fprintf(stderr, "%s at %d p = %.5s\n", parse_error, cur_line, p);
		free(parse_error);
		return 1;
	}

	if (in_text) {
		fprintf(stderr, "not in start, line %d\n", cur_line);
		return 1;
	}

	/* check we have consumed the entire buffer */
	if (p != pe) {
		fprintf(stderr, "entire buffer unconsumed, line %d\n", cur_line);
		return 1;
	}

	if (cs == toml_error) {
		fprintf(stderr, "PARSE_ERROR, line %d, p = '%.5s'", cur_line, p);
		return 1;
	}

	return 0;
}
