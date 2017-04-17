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

void str_append(char* dst, ...)
{// TODO: fix me
	//va_list argv;
	//va_start(argv, dst);
	//va_arg(argv, int);
	//va_end(argv);
}

/* Tested:
  s   |   h
--------------
"", 	"abc"
"x", 	"abc"
"a", 	"abc"
"abc",	"abc"
"abcd",	"abc"
*/
BOOL str_startwith(const char* s, const char* h)
{
	if ((strlen(s) <= 0) || (strlen(h) <= 0)) // handle it as invalid input
		return FALSE;

	while (*h != '\0')
	{
		if (*s != *h)
			return FALSE;

		s ++;
		h ++;
	}

	return TRUE;
}

// *******************************************************************
#include <pthread.h>

void new_task(void(*func)(void*), void* arg)
{
	pthread_t pthread;
	if (pthread_create(&pthread, NULL, func, arg) != 0) // TODO: "pthread" retrieval and release
		error("Task creation failed\n");
}

/* for example:
		char* v[] = {"ls", "-al", "-t"};
		new_exec("/bin/ls", v, NULL);

	// Then, while the program runing:
	// argv[0] = "ls"
	// argv[1] = "-al"
	// argv[2] = "-t"
*/
void new_exec(const char* exec, char *const argv[], char *const envp[])
{
	pid_t newpid = fork();
	if (newpid == 0)
	{
		int exe = execve(exec, argv, envp);
		if (exe < 0)
		{
			error("Failed to execute <%s>!!\n", exec);
			say_errno();
		}
	}
	else if (newpid < 0)
	{
		error("Failed to fork!!\n");
		say_errno();
	}
	else
	{
		// TODO: waitpid??
	}
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