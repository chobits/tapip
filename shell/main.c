#include "lib.h"
#include "netif.h"
#include "ether.h"
#include "ip.h"
#include "arp.h"
#include "route.h"
#include "socket.h"

extern void shell_master(char *);
extern void *shell_worker(void *);
extern void shell_init(void);

/*
 * 0 timer
 * 1 core stack
 * 2 shell worker
 */
pthread_t threads[3];

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
	socket_init();
	shell_init();
}

void net_stack_run(void)
{
	/* create timer thread */
	threads[0] = newthread((pfunc_t)net_timer);
	dbg("thread 0: net_timer");
	/* create netdev thread */
	threads[1] = newthread((pfunc_t)netdev_interrupt);
	dbg("thread 1: netdev_interrupt");
	/* shell worker thread */
	threads[2] = newthread((pfunc_t)shell_worker);
	dbg("thread 2: shell worker");
	/* net shell runs! */
	shell_master(NULL);
}

void net_stack_exit(void)
{
	if (pthread_cancel(threads[0]))
		perror("kill child 0");
	if (pthread_cancel(threads[1]))
		perror("kill child 1");
	/* shell work will be killed by shell master */
	if (pthread_join(threads[2], NULL))
		perror("kill child 2");
	netdev_exit();
}

int main(int argc, char **argv)
{
	net_stack_init();
	net_stack_run();
	net_stack_exit();
	dbg("wait system exit");
	/* FIXME: release all alloced resources */
	return 0;
}
