#include <stdlib.h> // for "RAND_MAX" also

#include "util.h"

// *******************************************************************
#include <time.h>

char* curr_timestr()
{	
	time_t time_sec;
	if (time(&time_sec) == -1)
		return "[no-time-info]";

	return ctime(&time_sec);
}

int give_random(int loop)
{
	static char srand_did = 0;

	if (!srand_did)
	{
		srand_did = 1; // TODO: one call, out of rand() loop below, is enough
		srand(time(NULL));
	}

	return (1+loop*(rand()/(RAND_MAX+1.0))); // NOTE rand() just gives [0, RAND_MAX]
}

void append_str(char* dst, ...)
{// TODO: fix me
	//va_list argv;
	//va_start(argv, dst);
	//va_arg(argv, int);
	//va_end(argv);
}

// *******************************************************************
#define TRNAS_ENV_MAX 128

static int envnum = 0;
static char* envs[TRNAS_ENV_MAX]={NULL}; // NOTE: make sure last item is NULL

void env_add(char* key, int len1, char* value, int len2)
{
	if (envnum == TRNAS_ENV_MAX-1) // the last one is reserved to NULL???
	{
		error("Reached the max number of environment variants!!");
		return;
	}

	char* env = malloc(len1+len2);
	memcpy(env, key, len1);
	env[len1-1]='='; // replace '/0' at end of "key"
	memcpy(env+len1, value, len2);

	envs[envnum] = env;
	envnum ++;
}
void env_dump()
{
	debug("Env. variants:\n");
	int i;
	for (i = 0; i < envnum; i ++)
	{
		printf("%s ", envs[i]);
	}
	printf("\n");
}
// TODO: release all

// *******************************************************************
#include <pthread.h>

void new_task(void(*func)(void*), void* arg)
{
	pthread_t pthread;
	if (pthread_create(&pthread, NULL, func, arg) != 0) // TODO: "pthread" retrieval and release
		error("Task creation failed");
}

#include <signal.h>
/*
typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
:
     No    Name         Default Action       Description
     1     SIGHUP       terminate process    terminal line hangup
     2     SIGINT       terminate process    interrupt program
     3     SIGQUIT      create core image    quit program
     4     SIGILL       create core image    illegal instruction
     5     SIGTRAP      create core image    trace trap
     6     SIGABRT      create core image    abort program (formerly SIGIOT)
     7     SIGEMT       create core image    emulate instruction executed
     8     SIGFPE       create core image    floating-point exception
     9     SIGKILL      terminate process    kill program
     10    SIGBUS       create core image    bus error
     11    SIGSEGV      create core image    segmentation violation
     12    SIGSYS       create core image    non-existent system call invoked
     13    SIGPIPE      terminate process    write on a pipe with no reader
     14    SIGALRM      terminate process    real-time timer expired
     15    SIGTERM      terminate process    software termination signal
     16    SIGURG       discard signal       urgent condition present on socket
     17    SIGSTOP      stop process         stop (cannot be caught or ignored)
     18    SIGTSTP      stop process         stop signal generated from keyboard
     19    SIGCONT      discard signal       continue after stop
     20    SIGCHLD      discard signal       child status has changed
     21    SIGTTIN      stop process         background read attempted from control terminal
     22    SIGTTOU      stop process         background write attempted to control terminal
     23    SIGIO        discard signal       I/O is possible on a descriptor (see fcntl(2))
     24    SIGXCPU      terminate process    cpu time limit exceeded (see setrlimit(2))
     25    SIGXFSZ      terminate process    file size limit exceeded (see setrlimit(2))
     26    SIGVTALRM    terminate process    virtual time alarm (see setitimer(2))
     27    SIGPROF      terminate process    profiling timer alarm (see setitimer(2))
     28    SIGWINCH     discard signal       Window size change
     29    SIGINFO      discard signal       status request from keyboard
     30    SIGUSR1      terminate process    User defined signal 1
     31    SIGUSR2      terminate process    User defined signal 2
*/
void sig_routine(int signum)
{
	switch(signum)
	{
		case SIGCHLD:
			debug("** SIGCHLD captured\n");
		break;

		case SIGINT: // ctrl+c
			debug("** SIGINT captured\n");
		break;

		case SIGTSTP: //ctrl+z
			debug("** SIGTSTP captured\n");
		break;

		case SIGTERM: // "kill"
			debug("** SIGTERM captured\n");
		break;

		default:
			debug("** The signal %d captured\n", signum);
		break;
	}

	exit(0);
}