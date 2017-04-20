#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trans.h"
#include "util.h"

extern char** envlist_init(); // NOTE: (on MACOS??) it's MANDATORY;otherwise, it will crash when to read it here. WHY????????????

void respond(int fd, int err, char* s, int n)
{
	debug("To respond at fd=%d:\n", fd);
	if (fd < 0) // TODO: check "err"
		return;

	int i;
	for (i = 0; i < n; i ++)
		printf("%c", s[i]);
	printf("(end)\n");

	// status-line
	char* html_field="HTTP/1.1 200 OK\r\n";
	send(fd, html_field, strlen(html_field), 0);

	// respond-header
	html_field="Server: miniweb\r\nContent-Type: text/html; charset=utf-8\r\n";
	send(fd, html_field, strlen(html_field), 0);
	// "CONTENT_LENGTH" not mandatory???

	// content body
	char* html_format = "\r\n"; // content starts here - Mandatory
								// ,for some format, without webpage cannot identify
	send(fd, html_format, strlen(html_format), 0);

	int len = send(fd, s, n, 0); // TODO: flags
	if (len != n)
	{
		error("Failed to send, err=%d\n", len);
		say_errno();
		return;
	}

	debug("pid=%d Server responds %d bytes>>\n", getpid(), len);
}

void request(int fd, const char* msg, int msglen) // TODO: design a "listner" mechanism??? to notify, such as HTTP coming event
{
	debug("To request:\n");
	if ((msg == NULL) || (msglen <= 0))
	{
		error("Bad parameters!!\n");
		return;
	}

	int i;
#if 0 // TEST the entire received bytes
	for (i = 0; i < msglen; i++)
		printf("%c", msg[i]);
	printf("(end)\n");
#endif

	// construct env. variant retrieved from "msg"
	char** envlist = envlist_init();

	int envstart = 0;
	for (i = 0; i < msglen; i++) // Parse the line one by one
	{
		if ((msg[i] == '\r') || (msg[i] == '\n')) // one line ends
		{
			int envlen = i - envstart; // not contains '\0'
			if (envlen > 0) // to avoid that many '\r' or '\n' following together
			{
				char* envstr = malloc(envlen + 1); // TODO: any optimization can do here?? - not malloc always
				memcpy(envstr, msg + envstart, envlen);
				envstr[envlen] = '\0';

				envlist_add(envstr); // TODO: one more param for "envlist"

				free(envstr);
				envstr = NULL;
			}

			envstart = i + 1; // substring to parse starting from next byte
		}
	}

	envlist_dump(envlist, envlist_num());

	// run cgi
	if (envlist[0] != NULL) // 1st one is NULL, means no env variant
	{
		cgi_listen(fd, respond);
		cgi_run(envlist); // TODO: now it directly cb "respond", so still "syn" call! Improve!!
	}
}

// *******************************************************************

#define TRANS_RECVBUF_SIZE 120
#define TRANS_SENDBUF_SIZE (TRANS_RECVBUF_SIZE+8)

void transact(int conn_fd) // TODO: transfer a Trans struct not just a "fd"???
{
	char transrecv[TRANS_RECVBUF_SIZE];
	int bufflen = sizeof(transrecv), len;

#define TOTOAL_MSG_LEN 1500
	char totalmsg[TOTOAL_MSG_LEN]; // TODO: replace with realloc(), or direclty apply it to "transrecv" - char* transrecv;
	int totalrecv;

	debug("Start to receive at trans=%d\n", conn_fd);

	//while (1)
	// NOTE: one-time receive for one connetion(accepted) - "while" just for TEST
	{
#ifdef TEST_SEND_STRESS
		sleep(give_random(10));
#endif // for Stress not sleep always - i think it simulates an actual env.

		totalrecv = 0;

		do
		{
			//debug("Receiving...\n"); // to check the blocking
			len = recv(conn_fd, transrecv, bufflen, 0);// TODO: flags,        TODO: remove "transrecv" - direclty recv into "totalmsg"
			if (len == 0) // return 0, for TCP, means the peer has closed its half side of the connection
			{
				debug("End receive task, len=%d\n", len);

				break; // not "return": perhapse ever received - in case "totalrecv % bufflen == 0"
			}
			else if (len < 0)
			{
				error("Failed to receive, err=%d\n", len);
				say_errno();
				return;
			} // TODO: len > bufflen, possible????

			debug("pid=%d Server receive %d bytes<<\n", getpid(), len);
/*			int i;
			for (i = 0; i < len; i++)
				printf("%c", transrecv[i]);
			//printf("%s", transrecv);
			printf("(end)\n");
*/
			// Bufffering
			if (totalrecv >= TOTOAL_MSG_LEN)
			{
				error("Too large message requested!!\n");
				return;
			}
			memcpy(totalmsg+totalrecv, transrecv, len);
			totalrecv += len;
		} while (len == bufflen); // MOSTLY there're still bytes remained in kernel buff

		// Submit received bytes
		if (totalrecv > 0)
		{
			request(conn_fd, totalmsg, totalrecv);
		}
	}
}

