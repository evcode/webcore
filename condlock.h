#ifndef CONDLOCK_H
#define CONDLOCK_H

#include <pthread.h>

typedef struct cond_lock_t
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int needwait;

	void (*cond_cleanup)(struct cond_lock_t*);

	int routine_oneshot;
	void* routine_arg;
	void* (*cond_routine)(void *);
} CondLock;

void condlock_init(CondLock* lock);
void condlock_set_task(CondLock* lock, void (*cond_met)(void*), void* arg);
void condlock_wait_task(CondLock* lock);
void condlock_notify(CondLock* lock, int needwait);
#endif