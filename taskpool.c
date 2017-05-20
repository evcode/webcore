#include "taskpool.h"

/*
SYNOPSIS
     #include <pthread.h>

     The POSIX thread functions are summarized in this section in the following groups:

           o   Thread Routines
           o   Attribute Object Routines
           o   Mutex Routines
           o   Condition Variable Routines
           o   Read/Write Lock Routines
           o   Per-Thread Context Routines
           o   Cleanup Routines

     int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
             void *(*start_routine)(void *), void *arg)
             Creates a new thread of execution.

     int pthread_cancel(pthread_t thread)
             Cancels execution of a thread.

     int pthread_detach(pthread_t thread)
             Marks a thread for deletion.

     int pthread_equal(pthread_t t1, pthread_t t2)
             Compares two thread IDs.

     void pthread_exit(void *value_ptr)
             Terminates the calling thread.

     int pthread_join(pthread_t thread, void **value_ptr)
             Causes the calling thread to wait for the termination of the specified thread.

     int pthread_kill(pthread_t thread, int sig)
             Delivers a signal to a specified thread.

     int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
             Calls an initialization routine once.

     pthread_t pthread_self(void)
             Returns the thread ID of the calling thread.
------------------------------------------------------------------------ Mutex Routines
     int pthread_mutex_destroy(pthread_mutex_t *mutex)
             Destroy a mutex.

     int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
             Initialize a mutex with specified attributes.

     int pthread_mutex_lock(pthread_mutex_t *mutex)
             Lock a mutex and block until it becomes available.

     int pthread_mutex_trylock(pthread_mutex_t *mutex)
             Try to lock a mutex, but do not block if the mutex is locked by another thread, includ-
             ing the current thread.

     int pthread_mutex_unlock(pthread_mutex_t *mutex)
             Unlock a mutex.
------------------------------------------------------------------------ Condition Variable Routines
     int pthread_cond_destroy(pthread_cond_t *cond)
             Destroy a condition variable.

     int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
             Initialize a condition variable with specified attributes.

     int pthread_cond_signal(pthread_cond_t *cond)
             Unblock at least one of the threads blocked on the specified condition variable.

     int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
             const struct timespec *abstime)
             Unlock the specified mutex, wait no longer than the specified time for a condition, and
             then relock the mutex.

     int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *mutex)
             Unlock the specified mutex, wait for a condition, and relock the mutex.
...
*/

/*
	Internal definitions
*/
#include "condlock.h"

#define TASKPOOL_CORE_CAPABILITY 8 // always "core_capability" tasks is waiting...
#define TASKPOOL_MAX_CAPABILITY (TASKPOOL_CORE_CAPABILITY+20)

typedef struct task_worker
{
	void (*w_routine)(void*);
	void* w_arg;

	struct task_worker *next_worker;
} TaskWorker;

typedef enum
{
	TASK_STATE_UNINIT = 0,
	TASK_STATE_IDLE,
	TASK_STATE_BUSY,
} TASK_STATE;

typedef struct
{
	TASK_STATE t_state;
	//int32 idle_timeout; // <=0: infinite, in sec

	pthread_t t_id;
	CondLock t_cond;

	TaskWorker *t_worker;
} TaskProcess;

typedef struct
{
	// thread pool
	TaskProcess task_list[TASKPOOL_MAX_CAPABILITY];
	uint32 task_count; // created task(thread) cnt

	// waiting queue - FIFO
	pthread_mutex_t queue_mutex;
	TaskWorker *worker_queue;
	TaskWorker *worker_tail;
	uint32 worker_count;
} TaskPool;

/*
	Local variables
*/
static TaskPool instance;

/*
	Functions implementation
*/
static void _push_worker_queue(void(*fn)(void*), void* arg)
{
	TaskWorker* worker = malloc(sizeof(TaskWorker));

	memset(worker, 0 , sizeof(TaskWorker));
	worker->w_routine = fn;
	worker->w_arg     = arg;

	pthread_mutex_lock(&instance.queue_mutex);
	if (instance.worker_queue == NULL)
	{
		instance.worker_queue = worker;
	}
	else
	{
		if (instance.worker_tail == NULL)
			error("fatal error in worker queue\n");
		else
			instance.worker_tail->next_worker = worker;
	}
	instance.worker_tail = worker;

	//
	instance.worker_count ++;
	debug("@--> pushed a new into worker-queue, total=%d\n", instance.worker_count);
	pthread_mutex_unlock(&instance.queue_mutex);
}

static BOOL _pop_worker_queue(TaskWorker* work)
{
	TaskWorker* pop = NULL;

	pthread_mutex_lock(&instance.queue_mutex);
	if (instance.worker_queue != NULL)
	{
		pop = instance.worker_queue;

		instance.worker_queue = pop->next_worker;
		instance.worker_count --;
		debug("@--< one pop from worker-queue..., and %d tasks remained\n", instance.worker_count);
	}
    pthread_mutex_unlock(&instance.queue_mutex);

    if (pop != NULL)
    {
		memcpy(work, pop, sizeof(TaskWorker));
        
		free(pop);
		pop = NULL;
		return TRUE;
    }
	return FALSE;
}

static void _taskpool_routine(TaskProcess* task)
{
	while (1) // TODO: lifecycle flag
	{
		TaskWorker worker;
		if (_pop_worker_queue(&worker))
		{
			if (worker.w_routine != NULL)
				worker.w_routine(worker.w_arg);
		}
		else
		{
			usleep(300*1000); // TODO: for debug only- the busy-waiting here
		}

#if 0
		pthread_mutex_lock(&task->t_cond.mutex);
		task->t_state = TASK_STATE_IDLE;
		while (task->t_worker == NULL)
		{
			pthread_cond_wait(&task->t_cond.cond, &task->t_cond.mutex);
		}
		task->t_state = TASK_STATE_BUSY;
		pthread_mutex_unlock(&task->t_cond.mutex);

		// do working
		if (task->t_worker->w_routine != NULL)
		{
			task->t_worker->w_routine(task->t_worker->w_arg);

			// the execution is ONESHOT
			task->t_worker->w_routine = NULL;
			task->t_worker->w_arg     = NULL;
		}

		// auto pop to execute the task unil the end
		TaskWorker worker;
		while (_pop_worker_queue(&worker))
		{
			if (worker.w_routine != NULL)
				worker.w_routine(worker.w_arg);
		}
#endif
	}

	pthread_detach(pthread_self());
}

static BOOL _create_taskproecss(TaskProcess *task)
{
	memset(task, 0, sizeof(TaskProcess));

	condlock_init(&task->t_cond);

	if (pthread_create(&task->t_id, NULL, _taskpool_routine, task) != 0)
	{
		error("Task creation failed\n");
		return FALSE;
	}

	return TRUE;
}

TaskPoolHandle taskpool_create(int initcount)
{
	memset(&instance, 0, sizeof(TaskPool));
    
	// waiting queue
	pthread_mutex_init(&instance.queue_mutex, NULL);

	// thread pool
	int i;
	for (i = 0; i < TASKPOOL_CORE_CAPABILITY; i ++)
	{
		_create_taskproecss(&instance.task_list[i]);
	}
	instance.task_count = i;

	return &instance;
}

BOOL taskpool_request(TaskPoolHandle handle, void(*fn)(void*), void* arg)
{
	_push_worker_queue(fn, arg);

	return TRUE;
}