#ifndef __WAIT_H
#define __WAIT_H

//#define WAIT_DEBUG

#include "lib.h"
#include "compile.h"
#include <pthread.h>

/* simulating thread blocking(used for _accept(), _read(), _recv()) */
struct wait {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int notified;		/* log whether wait is waken up already */
	int dead;		/* for safe exiting wait state */
	int sleep;
};

/*
 * 1. @notified avoid double calling of wake_up()
 * 2. If wake_up() is called before sleep_on(),
 *    @notified can skip next waiting of sleep_on().
 */
#ifdef WAIT_DEBUG
#define wake_up(w) \
do { \
	dbg("[*]wake_up " #w); \
	_wake_up(w); \
} while (0)
static _inline int _wake_up(struct wait *w)
#else
static _inline int wake_up(struct wait *w)
#endif
{
	pthread_mutex_lock(&w->mutex);
	/* Should we put this code before locking? */
	if (w->dead)
		goto unlock;
	if (!w->notified) {
		w->notified = 1;
		if (w->sleep)
			pthread_cond_signal(&w->cond);
	}
unlock:
	pthread_mutex_unlock(&w->mutex);
	return -(w->dead);
}

#ifdef WAIT_DEBUG
#define sleep_on(w) \
({ int ret;\
	dbg("[+]sleep on " #w); \
	ret = _sleep_on(w); \
	dbg("[-]Be waken up " #w); \
	ret; \
})
static _inline int _sleep_on(struct wait *w)
#else
static _inline int sleep_on(struct wait *w)
#endif
{
	pthread_mutex_lock(&w->mutex);
	if (w->dead)
		goto unlock;
	w->sleep = 1;
	if (!w->notified)
		pthread_cond_wait(&w->cond, &w->mutex);
	w->notified = 0;
	w->sleep = 0;
unlock:
	pthread_mutex_unlock(&w->mutex);
	return -(w->dead);
}

static _inline void wait_init(struct wait *w)
{
	/* XXX: Should it need error checking? */
	pthread_cond_init(&w->cond, NULL);
	pthread_mutex_init(&w->mutex, NULL);
	w->dead = 0;
	w->notified = 0;
	w->sleep = 0;
}

static _inline void wait_exit(struct wait *w)
{
	pthread_mutex_lock(&w->mutex);
	if (w->dead)
		goto unlock;
	w->dead = 1;
	if (w->sleep)
		pthread_cond_broadcast(&w->cond);
unlock:
	pthread_mutex_unlock(&w->mutex);
}

#endif	/* wait.h */
