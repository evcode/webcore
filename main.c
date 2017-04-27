#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trans.h"
#include "util.h"
extern void on_trans_notified(TransEvent, TransConn*, char*, unsigned int);

static int system_le()
{
	int a = 1;

	/* in HEX,
	BE is 0000_0001
	LE is 0100_0000,
	then return first byte */

	return (*(char*)&a);
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

	if (system_le())
		debug("System Little-endian\n");

	//debug("argc=%d, argv[0]=%s\n", argc, argv[0]);
	int i = 0;
	for (i = 0; i < argc; i ++)
		printf("%s ", argv[i]);
	for (i = 0; envp[i] != NULL; i ++)
		printf("%s ", envp[i]);
	printf("\n\n");

	char * dst = "ANY";//"192.168.100.218:3000" "ANY" // TODO: read from cmdline

	int signum = 0;
	for (signum = 0; signum <= 30; signum ++) // a test - actually no need install so many
	{
		//signal(signum, sig_routine); // TODO: replace with "sigaction()"
	}

	// processing main-body
	Transaction* p = trans_create((argc>1), dst);

	trans_addlisten(p, on_trans_notified);
	trans_start(p);

	trans_destory(p);

	// cannot reach here

    return 0;
}