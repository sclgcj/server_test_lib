#include "tc_std_comm.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_thread.h"
#include "tc_err.h"
#include "tc_init_module.h"
#include "tc_global_log_private.h"

/*
 * Normally, upstream don't  need to care about 
 * how downstream init its modules, what they 
 * need to understand is how to init their own
 * code. All apps can use TC_MOD_INIT to declare
 * its data before main start, but please note,
 * if there is some init operation needing the ones
 * which depend on the the downsteam, it'd better
 * use tc_init_register function in its init function
 * to register it. Because the archecture will call 
 * these function after its own modules initialization.
 */

struct tc_init_conf {
	int (*func)();
	struct list_head node;
};

/*
 * we hope ervery module use this to register its init function to initialize 
 * their own data
 */
static int 
tc_register_handle(
	struct tc_init_module *mod,
	int (*func)(),
	struct list_head *head
)
{
	struct tc_init_conf *init_conf = NULL;

	init_conf = (struct tc_init_conf*)calloc(1, sizeof(*init_conf));
	if (!init_conf) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}

	init_conf->func = func;
	list_add_tail(&init_conf->node, head);

	return TC_OK;
}

int
tc_init_module_local_init_register(
	struct tc_init_module *mod,
	int (*init)()
)
{
	int ret = 0;

	pthread_mutex_lock(&mod->local_init_mutex);
	ret = tc_register_handle(mod, init, &mod->local_init_list);
	pthread_mutex_unlock(&mod->local_init_mutex);

	return ret;
}

int
tc_init_module_init_register(
	struct tc_init_module *mod,
	int (*init)()
)
{
	int ret = 0;

	pthread_mutex_lock(&mod->init_mutex);
	tc_register_handle(mod, init, &mod->init_list);
	pthread_mutex_unlock(&mod->init_mutex);

	return ret;
}

int
tc_init_module_local_uninit_register(
	struct tc_init_module *mod,
	int (*uninit)()
)
{
	int ret = 0;

	pthread_mutex_lock(&mod->local_uninit_mutex);
	ret = tc_register_handle(mod, uninit, &mod->local_uninit_list);
	pthread_mutex_unlock(&mod->local_uninit_mutex);

	return ret;
}

int
tc_init_module_uninit_register(
	struct tc_init_module *mod,
	int (*uninit)()
)
{
	int ret = 0;

	pthread_mutex_lock(&mod->uninit_mutex);
	ret = tc_register_handle(mod, uninit, &mod->uninit_list);
	pthread_mutex_unlock(&mod->uninit_mutex);

	return ret;
}

static int
tc_init_module_handle(
	struct list_head *head
)
{
	int ret = TC_OK;
	int count = 0;
	struct tc_init_conf *init_conf = NULL;

	list_for_each_entry(init_conf, (head), node) {
		TC_GINFO("count = %d\n", count++);
		if (init_conf->func) {
			ret = init_conf->func();
			if (ret != TC_OK) 
				break;
		}
	}

	return ret;
}

int
tc_init_module_uninit(
	struct tc_init_module *mod
)
{
	int flag = 0;
	pthread_mutex_lock(&mod->mutex);
	flag = mod->uninit_flag;
	pthread_mutex_unlock(&mod->mutex);
	if (flag == 1) 
		goto out;
	else
		mod->uninit_flag = 1;
	tc_thread_exit_wait();
	pthread_mutex_lock(&mod->uninit_mutex);
	tc_init_module_handle(&mod->uninit_list);
	pthread_mutex_unlock(&mod->uninit_mutex);
	pthread_mutex_lock(&mod->local_uninit_mutex);
	tc_init_module_handle(&mod->local_uninit_list);
	pthread_mutex_unlock(&mod->local_uninit_mutex);
out:
	return TC_OK;
}

int
tc_init_module_init(
	struct tc_init_module *mod	
)
{
	int flag = 0;

	pthread_mutex_lock(&mod->mutex);
	flag = mod->init_flag;
	pthread_mutex_unlock(&mod->mutex);
	if (flag == 1)
		return TC_OK;

	pthread_mutex_lock(&mod->local_init_mutex);
	tc_init_module_handle(&mod->local_init_list);
	pthread_mutex_unlock(&mod->local_init_mutex);
	pthread_mutex_lock(&mod->init_mutex);
	tc_init_module_handle(&mod->init_list);
	pthread_mutex_unlock(&mod->init_mutex);

	pthread_mutex_lock(&mod->mutex);
	mod->init_flag = 1;
	pthread_mutex_unlock(&mod->mutex);

	return TC_OK;
}

int
tc_init_module_init_test(
	struct tc_init_module *mod
)
{
	int ret = 0;

	pthread_mutex_lock(&mod->mutex);
	ret = mod->init_flag;
	pthread_mutex_unlock(&mod->mutex);

	return ret;
}

struct tc_init_module *
tc_init_module_create()
{
	struct tc_init_module *mod = NULL;

	mod = (struct tc_init_module *)calloc(1, sizeof(*mod));
	if (!mod)
		TC_PANIC("not enough memory for %d bytes\n", sizeof(*mod));

	pthread_mutex_init(&mod->mutex, NULL);
	mod->init_flag = 0;
	mod->uninit_flag = 0;
	INIT_LIST_HEAD(&mod->init_list);
	INIT_LIST_HEAD(&mod->uninit_list);
	INIT_LIST_HEAD(&mod->local_init_list);
	INIT_LIST_HEAD(&mod->local_uninit_list);

	pthread_mutex_init(&mod->mutex, NULL);
	pthread_mutex_init(&mod->init_mutex, NULL);
	pthread_mutex_init(&mod->uninit_mutex, NULL);
	pthread_mutex_init(&mod->local_init_mutex, NULL);
	pthread_mutex_init(&mod->local_uninit_mutex, NULL);

	mod->init_handle = tc_init_module_init;
	mod->uninit_handle = tc_init_module_uninit;
	mod->local_init_register = tc_init_module_local_init_register;
	mod->local_uninit_register = tc_init_module_local_uninit_register;
	mod->other_init_register = tc_init_module_init_register;
	mod->other_uninit_register = tc_init_module_uninit_register;
	mod->init_test = tc_init_module_init_test;

	return mod;
}

static void
tc_init_module_list_destroy(
	pthread_mutex_t *mutex,
	struct list_head *head
)
{
	struct list_head *sl = NULL;
	struct tc_init_conf *ic_node = NULL;

	pthread_mutex_lock(mutex);
	sl = head->next;
	while (sl != head) {
		ic_node = tc_list_entry(sl, struct tc_init_conf, node);
		sl = sl->next;
		list_del_init(&ic_node->node);
		TC_FREE(ic_node);
	}
	pthread_mutex_unlock(mutex);
}

void
tc_init_module_destroy(
	struct tc_init_module *mod
)
{
	tc_init_module_list_destroy(&mod->init_mutex, &mod->init_list);
	tc_init_module_list_destroy(&mod->uninit_mutex, &mod->uninit_list);
	tc_init_module_list_destroy(&mod->local_init_mutex, &mod->local_init_list);
	tc_init_module_list_destroy(&mod->local_uninit_mutex, &mod->local_uninit_list);
}

