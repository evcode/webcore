#include "condlock.h"
#include "util.h"

static void cond_clean(CondLock* lock)
{
	debug("Warning: condlock unexpected to exit. Do cleanup\n\n");

	pthread_mutex_unlock(&lock->mutex);

	//pthread_mutex_destroy(&lock->mutex);
	//pthread_cond_destroy(&lock->cond);
}

void condlock_init(CondLock* lock)
{
	memset(lock, 0, sizeof(CondLock));

	pthread_mutex_init(&lock->mutex, NULL);
	pthread_cond_init(&lock->cond, NULL);
	lock->needwait = 1;

	lock->cond_cleanup = cond_clean; // internal cleanup
}

void condlock_set_task(CondLock* lock, void (*cond_met)(void*), void* arg)
{
	pthread_mutex_lock(&lock->mutex);

	lock->cond_routine = cond_met;
	lock->routine_arg = arg;
	lock->routine_oneshot = 1;  // TODO: one shot or NOT?

	pthread_mutex_unlock(&lock->mutex);
}

void condlock_wait_task(CondLock* lock) // TODO: unexpected exit, such as by "cancel"
{
	debug("task %x: enter cond wait\n", pthread_self());
	pthread_cleanup_push(lock->cond_cleanup, lock);

	//pthread_mutex_trylock(lock->mutex);
	pthread_mutex_lock(&lock->mutex);
	debug("task %x: acquired mutex, needwait=%d\n", pthread_self(), lock->needwait);
	while (lock->needwait)
	{
		debug("task %x: continue cond-wait...\n", pthread_self());
		/* in cond_wait:
		1. thread hung, and mutex will be unlocked to wait signal
		2. after signalled, thread can go and LOCK mutex again

		NOTE: it's a Cancel point... */
		pthread_cond_wait(&lock->cond, &lock->mutex);

		// cond met. do my job
		debug("task %x: cond met and needwait=%d. Do my job!!!\n", pthread_self(), lock->needwait);
		if (lock->cond_routine != NULL)
			lock->cond_routine(lock->routine_arg);
		else
			debug("Warning: task %x: no execution routine in condwait\n\n", pthread_self());

		if (lock->routine_oneshot)
			lock->cond_routine = NULL;
	}
	pthread_mutex_unlock(&lock->mutex);

	pthread_cleanup_pop(0);
	debug("task %x: end of cond wait\n", pthread_self());
}

void condlock_notify(CondLock* lock, int needwait)
{
	//pthread_mutex_trylock(lock->mutex);
	pthread_mutex_lock(&lock->mutex);

	lock->needwait = needwait;
	pthread_cond_signal(&lock->cond); // NOTE: the calling can placed after "unlock" also

	pthread_mutex_unlock(&lock->mutex);

	// TODO: if needwait == 0, release the lock???
}