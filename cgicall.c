#include "util.h"

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

	if (newpid == 0)
	{
		// redirect stdout of new execution - 
		// it means cgi "printf" will write into fromcgi[1]
		close(STDOUT_FILENO);
		dup2(fromcgi[1], STDOUT_FILENO);
		// and, no need to redirect its stdin, just close it
		close(STDIN_FILENO);
		//dup2(fromcgi[0], STDIN_FILENO);

		/*
			from here, all STDIN called below will be sent over pipe!!
			hence, printf should not be called anymore to avoid "waste" info. sent
		*/

		char* exec = "cgi/cgi";
		char* argv[] = {"cgi", NULL}; // NOTE: on MACOS "argv" also must be NULL as tail
		int exe = execve(exec, argv, envp);
		if (exe < 0)
		{
			//error("Failed to execute <%s>!!\n", exec);
			//say_errno();
			return;
		}

		// NOTE: cannot reach here
	}
	else
	{
		// To read from cgi, we need check fromcgi[0]
		char buff[128];
		int bufflen = sizeof(buff);
		int rlen;
		int to = 5, per = 100; // set 5*100 TO to wait "cgi" responding

		// Codes below aims to "asyn" I/O as per "O_NONBLOCK" set above
		while (((rlen = read(fromcgi[0], buff, bufflen)) <= 0) && (to)) // continuus reading until TO
		{
			usleep(per); // sleep for next try reading
			to --;
		}
		// dump
		int i = 0;
		for (i = 0; i < rlen; i ++)
			printf("%c", buff[i]);
		printf("(end)\n");

		while (rlen == bufflen) // if just read fully, continue to read until cannnot read anymore
		{
			rlen = read(fromcgi[0], buff, bufflen);
			// dump
			i = 0;
			for (i = 0; i < rlen; i ++)
				printf("%c", buff[i]);
			printf("(end)\n");

			// TODO: any sleep here??? to release timeslice
		}

		// TODO: waitpid??
	}

	// TODO: such close pipe is OK??? how about in forked process??
	close(to_fd);
	close(from_fd);

	debug("-------------------- cgi completed!!\n\n");
}