// *******************************************************************
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

/*
	Transaction init
*/
	Transaction trans;
	memset(&trans, 0, sizeof(Transaction));

	opensock(TRANS_TCP, &trans);
	if (argc > 1)
	{
		trans.trans_mode = 1; // non-zero value means Client
//TODO: NOW the code can work!! Go to get local port from cmdline
//		if (bindsock(&trans, "127.0.0.1:9002") !=0 ) // NOTE: +bind() to use the dedicated port
//			return -2;

		if (connectsock(&trans, dst) != 0) // TODO: test the case not on local
			return -2;
	}
	else // server mode
	{
		if (bindsock(&trans, dst) != 0)
			return -2;

		if (listensock(&trans) != 0)
			return -2;
	}

/*
	Here we go
*/
	char transsnd[TRANS_SENDBUF_SIZE];
	int bufflen = sizeof(transsnd), len;
	if (trans.trans_mode) // the transaction is requesting
	{
		while (1)
		{
			memset(transsnd, 'c', sizeof(transsnd));

			sprintf(transsnd, "Client xxx:xx says:"); // TODO: add local IP:port
			strcat(transsnd, curr_timestr());

#ifdef TEST_SEND_STRESS
			debug("Sending...\n"); // to check the blocking
#endif
			len = send(trans.trans_fd, transsnd, bufflen, 0);// TODO: flags
			if (len != bufflen)
			{
				error("Failed to send, err=%d\n", len);
				say_errno();
				return -3;
			}

			debug("pid=%d Client sent %d bytes>>\n", getpid(), len);
/*			int i;
			for (i = 0; i < len; i ++)
				printf("%c", transsnd[i]);
			//printf("%s", transsnd);
			printf("(end)\n");
*/
#ifdef TEST_SEND_STRESS
			// not sleep to keep sending until next blocking
#else
			sleep(give_random(5)); // sleep a random sec from 1 to 5
#endif
		}
	}
	else // standby for next connection
	{
		while (1)
		{
			int err = acceptsock(&trans);
			if (err != 0)
			{
				return -3;
			}

			// Another new task created to receive the requests...
#if 0
			// NOTE: "conn_end->conn_fd" is just the new accepted Client
			new_task(transact, trans.conn_end->conn_fd);
#else
			int pid = fork();
			if (pid == 0)
			{
				/*
					NOTE: in Chind process do close the unnessary "fd"
				*/
				close(trans.trans_fd);

				debug("After fork, Server child pid=%d (recv) continues\n", getpid());
				transact(trans.conn_end->conn_fd);
				debug("--> Server pid=%d recvtask ends\n\n", getpid());

				/* NOTE: child (client) ends, so must exit;
				otherwise the client will loop to next accept(), which not we want */
				closesock(&trans);
				exit(0);
			}
			else if (pid < 0)
			{
				error("Failed to fork to accept!!\n");
				say_errno();
			}
			else
			{
				debug("After fork, Server parent pid=%d (accept) continues\n", getpid());

				// TODO: waitpid??

				// TODO end the zombie
				// TOOD: close any fd, especially client-fd???
			}
#endif
		}
	}

	// TODO: RELEASE the resource before leaving (return or exit)!!!
	closesock(&trans);

    return 0;
}