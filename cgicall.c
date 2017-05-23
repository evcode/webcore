#include "cgicall.h"
#include "envlist.h"
#include "taskpool.h"

static CGI_StatusCode http_statuslist[] =
{
	/*
	http://www.baidu.com/link?url=sjNCIH7jFNEVlq8lA-vPCkfJJ-qyAGLwC6RqKYUs6srxCCna9khV0eb_qpGjB4gSr46dZj9WTPc07w5M2xqCxZE5LD_KFDVcmJUkxIih1eEzkIf1QNRI_S_sF6P9SuTD&wd=&eqid=ebc44488000276c70000000458f9b21e
	*/

	// TODO: take the first one as DEFAULT
	// Server error
	{500, "Internal Server Error", ""},

	// Message
	{100, "Continue", ""},

	// Success
	{200, "OK", ""},
	{201, "Created", ""},
	{202, "Accepted", ""},
	{204, "No Content", ""},
	{205, "Reset Content", ""},

	// Redirection
	{302, "Move temporarily", ""},
	{303, "See Other", ""},
	{305, "Use Proxy", ""},

	// Client request error
	{400, "Bad Request", ""},
	{401, "Unauthorized", ""},
	{403, "Forbidden", ""},
	{404, "Not Found", ""},
	{405, "Method Not Allowed", ""},
	{406, "Not Acceptable", ""},
	{408, "Request Timeout", ""},
	{413, "Request Entity Too Large", ""},
	{414, "Request-URI Too Long", ""},
	{415, "Unsupported Media Type", ""},

	// Server error
	{501, "Not Implemented", ""},
	{502, "Bad Gateway", ""},
	{503, "Service Unavailable", ""},
	{504, "Gateway Timeout", ""},
	{505, "HTTP Version Not Supported", ""},
	{507, "Insufficient Storage", ""}
};

CGI_StatusCode cgi_map_httpstatus(int err)
{
	int sta;

	if ((err >= 100) && (err <= 600))
	{
		sta = err;
	}
	else
	{
	switch(err) // internal-error mapping to statuscode
	{
		case CGI_NOTIFY_OK:
			sta = 200;
		break;

		case CGI_NOTIFY_BAD_ENV:
		case CGI_NOTIFY_BAD_REQUEST:
			sta = 400;
		break;

		case CGI_NOTIFY_TOOLONG_REQUEST:
			sta = 414;
		break;

		case CGI_NOTIFY_CGI_ERR:
		case CGI_NOTIFY_INVALID_DATA:
			sta = 500;
		break;
		case CGI_NOTIFY_TIMEOUT:
			sta = 504;
		break;

		default:
			sta = 501;
		break;
	}
	}

	// TODO: IMPROVE IT!! current mapping impl is poor!! a O(1) algorithm???
	int i;
	for (i = 0; i < sizeof(http_statuslist)/sizeof(http_statuslist[0]); i ++)
	{
		if (sta == http_statuslist[i].sta_code)
			return http_statuslist[i];
	}

	// TOOD: 1st one is DEFAULT (actually cannot reach here) - UGLY coding!!!!!
	return http_statuslist[0];
}

char* cgi_get_notifyname(int evt)
{
	switch(evt)
	{
		case CGI_NOTIFY_OK:
			return "NOTIFY OK";
		case CGI_NOTIFY_BAD_ENV:
			return "BAD ENV VARIANT";
		case CGI_NOTIFY_CGI_ERR:
			return "CGI PROGAM ERROR";
		case CGI_NOTIFY_BAD_REQUEST:
			return "BAD REQUEST";
		case CGI_NOTIFY_TOOLONG_REQUEST:
			return "TOO LONG REQUEST";
		case CGI_NOTIFY_METHOD_NOT_SUPPORT:
			return "METHOD NOT SUPPORT";
		case CGI_NOTIFY_TIMEOUT:
			return "RESPOND TIME OUT";
		case CGI_NOTIFY_INVALID_DATA:
			return "INVALID DATA";
	}
	// all are (local) internal-errors, http-statuscode cannot be identified
	return "<unknown cgi event>";
}

static CgiListenerList listenlist;

static void _cgilisten_init()
{
	static BOOL is = FALSE;

	if (!is)
	{
		memset(&listenlist, 0, sizeof(CgiListenerList));

		pthread_mutex_init(&listenlist.list_mutex, NULL);
		is = TRUE;
	}
}

CgiListener* cgi_addlisten(void* id, CgiListen_CB cb)
{
	CgiListener* l = malloc(sizeof(CgiListener));
	memset(l, 0, sizeof(CgiListener));
	l->listen_id = id;
	l->listen_cb = cb;

	//
	pthread_mutex_lock(&listenlist.list_mutex);
	_cgilisten_init();

	if (listenlist.list == NULL)
		listenlist.list = l;
	else
	{
		CgiListener* curr = listenlist.list;
		while (curr->next_listen != NULL) // TODO: it's O(n) to find tail:(
		{
			curr = curr->next_listen;
		}
		curr->next_listen = l;
	}

	listenlist.list_num ++;
	pthread_mutex_unlock(&listenlist.list_mutex);

	return l;
}

