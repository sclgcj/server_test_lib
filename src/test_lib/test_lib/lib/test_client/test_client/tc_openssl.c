#include "tc_openssl_private.h"
#include "tc_init.h"
#include "tc_comm.h"
#include "tc_err.h"
#include "openssl/crypto.h"

struct tc_openssl_data {
	pthread_mutex_t *openssl_mutex;
};

static struct tc_openssl_data global_openssl_data;

static void
tc_openssl_lock_callback(
	int mode,
	int type,
	const char *file,
	int line
)
{
	(void)file;
	(void)line;

	if (mode & CRYPTO_LOCK) 
		pthread_mutex_lock(&global_openssl_data.openssl_mutex[type]);
	else 
		pthread_mutex_unlock(&global_openssl_data.openssl_mutex[type]);
}

static int
tc_openssl_setup()
{
	int i = 0;
	int num = 0;
	
	num = CRYPTO_num_locks();
	global_openssl_data.openssl_mutex = 
		(pthread_mutex_t*)calloc(num, sizeof(pthread_mutex_t));
	if (!global_openssl_data.openssl_mutex) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	for (; i < num; i++) {
		pthread_mutex_init(&global_openssl_data.openssl_mutex[i], NULL);
	}

	CRYPTO_set_locking_callback(tc_openssl_lock_callback);

	return TC_OK;
}

int
tc_openssl_init()
{
	return tc_init_register(tc_openssl_setup);
}

TC_MOD_INIT(tc_openssl_init);
