#ifndef ST_COMM_CONFIG_H
#define ST_COMM_CONFIG_H 1

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
	ST_CT_OPT,
	ST_CT_CONFIG,
	ST_CT_MAX
};

#define ST_ALLOC_CONFIG(cur) do{ \
	STCommConfig *pStruCC = NULL; \
	ST_CALLOC(pStruCC, STCommConfig, 1); \
	cur->pStruNext = pStruCC; \
}while(0)

#define ST_SET_CONFIG(name, val, cur) do{ \
	int iLen = strlen(name); \
	ST_CALLOC(cur->sName, char, (iLen + 1)); \
	if( val ) \
	{\
		iLen = strlen(val); \
		ST_CALLOC(cur->sVal, char, (iLen + 1)); \
		memcpy(cur->sVal, val, iLen); \
	}\
}while(0)

#define ST_SET_CONFIG_WITH_NEXT(name, val, cur) do{ \
	ST_SET_CONFIG(name, val, cur); \
	ST_ALLOC_CONFIG(cur); \
	cur = cur->pStruNext; \
}while(0)

#define ST_DESTROY_COMM_CONFIG(head) do{ \
	STCommConfig *pStruCur = NULL, *pStruNext = head; \
	while(pStruNext) \
	{ \
		ST_FREE(pStruNext->sName); \
		ST_FREE(pStruNext->sVal); \
		pStruCur = pStruNext; \
		pStruNext = pStruCur->pStruNext; \
		ST_FREE(pStruCur); \
	} \
}while(0)

#define ST_GET_VAL(name, len, val, head, ret) do{\
	int ilen = 0; \
	STCommConfig *pStruCur = head; \
	while(pStruCur) \
	{ \
		ST_CMP_DATA(name, pStruCur->sName, ret); \
		if(ret == ST_OK) \
		{ \
			if(pStruCur->sVal && len > 0) \
			{ \
				ilen = strlen(pStruCur->sVal); \
				if( len < ilen )\
				{ \
					ret = ST_ERR;	\
				} \
				memcpy(val, pStruCur->sVal, ilen); \
			} \
			break; \
		}\
		pStruCur = pStruCur->pStruNext; \
	} \
}while(0)


#endif
