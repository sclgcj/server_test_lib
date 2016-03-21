#ifndef SERVER_TEST_COMM_CONFIG_H
#define SERVER_TEST_COMM_CONFIG_H 1

struct _STOptConfig;
typedef struct _STOptConfig STCommConfig,*PSTCommConfig;

struct _STOptConfig
{
	char *sName;
	char *sVal;
	STCommConfig *pStruNext;
};

enum
{
	SERVER_TEST_CT_OPT,
	SERVER_TEST_CT_CONFIG,
	SERVER_TEST_CT_MAX
};

#define SERVER_TEST_ALLOC_CONFIG(cur) do{ \
	STCommConfig *pStruCC = NULL; \
	SERVER_TEST_CALLOC(pStruCC, STCommConfig, 1); \
	cur->pStruNext = pStruCC; \
}while(0)

#define SERVER_TEST_SET_CONFIG(name, val, cur) do{ \
	int iLen = strlen(name); \
	SERVER_TEST_CALLOC(cur->sName, char, (iLen + 1)); \
	if( val ) \
	{\
		iLen = strlen(val); \
		SERVER_TEST_CALLOC(cur->sVal, char, (iLen + 1)); \
		memcpy(cur->sVal, val, iLen); \
	}\
}while(0)

#define SERVER_TEST_SET_CONFIG_WITH_NEXT(name, val, cur) do{ \
	SERVER_TEST_SET_CONFIG(name, val, cur); \
	SERVER_TEST_ALLOC_CONFIG(cur); \
	cur = cur->pStruNext; \
}while(0)

#define SERVER_TEST_DESTROY_COMM_CONFIG(head) do{ \
	STCommConfig *pStruCur = NULL, *pStruNext = head; \
	while(pStruNext) \
	{ \
		SERVER_TEST_FREE(pStruNext->sName); \
		SERVER_TEST_FREE(pStruNext->sVal); \
		pStruCur = pStruNext; \
		pStruNext = pStruCur->pStruNext; \
		SERVER_TEST_FREE(pStruCur); \
	} \
}while(0)

#define SERVER_TEST_GET_VAL(name, len, val, head, ret) do{\
	int ilen = 0; \
	STCommConfig *pStruCur = head; \
	while(pStruCur) \
	{ \
		SERVER_TEST_CMP_DATA(name, pStruCur->sName, ret); \
		if(ret == SERVER_TEST_OK) \
		{ \
			if(pStruCur->sVal && len > 0) \
			{ \
				ilen = strlen(pStruCur->sVal); \
				if( len < ilen )\
				{ \
					ret = SERVER_TEST_ERR;	\
				} \
				memcpy(val, pStruCur->sVal, ilen); \
			} \
			break; \
		}\
		pStruCur = pStruCur->pStruNext; \
	} \
}while(0)


#endif
