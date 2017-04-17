#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

// *******************************************************************

/*
       #include <unistd.h>
       int pipe(int pipefd[2]);

       #define _GNU_SOURCE             // See feature_test_macros(7)
       #include <fcntl.h>              // Obtain O_* constant definitions
       #include <unistd.h>
       int pipe2(int pipefd[2], int flags);

@flags:
       O_NONBLOCK  Set the O_NONBLOCK file status flag on the two new open  file  descriptions.   Using  this  flag
                   saves extra calls to fcntl(2) to achieve the same result.

       O_CLOEXEC   Set the close-on-exec (FD_CLOEXEC) flag on the two new file descriptors.  See the description of
                   the same flag in open(2) for reasons why this may be useful.

----------------------------------------------------

       #include <unistd.h>
       int dup(int oldfd);
       int dup2(int oldfd, int newfd);

       #define _GNU_SOURCE             // See feature_test_macros(7) 
       #include <fcntl.h>              // Obtain O_* constant definitions 
       #include <unistd.h>
       int dup3(int oldfd, int newfd, int flags);
*/

#define _GNU_SOURCE // mandatory???
#include <fcntl.h> // + pipe2, and O_*
#include <unistd.h> // for pipe(), dup, etc
					// + STDIN_FILENO、STDOUT_FILENO and STDERR_FILENO

void run_cgi(char* envp[])
{
	debug("++++++++++++++++++++ Start to run cgi...\n");
#define PIPE_READ  0
#define PIPE_WRITE 1

	int tocgi[2]; // server channels to cgi
	int fromcgi[2]; // cgi channels to server

    /* Create the full-duplex pipes:
    miniWeb (w tocgi[1])------------->>(r tocgi[0])   cgi
    miniWeb (r fromcgi[0])<<-----------(w fromcgi[1]) cgi
    */
	int to_fd = pipe(tocgi);
	if (to_fd < 0)
	{
		error("Failed to pipe!!\n");
		say_errno();
		return;
	}
	int from_fd = pipe(fromcgi);//pipe2(fromcgi, O_NONBLOCK);
	if (from_fd < 0)
	{
		error("Failed to pipe!!\n");
		say_errno();
		return;
	}

	// Execute CGI
	pid_t newpid = fork();
	if (newpid == 0)
	{
		// redirect stdout of new execution - 
		// it means cgi "printf" will write into fromcgi[1]
		close(STDOUT_FILENO);
		dup2(fromcgi[1], STDOUT_FILENO);
		// and, no need to redirect its stdin, just close it
		close(STDIN_FILENO);
		//dup2(fromcgi[0], STDIN_FILENO);

		char* exec = "cgi/cgi";
		char* argv[] = {"cgi"};
		int exe = execve(exec, argv, envp);
		if (exe < 0)
		{
			error("Failed to execute <%s>!!\n", exec);
			say_errno();
			return;
		}

		// NOTE: cannot reach here
	}
	else if (newpid < 0)
	{
		error("Failed to fork!!\n");
		say_errno();
		return;
	}
	else
	{
		// to read from cgi, we need check fromcgi[0]
		while(1)
		{
			char buff[128];
//			debug("Pipe reading...\n");
			int rlen = read(fromcgi[0], buff, sizeof(buff));
			debug("Pipe %d bytes read:\n", rlen);

//			debug("...%d bytes read\n", rlen);
			int i = 0;
			for (i = 0; i < rlen; i ++)
				printf("%c", buff[i]);
			printf("(end)\n");
		}

		// TODO: waitpid??
	}

	// TODO: such close pipe is OK??? how about in forked process??
	close(to_fd);
	close(from_fd);

	debug("-------------------- cgi completed!!\n\n");
}

// *******************************************************************
extern char** envlist_init(); // NOTE: (on MACOS??) it's MANDATORY;otherwise, it will crash when to read it here. WHY????????????

void submit_msg(const char* msg, int msglen) // TODO: design a "listner" mechanism??? to notify, such as HTTP coming event
{
	debug("A new message constructed:\n");
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

				envlist_add(envstr);

				free(envstr);
				envstr = NULL;
			}

			envstart = i + 1; // substring to parse starting from next byte
		}
	}

	envlist_dump(envlist, envlist_num());

	// run cgi
	run_cgi(envlist);
}

// *******************************************************************
#include "trans.h"

#define TRANS_RECVBUF_SIZE 120
#define TRANS_SENDBUF_SIZE (TRANS_RECVBUF_SIZE+8)

void trans_recvtask(int conn_fd) // TODO: transfer a Trans struct not just a "fd"???
{
	char transrecv[TRANS_RECVBUF_SIZE];
	int bufflen = sizeof(transrecv), len;

#define TOTOAL_MSG_LEN 1500
	char totalmsg[TOTOAL_MSG_LEN]; // TODO: replace with realloc(), or direclty apply it to "transrecv" - char* transrecv;
	int totalrecv;

	debug("Start to receive at trans=%d\n", conn_fd);

	//while (1) // NOTE: one-time receive for one connetion(accepted)
	{
#ifdef TEST_SEND_STRESS
		sleep(give_random(10));
#endif // for Stress not sleep always - i think it simulates an actual env.

		totalrecv = 0;

		do
		{
			len = recv(conn_fd, transrecv, bufflen, 0);// TODO: flags
			if (len == 0) // return 0, for TCP, means the peer has closed its half side of the connection
			{
				debug("End receive task, len=%d\n", len);

				if (totalrecv > 0) // ever received - in case of that "totalrecv % bufflen == 0"
					submit_msg(totalmsg, totalrecv); // Submit received bytes
				return;
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
		} while (len == bufflen); // MOSTLY there're still bytes remaind in kernel

		// Submit received bytes
		submit_msg(totalmsg, totalrecv);
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
		submit_msg(s, len);

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

/* --------------------------------------------------
The core logical processing:

   main
    |
->accept
|   |
|   |fork
|___|  \
        \
        recv
         |
       submit
         |
         |fork
         |  \
         |   \
         |    \----"cgi"
         |           |
     ->read <-pipe- stdin
     |   |
     |___|

-------------------------------------------------- */
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
			new_task(trans_recvtask, trans.conn_end->conn_fd);
#else
			int pid = fork();
			if (pid == 0)
			{
				debug("After fork, Server child pid=%d (recv) continues\n", getpid());
				trans_recvtask(trans.conn_end->conn_fd);
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