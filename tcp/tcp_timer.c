#include "tcp.h"
#include "lib.h"

static struct tcp_timer_head timewait;
/* static struct tcp_timer_head retrans; */

/* TIME-WAIT TIMEOUT */
void tcp_timewait_timer(int delta)
{
	struct tcp_timer *t, *next, **pprev;
	struct tcp_sock *tsk;
	for (pprev = &timewait.next, t = timewait.next; t; t = next) {
		next = t->next;		/* for safe deletion */
		t->next = NULL;
		t->timeout -= delta;
		if (t->timeout <= 0) {
			/* TIME-WAIT expires: tcb deletion */
			tsk = timewait2tsk(t);
			if (!tsk->parent)
				tcp_unbhash(tsk);
			tcp_unhash(&tsk->sk);
			tcp_set_state(tsk, TCP_CLOSED);
			free_sock(&tsk->sk);
			*pprev = next;
		} else {
			pprev = &t->next;
		}
	}
}

void tcp_set_timewait_timer(struct tcp_sock *tsk)
{
	tcp_set_state(tsk, TCP_TIME_WAIT);
	/* FIXME: lock */
	tsk->timewait.timeout = TCP_TIMEWAIT_TIMEOUT;
	tsk->timewait.next = timewait.next;
	timewait.next = &tsk->timewait;
	/* reference for TIME-WAIT TIMEOUT releasing */
	get_tcp_sock(tsk);
}

void tcp_timer(void)
{
	unsigned int i = 0;
	/* init */
	timewait.next = NULL;
	while (1) {
		usleep(TCP_TIMER_DELTA);
		i++;
		/* 1 second timeout for TIME-WAIT timer */
		if ((i % (1000000 / TCP_TIMER_DELTA)) == 0)
			tcp_timewait_timer(1000000);
	}
}
