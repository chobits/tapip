#ifndef __TCP_TIMER_H
#define __TCP_TIMER_H

struct tcp_sock;

struct tcp_timer_head {
	struct tcp_timer *next;
};

struct tcp_timer {
	struct tcp_timer *next;
	int timeout;		/* micro second */
};

#define timewait2tsk(t) timer2tsk(t, timewait)
#define timer2tsk(t, member) containof(t, struct tcp_sock, member)
#define TCP_TIMER_DELTA		200000		/* 200ms */
#define TCP_MSL			1000000		/* 1sec */
#define TCP_TIMEWAIT_TIMEOUT	(2 * TCP_MSL)	/* 2MSL */

extern void tcp_set_timewait_timer(struct tcp_sock *);

#endif	/* tcp_timer.h */