void cgi_rmlisten(CgiListener* l)
{
	pthread_mutex_lock(&listenlist.list_mutex);
	if (listenlist.list == l)
	{
		listenlist.list = l->next_listen;
		listenlist.list_num --;

		free(l);
		l = NULL;
	}
	else
	{
		CgiListener* curr = listenlist.list;
		while (curr->next_listen != NULL) // TODO: it's O(n) to find tail:(
		{
			if (curr->next_listen == l)
			{
				curr->next_listen = l->next_listen;
				listenlist.list_num --;

				free(l);
				l = NULL;
			}

			curr = curr->next_listen;
		}
	}
	pthread_mutex_unlock(&listenlist.list_mutex);
}

static void _cgilisten_cb(CgiListener* l, int evt, char* msg, int len) // TODO: if cb inside is BLOCKING, efficiency is low!! Improve
{
	if ((l == NULL) ||
		(l->listen_id == NULL) || (l->listen_cb == NULL))
		return;

	l->listen_cb(l->listen_id, evt, msg, len);

	// TODO: remove "l" here???
	cgi_rmlisten(l);
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

typedef union
{
	int fds[2];
	struct
	{
		int r; // mapping to fds[0]
		int w; // mapping to fds[1]
	} pair;
} PipeDesc;

static void _run_cgi(CgiListener* l)
{
	// Get envs
	Envlist elist;
	Envlist* envlist = l->metadata;
	memcpy(&elist, envlist, sizeof(Envlist));
	envlist_uninit(envlist); // release the envlist

	char** envp = elist.envs;

    /* Create the full-duplex pipes (now "fromcgi" only should be enough):
    miniweb (w tocgi[1])------------->>(r tocgi[0])   cgi
    miniweb (r fromcgi[0])<<-----------(w fromcgi[1]) cgi
    */
	int tocgi[2]; // server channels to cgi
	int fromcgi[2]; // cgi channels to server

	int from_fd = pipe(fromcgi);//pipe2(fromcgi, O_NONBLOCK);// now MACOS has no pipe2, replace to use fcntl
	if (from_fd < 0)
	{
		error("Failed to pipe!!\n");
		say_errno();
		return;
	}

	// Configure CGI pipe fds
	int rfd = fromcgi[0];
	int flags = fcntl(rfd, F_GETFL, 0);
	flags |= O_NONBLOCK; // set "nonblocking"
	//flags &= ~O_NONBLOCK; // blocking
	fcntl(rfd, F_SETFL, flags);
	debug("CGI pipe status flags=%08x, (O_NONBLOCK:%08x)\n", flags, O_NONBLOCK);

	flags = fcntl(rfd, F_GETFD);
	flags |= FD_CLOEXEC;
	fcntl(rfd, F_SETFD, flags);
	debug("CGI pipe desc flags=%08x, (FD_CLOEXEC:%08x)\n", flags, FD_CLOEXEC);

	// Execute CGI
	debug("++++++++++++++++++++ cgi calling...\n");

	pid_t newpid = fork();
	if (newpid < 0)
	{
		error("Failed to fork!!\n");
		say_errno();
		return;
	}

	// TODO handle cgi_run() error before fork to respond HTTP

	if (newpid == 0) // child: cgi execution
	{
		// redirect stdout of new execution - 
		// it means cgi "printf" will write into fromcgi[1]
		close(STDOUT_FILENO);
		dup2(fromcgi[1], STDOUT_FILENO);
		// and, no need to redirect its stdin, just close it
		close(STDIN_FILENO);
		//dup2(fromcgi[0], STDIN_FILENO);

		/* see also above, close it
			no need Read in Child
		*/
		close(fromcgi[0]);

		/*
			hereafter all STDIN input in Child will be redirected into pipe!!
			hence, printf for Debug is forbidden to avoid garbage info. sent
			, except that's just your expectation
		*/

		char* exec = "cgi/cgi";
		char* argv[] = {"cgi", NULL}; // NOTE: on MACOS "argv" also must be NULL as tail
		int exe = execve(exec, argv, envp);

		// NOTE: cannot reach here; otherwise it's error
		close(fromcgi[1]);

		{
			char cgistr[128]; // NOTE: output into pipe - see also the error-check below!!
			sprintf(cgistr, "%s:%d:%s", CGI_ERROR, CGI_NOTIFY_CGI_ERR, "CGI program is not ready");
			printf(cgistr);
		}
		exit(1);
	}
	else // parent: pipe receive
	{
		debug("CGI forked the process %d\n", newpid);

		/*
			no need Write in Parent
		*/
		close(fromcgi[1]);

		#define CGI_RESPONSE_LEN 256
		char* totalread = malloc(CGI_RESPONSE_LEN);
		unsigned int totaloff = 0, n = 1, remaining;
		int32 rlen; // FxCK! as "return" of read(), it must be "signed"!!!

		int to = 5, per = 100*1000; // totally 500ms TO to wait "cgi" responding

		/*
			TODO: any optimization can do below?!
			, espeically for malloc, wait-read??????????
			and, duplicate codes: memcpy+realloc - improve it!!!!!
		*/
		totaloff = 0;
		n = 1; //  count of "realloc"

		// Codes below aims to "nonblocking" I/O as per "O_NONBLOCK" set above
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
			//debug("realloc more=%d\n", n*CGI_RESPONSE_LEN);
			printf(".");

			remaining = n*CGI_RESPONSE_LEN-totaloff;
			rlen = read(fromcgi[0], totalread+totaloff, remaining);
			if (rlen > 0)
			{
				totaloff += rlen;
				remaining-= rlen;
			}

			// TODO: any sleep here??? to release timeslice
		}
		// end the Read
		close(fromcgi[0]);

		printf("\n"); // end of "." above
		debug("CGI totally %d bytes (to=%d left) received from cgi\n", totaloff, to);

		// reading done, and respond it
		//if (response_cb)
		{
			char* desc;

			// NOTE: Do judge TO firstly due to un"SIGNED" judge!!!!!
			// : "totaloff" is uint32, which can just handle "rlen=-1" as > 0
			if (to == 0)
			{
				desc = "Timeout to wait CGI responding";
				_cgilisten_cb(l, CGI_NOTIFY_TIMEOUT, desc, strlen(desc));
			}
			else if (totaloff > 0) // NOTE: here cannot handle "totaloff" is negative!!!!!
			{
				if (str_startwith(totalread, CGI_ERROR)) // format is "CGI_ERROR:2:CGI is not ready!!"
				{
					char success = 0;
					do
					{
						char* substr = strstr(totalread, ":");
						if (substr == NULL)
							break;
						substr ++; // here error code starts

						char* substr2 = strstr(substr, ":");
						if (substr2 == NULL)
							break;
						substr2 ++; // here error desc starts

						int err_str;
						int len = substr2 - substr;
						char err[8];
						memcpy(err, substr, len);
						err[len]='\0';
						if (sscanf(err, "%d", &err_str) != 1)
							break;

						success = 1;
						_cgilisten_cb(l, err_str, substr2, totaloff-(substr2-totalread));
					} while (0);

					if (!success)
					{
						desc = "Not identified CGI error";
						_cgilisten_cb(l, CGI_NOTIFY_CGI_ERR, desc, strlen(desc));
					}
				}
				else
					_cgilisten_cb(l, CGI_NOTIFY_OK, totalread, totaloff);
			}
			else if (totaloff == 0)
			{
				desc = "Invalid data retrieved from CGI";
				_cgilisten_cb(l, CGI_NOTIFY_INVALID_DATA, desc, strlen(desc));
			}
		}

		free(totalread);

		// TODO: waitpid??
	}

	debug("-------------------- cgi completed!!\n\n");
}

