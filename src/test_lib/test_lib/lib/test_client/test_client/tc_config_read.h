#ifndef TC_CONFIG_READ_H
#define TC_CONFIG_READ_H

cJSON *
tc_config_read_get(
	char *table_name
);

void
tc_config_read_del(
	char *table_name
);

int
tc_config_read_handle();

#define CR_INT(json, name, data) do{ \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	if (tmp && tmp->valuestring)  \
		data = atoi(tmp->valuestring); \
}while(0)

#define CR_STR(json, name, data) do{ \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	if (tmp && tmp->valuestring) \
		memcpy(data, tmp->valuestring, strlen(tmp->valuestring)); \
}while(0)

#define CR_SHORT(json, name, data) do{ \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	if (tmp && tmp->valuestring) \
		data = (short)atoi(tmp->valuestring); \
}while(0)

#define CR_USHORT(json, name, data) do{ \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	if (tmp && tmp->valuestring) \
		data = (unsigned long)atoi(tmp->valuestring); \
}while(0)

#define CR_IP(json, name, data) do{ \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	if (tmp && tmp->valuestring) \
		data = inet_addr(tmp->valuestring); \
}while(0)

#define CR_DURATION(json, name, data) do{ \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	int day = 0, hour = 0, min = 0, sec = 0; \
	if (tmp && tmp->valuestring) { \
		sscanf(tmp->valuestring, "%d:%d:%d:%d", &day, &hour, &min, &sec);\
		if (hour >= 24 || min >= 60 || sec >= 60) \
			TC_PANIC("Wrong duration :%s\n", tmp); \
		data = day * 3600 * 24 + hour * 3600 + min * 60 + sec; \
	} \
}while(0);

#define CR_DEV(json, name, data) do{ \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	if (tmp && tmp->valuestring) { \
		if (!strcmp(tmp->valuestring, "server")) \
			data = TC_DEV_SERVER; \
		else if (!strcmp(tmp->valuestring, "client")) \
			data = TC_DEV_CLIENT; \
	} \
}while(0)

#define CR_SIZE(json, name, data) do{ \
	int size = 0; \
	char *ch = NULL; \
	cJSON *tmp = cJSON_GetObjectItem(json, name); \
	if (tmp && tmp->valuestring) { \
		size = atoi(tmp->valuestring); \
		ch = strchr(tmp->valuestring, 'K'); \
		if (ch)  \
			size *= 1024; \
		else if ((ch = strchr(tmp->valuestring,'M'))) \
			size *= (1024 * 1024); \
		data = size; \
	} \
}while(0)

#endif
