
#include "lib.h"
#include "netif.h"
#include "ether.h"
#include "ip.h"
#include "arp.h"
#include "route.h"

extern void test_shell(char *);

static pthread_t threads[2];

typedef void *(*pfunc_t)(void *);

int newthread(pfunc_t thread_func)
{
	pthread_t tid;
	if (pthread_create(&tid, NULL, thread_func, NULL))
		perrx("pthread_create");
	return tid;
}

void net_stack_init(void)
{
	netdev_init();
	arp_cache_init();
	rt_init();
}

void net_stack_run(void)
{
	/* create timer thread */
	threads[0] = newthread((pfunc_t)net_timer);
	dbg("net_timer thread 0: net_timer");
	/* create netdev thread */
	threads[1] = newthread((pfunc_t)netdev_interrupt);
	dbg("netdev_interrupt thread 1: netdev_interrupt");
	/* net shell runs! */
	test_shell(NULL);
}

void net_stack_exit(void)
{
	if (pthread_cancel(threads[0]))
		perror("kill child 0");
	if (pthread_cancel(threads[1]))
		perror("kill child 1");
	netdev_exit();
}

int main(int argc, char **argv)
{
	net_stack_init();
	net_stack_run();
	net_stack_exit();

	return 0;
}
