#include "cgicall.h"

CGI_StatusCode cgi_get_statuscode(int err)
{
	switch(err)
	{
		case CGI_NOTIFY_OK:
		break;

		case CGI_NOTIFY_BAD_ENV:
		break;

		case CGI_NOTIFY_CGI_ERR:
		break;

		case CGI_NOTIFY_TIMEOUT:
		beak;

		case CGI_NOTIFY_INVALID_DATA:
		break;

		default:
		break;
	}
}

static char* cginotify[] = {"NOTIFY OK", "BAD ENV VARIANT", "CGI ERROR", "TIME OUT", "INVALID DATA", "UNKNOWN ERROR"};

char* cgi_get_notifyname(int evt)
{
	if (evt >= sizeof(cginotify)/sizeof(cginotify[0]))
		return "unknown-notify";

	return cginotify[evt];
}

static int response_id = -1;
static void (*response_cb)(int, int, char*, int) = NULL;

/* @"msg": as http/non-http data read from cgi to send to browser
*/
void cgi_addlisten(int id, void(*cb)(int id, int err, char* msg, int len))
{
	response_id = id;
	response_cb = cb;

	//fflush(stdout); // TODO: maybe added in cgi program??? NO NEED HERE
}

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

void cgi_run(char* envp[])
{
	int tocgi[2]; // server channels to cgi
	int fromcgi[2]; // cgi channels to server

    /* Create the full-duplex pipes (now "fromcgi" only should be enough):
    miniweb (w tocgi[1])------------->>(r tocgi[0])   cgi
    miniweb (r fromcgi[0])<<-----------(w fromcgi[1]) cgi
    */
	int to_fd = pipe(tocgi);
	if (to_fd < 0)
	{
		error("Failed to pipe!!\n");
		say_errno();
		return;
	}
	int from_fd = pipe(fromcgi);//pipe2(fromcgi, O_NONBLOCK);// now MACOS has no pipe2, replace to use fcntl
	if (from_fd < 0)
	{
		error("Failed to pipe!!\n");
		say_errno();
		return;
	}

	fcntl(from_fd, F_GETFL, O_NONBLOCK); // "asyn" I/O

	// Execute CGI
	debug("++++++++++++++++++++ cgi calling...\n");

	pid_t newpid = fork();
	if (newpid < 0)
	{
		error("Failed to fork!!\n");
		say_errno();
		return;
	}

	if (newpid == 0) // child: cgi execution
	{
		// redirect stdout of new execution - 
		// it means cgi "printf" will write into fromcgi[1]
		close(STDOUT_FILENO);
		dup2(fromcgi[1], STDOUT_FILENO);
		// and, no need to redirect its stdin, just close it
		close(STDIN_FILENO);
		//dup2(fromcgi[0], STDIN_FILENO);

		/*
			hereafter all STDIN input in Child will be redirected into pipe!!
			hence, printf for Debug is forbidden to avoid garbage info. sent
			, except that's just your expectation
		*/

		char* exec = "cgi/cgi";
		char* argv[] = {"cgi", NULL}; // NOTE: on MACOS "argv" also must be NULL as tail
		int exe = execve(exec, argv, envp);
		if (exe < 0)
		{
			printf(CGI_ERROR); // NOTE: output into pipe - see also below!!
			//error("Failed to execute <%s>!!\n", exec);
			//say_errno();
		}

		// NOTE: cannot reach here
		exit(1);
	}
	else // parent: pipe receive
	{
		#define CGI_RESPONSE_LEN 256
		char* totalread = malloc(CGI_RESPONSE_LEN);
		unsigned int totaloff = 0, n = 1, remaining;
		unsigned int rlen;

		int to = 5, per = 100; // set 5*100 TO to wait "cgi" responding

		/*
			TODO: any optimization can do below?!
			, espeically for malloc, wait-read??????????
			and, duplicate codes: memcpy+realloc - improve it!!!!!
		*/
		totaloff = 0;
		n = 1; //  count of "realloc"

		// Codes below aims to "asyn" I/O as per "O_NONBLOCK" set above
		// TOOD: double review the codes as below!!!!!

		// continous reading until TO
		remaining = n*CGI_RESPONSE_LEN-totaloff;
		while (((rlen = read(fromcgi[0], totalread+totaloff, remaining)) <= 0) && (to))
		{
			usleep(per); // sleep for next try reading
			to --;
		}
		if (rlen > 0)
		{
			totaloff += rlen;
			remaining-= rlen;
		}

		// if just read fully, continue to read until cannnot read anymore
		while (0 == remaining)
		{
			n ++;
			totalread = realloc(totalread, n*CGI_RESPONSE_LEN);
			debug("realloc more=%d\n", n*CGI_RESPONSE_LEN);

			remaining = n*CGI_RESPONSE_LEN-totaloff;
			rlen = read(fromcgi[0], totalread+totaloff, remaining);
			if (rlen > 0)
			{
				totaloff += rlen;
				remaining-= rlen;
			}

			// TODO: any sleep here??? to release timeslice
		}

		// reading done, and respond it
		if (response_cb)
		{
			if (totaloff > 0)
			{
				if (str_startwith(CGI_ERROR))
					response_cb(response_id, CGI_NOTIFY_CGI_ERR, totalread, totaloff);
				else
					response_cb(response_id, CGI_NOTIFY_OK, totalread, totaloff); // TODO: if cb inside is BLOCKING, efficiency is low!! Improve
			}
			else if (to == 0)
			{
				response_cb(response_id, CGI_NOTIFY_TIMEOUT, NULL, 0);
			}
			else if (totaloff == 0)
			{
				response_cb(response_id, CGI_NOTIFY_INVALID_DATA, NULL, 0);
			}
		}

		free(totalread);

		// TODO: waitpid??
	}

	// TODO: such close pipe is OK??? how about in forked process??
	close(to_fd);
	close(from_fd);

	debug("-------------------- cgi completed!!\n\n");
}

void cgi_request(const char* msg, int msglen)
{
	int i;
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
		cgi_run(envlist); // TODO: now it directly cb "respond", so still "syn" call! Improve!!
	}
	else
	{
		if (response_cb)
			response_cb(response_id, CGI_NOTIFY_BAD_ENV, NULL, 0);
	}
}