/* Take it for example of the param "msg":
GET / HTTP/1.1
Host: 127.0.0.1:9000
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp;q=0.8
DNT: 1
Accept-Encoding: gzip, deflate, sdch, br
Accept-Language: zh-CN,zh;q=0.8
Cookie: _ga=GA1.1.1160035858.1493825013

Then, translate each of lines one by one, and add into a env. variables list
*/
void cgi_request(CgiListener* l, const char* msg, int msglen)
{
	int i;

	debug("Do request CGI:\n");
	// construct env. variant retrieved from "msg"
	Envlist* envlist = envlist_init();

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

				CGI_NOTIFY notify = envlist_add(envlist, envstr); // TODO: one more param for "envlist"
				if (notify != CGI_NOTIFY_OK)
				{
					_cgilisten_cb(l, notify, envstr, envlen);

					free(envstr);
					return; // release res to exit
				}

				free(envstr);
			}

			envstart = i + 1; // substring to parse starting from next byte
		}
	}

	envlist_dump(envlist);

	// run cgi
	if (envlist->envnum != 0) // 1st one is NULL, means no env variant
	{
		l->metadata = envlist; // TODO: UGLY design - to transmit the param into another task
#ifdef ENABLE_TASKPOOL
		taskpool_request(NULL, _run_cgi, l);
#else
		_run_cgi(l); // TODO: now it directly cb "respond", so still "syn" call! Improve!!
#endif
	}
	else
	{
		char* desc = "No environment variable found";
		_cgilisten_cb(l, CGI_NOTIFY_BAD_ENV, desc, strlen(desc));
	}
}