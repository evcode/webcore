#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trans.h"
#include "util.h"
#include "taskpool.h"
#include "vheap.h"

extern void on_trans_notified(TransEvent evt, TransConn* conn, char* s, unsigned int l);

static int system_le()
{
	int a = 1;

	/* in HEX,
	BE is 0000_0001
	LE is 0100_0000,
	then return first byte */

	return (*(char*)&a);
}

#include <signal.h>
static void sig_routine2(int sig)
{
	if (sig == SIGCHLD)
	{
		debug("* SIGCHLD captured\n");

		int rtn;
		do
		{
			rtn = waitpid(-1, NULL, WNOHANG); // TODO: handle Status, following a better usage

			debug("* --> Process %d done\n", rtn);
		} while (rtn == 0);

		debug("* All Zombines processed\n");
	}
}

static void install_signals()
{
	int signum = 0;
	for (signum = 0; signum <= 30; signum ++) // a test - actually no need install so many
	{
		//signal(signum, sig_routine); // TODO: replace with "sigaction()"
	}

	signal(SIGCHLD, sig_routine2); // TODO: replace with "sigaction()"
}

int main (int argc, char* argv[], char* envp[])
{
#if 0 // http analysis TEST
	{
		char* str = "GET /index HTTP/1.1\r\nHost: 127.0.0.1:9000\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36\r\n";
		int len = strlen(str)+1;
		char * s = malloc(len);
		memcpy(s, str, len);
		request(s, len);

		return 1;
	}
#endif

#if 0 // taskpool unit test
	{
		test_taskpool();
		return 1;
	}
#endif

	if (system_le())
		debug("System Little-endian\n");

	//debug("argc=%d, argv[0]=%s\n", argc, argv[0]);
	int i = 0;
	for (i = 0; i < argc; i ++)
		printf("%s ", argv[i]);
	for (i = 0; envp[i] != NULL; i ++)
		printf("%s ", envp[i]);
	printf("\n\n");

	// TODO: bug fixing - ENABLE_TASKPOOL: signal cannot work well with multi-threads:(
	//install_signals();
	debugger_enable();

	vheap_init(1);
#ifdef ENABLE_TASKPOOL
	taskpool_create(10);
#endif

	char * dst = "ANY";//"192.168.100.218:3000" "ANY" // TODO: read from cmdline

#ifdef TEST_SEND_STRESS
	if (argc>1)
		send_stress(dst); // simulate to send to Server
#endif

	// processing main-body
	Transaction* p = trans_create((argc>1), dst);

	trans_addlisten(p, on_trans_notified);
	trans_start(p);

	trans_destory(p);

	// cannot reach here

    return 0;
}
