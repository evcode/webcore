#include <stdlib.h>

#include "trans.h"
#include "util.h"

int format_sockaddr(int type, const char* str, struct sockaddr* s)
{
	struct sockaddr_in *addr = (struct sockaddr_in *)s;
	unsigned int   ip_addr;
	unsigned short ip_port;

	if (strcmp(str, "ANY") == 0)
	{
		ip_port = 9000;
		ip_addr = INADDR_ANY;
	}
	/*else if (strcmp(str, "localhost") == 0) // not necessary - INADDR_ANY is local
	{
		ip_port = 9000;
		ip_addr = inet_addr("127.0.0.1");
	}*/
	else
	{
		char ip_str[32];
	    int nbr = sscanf(str, "%[0-9,.]:%d", ip_str, &ip_port); // NOTE AMAZING sscanf()!!
	    if (nbr != 2)
	    {
	    	error("Failed to format sockaddr by %s\n", str);
	    	return -2;
	    }
	    debug("Get IP str=%s, and port int=%d\n", ip_str, ip_port);

		ip_addr = inet_addr(ip_str);
	}

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family      = AF_INET; // TODO: only IPv4 now
	addr->sin_addr.s_addr = (ip_addr); // TODO: cannot htonl();othwerwise,bind() fails. WHY?????
	addr->sin_port        = htons(ip_port);
	debug("IP %s=%u, and port=%d(networkaddr is %d)\n", inet_ntoa(addr->sin_addr),
		addr->sin_addr.s_addr, ntohs(addr->sin_port), addr->sin_port);

	return 0;
}

void dumpsock(int fd)
{
	int otpvalue;
	socklen_t optlen = sizeof(otpvalue); // the init is mandatory !!!!!

	debug("----------------------------------\n");
	debug("The dump information of socket fd(%d)\n", fd);

	// NOTE: the following get value is "x2" of buffsize SET value. WHY?????
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &otpvalue, &optlen);
	debug("SO_RCVBUF:%d\n", otpvalue);
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &otpvalue, &optlen);
	debug("SO_SNDBUF:%d\n", otpvalue);

	getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &otpvalue, &optlen);
	debug("SO_REUSEADDR:%d\n", otpvalue);

	getsockopt(fd, IPPROTO_IP, TCP_NODELAY, &otpvalue, &optlen);
	debug("TCP_NODELAY:%d\n", otpvalue);
#ifdef MACOS
	debug("TCP_CORK:%s\n", "<not supported on macos>");
#else
	getsockopt(fd, IPPROTO_IP, TCP_CORK, &otpvalue, &optlen);
	debug("TCP_CORK:%d\n", otpvalue);
#endif

	printf("\n");
}

/* BSD System Calls Manual
#include <sys/types.h>
#include <sys/socket.h>
----------------------------------------------------------------------
int socket(int domain, int type, int protol);
           PF_LOCAL        Host-internal protocols, formerly called PF_UNIX,
           PF_UNIX         Host-internal protocols, deprecated, use PF_LOCAL,
           PF_INET         Internet version 4 protocols,
           PF_ROUTE        Internal Routing protocol,
           PF_KEY          Internal key-management function,
           PF_INET6        Internet version 6 protocols,
           PF_SYSTEM       System domain,
           PF_NDRV         Raw access to network device

           SOCK_STREAM
           SOCK_DGRAM
           SOCK_RAW
RETURN VALUES
     A -1 is returned if an error occurs, otherwise the return value is a descriptor referencing the
     socket.
----------------------------------------------------------------------
int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
RETURN VALUES
     Upon successful completion, a value of 0 is returned.  Otherwise, a value of -1 is returned and the
     global integer variable errno is set to indicate the error.
----------------------------------------------------------------------
int listen(int sockfd, int backlog);
RETURN VALUES
     The listen() function returns the value 0 if successful; otherwise the value -1 is returned and the
     global variable errno is set to indicate the error.
----------------------------------------------------------------------
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
RETURN VALUES
     The call returns -1 on error and the global variable errno is set to indicate the error.  If it suc-
     ceeds, it returns a non-negative integer that is a descriptor for the accepted socket.
----------------------------------------------------------------------
int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
RETURN VALUES
     Upon successful completion, a value of 0 is returned.  Otherwise, a value of -1 is returned and the
     global integer variable errno is set to indicate the error.
*/

int opensock(TRANS_TYPE type, Transaction* trans)
{
	int domain, socktype, proto;

	domain = AF_INET; // TODO: now only IPv4 domain supported
	socktype = (type == TRANS_TCP)?SOCK_STREAM:SOCK_DGRAM;
	proto  = 0; // TODO: CHECK how to match in kernel
	debug("Creation of domain=%d, socktype=%d, proto=%d\n", domain, socktype, proto);

	int fd = socket(domain, socktype, proto);
	if (fd < 0)
	{
		error("Failed to create socket, err=%d\n", fd);
		say_errno();
		return -2;
	}

	// Set snd/rcv buffer size
	/* The recv-buffsize is TCP Window initation, but the actual Win size may be renewed 
	by a system (mini?default, WIN=1120 on my Ubuntu) value,
	such as: cat /proc/sys/net/ipv4/tcp_*mem ??? */
	int optvalue = 1000;
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optvalue, sizeof(optvalue));
	optvalue = 500;
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optvalue, sizeof(optvalue));
	// NOTE: here buff means the local fd - how about the conn_fd from remote:
	//       they're same!! refer to acceptsock() - at least on Ubuntu
	optvalue = 1; // NOTE: fix "address in use" due to TIME_WAIT
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));
	optvalue = 1;
	setsockopt(fd, IPPROTO_IP, TCP_NODELAY, &optvalue, sizeof(optvalue));

	debug("Open the trans=%d\n", fd);
	dumpsock(fd);

	memset(trans, 0, sizeof(Transaction));
	trans->trans_domain = DOMAIN_IPv4;
	trans->trans_type   = type;
	trans->backlog      = 5;
	trans->trans_fd     = fd;

	return 0;
}

int bindsock(Transaction* trans, const char* str)
{
	struct sockaddr_in sockaddr;

	if ((trans == NULL) || (str == NULL) || (strlen(str) == 0))
	{
		error("Bad parameters\n");
		return -1;
	}

	if (trans->trans_domain != DOMAIN_IPv4)
	{
		error("Invalid parameters\n");
		return -1;
	}

	int err = format_sockaddr(0, str, &sockaddr);
	if (err < 0)
	{
		return -2;
	}

	socklen_t addrlen = sizeof(struct sockaddr_in);
	err = bind(trans->trans_fd, &sockaddr, addrlen);
	if (err < 0)
	{
		error("Failed to bind, err=%d\n", err);
		say_errno();
		return -2;
	}
	debug("Succeed to bind\n");

	memcpy(&trans->trans_addr, &sockaddr, addrlen);
	trans->trans_len = addrlen;

	return 0;
}

int listensock(Transaction* trans)
{
	int err = listen(trans->trans_fd, trans->backlog);
	if (err < 0)
	{
		error("Failed to listen, err=%d\n", err);
		say_errno();
		return -2;
	}
	debug("Succeed to listen\n");

	trans->trans_mode = 0; // listen() makes socket passive

	return 0;
}

int acceptsock(Transaction* trans)
{
	// TODO: domain/AP_* comares with 'sockaddr' to judge. Now IPv4 ONLY
	struct sockaddr_in sockaddr;
	/*  NOTE !!! IMPORTANT !!!
		Do init of "socklen", especially to call accept();otherwise, 
		it will cause the next accept() failure. I am not clear the root cause, 
		but at least it's a better coding that avoids unnecessary/protential issue!! */
	socklen_t socklen = sizeof(sockaddr);

	debug("pid=%d Accepting...\n", getpid());
	int cli_fd = accept(trans->trans_fd, &sockaddr, &socklen);
	if (cli_fd < 0)
	{
		error("Failed to accept, err=%d\n", cli_fd);
		say_errno();
		return -2;
	}

	debug("--> New comming conn=%d (%s:%d)\n", cli_fd,
		inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));
	dumpsock(cli_fd);

	// Add the new client to the connection pool
	TransConn* conn = (TransConn*)malloc(sizeof(TransConn));
	memset(conn, 0, sizeof(TransConn));
	conn->conn_fd = cli_fd;
	memcpy(&conn->conn_addr, &sockaddr, socklen);
	conn->conn_len = socklen;
	//
	if (trans->conn_start == NULL)
	{
		trans->conn_start = conn;
	}
	else
	{
		trans->conn_end->nextconn = conn;
	}
	trans->conn_end = conn;
	//
	trans->conn_nbr ++;

	return 0;
}

int connectsock(Transaction* trans, const char* str)
{
	struct sockaddr_in sockaddr;

	if ((trans == NULL) || (str == NULL) || (strlen(str) == 0))
	{
		error("Bad parameters\n");
		return -1;
	}

	if (trans->trans_domain != DOMAIN_IPv4)
	{
		error("Invalid parameters\n");
		return -1;
	}

	int err = format_sockaddr(0, str, &sockaddr);
	if (err < 0)
	{
		return -2;
	}

	err = connect(trans->trans_fd, &sockaddr, sizeof(struct sockaddr_in));
	if (err < 0)
	{
		error("Failed to connect, err=%d\n", err);
		say_errno();
		return -2;
	}
	debug("Succeed to connect\n");

	return 0;
}

void closesock(Transaction* trans)
{
// TODO: fix me - loop to close conns and self-fd

	debug("Closed the trans=%d\n", trans->trans_fd);
}

void closeconn(Transaction* trans, TransConn* conn)
{
#if 0 // conn_start and conn_end not fixed well yet

	if ((trans == NULL) || (conn == NULL))
	{
		retrun -1;
	}

	close(conn->conn_fd); // TODO: shutdown()??

	if (p == NULL)
		return;

	TransConn* p = trans->conn_start;
	if (p == conn) // 1st one to remove
	{
		trans->conn_start = conn->nextconn;

		free(conn);
		conn = NULL;

		return;
	}

	while (p->nextconn != NULL)
	{
		if (p->nextconn == conn)
		{
			p->nextconn = conn->nextconn;

			free(conn);
			conn = NULL;
		}

		p = p->nextconn;
	}
#endif
}

// *******************************************************************
char* trans_get_eventname(TransEvent evt)
{
	switch(evt)
	{
		case TransEvent_NEWCONNECTION:
			return "NEW CONNECTION";
		break;
		case TransEvent_REQUEST_TIMEOUT:
			return "REQUEST TIMEOUT";
		break;
		case TransEvent_RECEIVE_FAILURE:
			return "RECEIVE FAILURE";
		break;
		case TransEvent_OUT_OF_MEMORY:
			return "OUT OF MEMORY";
		break;
		case TransEvent_INCOMING_MSG:
			return "INCOMING MSG";
		break;
		case TransEvent_ON_DISCONNECT:
			return "ON DISCONNECT";
		break;
		default:
		break;
	}

	return "<unknown trans event>";
}

static void (*notify_newevent)(TransEvent, TransConn*, char*, unsigned int) = NULL;

void trans_addlisten(Transaction* p, 
	void (*cb)(TransEvent, TransConn*, char*, unsigned int))
{
	notify_newevent = cb;
}

TransConn* trans_find(Transaction* trans, uint32 fd)
{
	TransConn* conn = trans->conn_start;

	while (conn != NULL)
	{
		if (conn->conn_fd == fd)
			return conn;

		conn = conn->nextconn;
	}

	debug("Warning: not found fd %d in conn pool\n", fd);
	return NULL;
}

#define TRANS_RECVBUF_SIZE 120
#define TRANS_SENDBUF_SIZE (TRANS_RECVBUF_SIZE+8)

// TODO: MSS should not be larger than '1500' - that put it in "static" area is good???
#define TOTOAL_MSG_LEN 1500
static char totalmsg[TOTOAL_MSG_LEN];

void transact(TransConn *conn)
{
	int conn_fd = conn->conn_fd;

	/* NOTE actually i can direclty put received data into "totalmsg",
	here "transrecv" mainly aims to simulate the bad netowrk env*/
	char transrecv[TRANS_RECVBUF_SIZE];
	int bufflen = sizeof(transrecv), len;

	int totalrecv;

	debug("Start to receive at conn=%d\n", conn_fd);

	//while (1)
	// NOTE: one-time receive for one connetion(accepted) - "while" just for TEST
	{
#ifdef TEST_SEND_STRESS
		sleep(give_random(10)); // for Stress not sleep always - i think it simulates an actual env.
#endif

		totalrecv = 0;

		// TODO: add a TO-management, and cb(REQUEST_TIMEOUT)

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

				//TODO: cb(RECEIVE_FAILURE)

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
				// TODO: cb(OUT_OF_MEMORY)

				error("Too large message requested!!\n");
				return;
			}
			memcpy(totalmsg+totalrecv, transrecv, len);
			totalrecv += len;
		} while (len == bufflen); // MOSTLY there're still bytes remained in kernel buff

		// Submit received bytes
		if (totalrecv > 0)
		{
			if (notify_newevent)
				notify_newevent(TransEvent_INCOMING_MSG,
					conn, totalmsg, totalrecv);
		}
	}
}

void perform_send(Transaction* trans)
{
	char transsnd[TRANS_SENDBUF_SIZE];
	int bufflen = sizeof(transsnd), len;

	//while (1)
	// NOTE: one-time send
	{
		memset(transsnd, 'c', sizeof(transsnd));

		sprintf(transsnd, "Client xxx:xx says:"); // TODO: add local IP:port
		strcat(transsnd, curr_timestr());

#ifdef TEST_SEND_STRESS
		debug("Sending...\n"); // to check the blocking
#endif
		len = send(trans->trans_fd, transsnd, bufflen, 0);// TODO: flags
		if (len != bufflen)
		{
			error("Failed to send, err=%d\n", len);
			say_errno();
			return;
		}

		debug("pid=%d Client sent %d bytes>>\n", getpid(), len);
/*			int i;
		for (i = 0; i < len; i ++)
			printf("%c", transsnd[i]);
		//printf("%s", transsnd);
		printf("(end)\n");
*/
	}
}

int trans_start(Transaction* p) // mode "0" means Server
{
	Transaction trans;
	memcpy(&trans, p, sizeof(trans));

/*
	Here we go
*/
	if (trans.trans_mode) // Client: the transaction is requesting
	{
		perform_send(&trans);
	}
	else // Server: standby for next connection
	{
#ifdef TRANS_IO_SELECT
		trans_start_io(&trans);
#else
		while (1)
		{
			int err = acceptsock(&trans);
			if (err != 0)
			{
				return -2;
			}

			// Another new task created to receive the requests...
#if 0
			// NOTE: "conn_end->conn_fd" is just the new accepted Client
			new_task(transact, trans.conn_end->conn_fd);
#else
			int pid = fork();
			if (pid == 0)
			{
				debug("After fork, Server child pid=%d (recv) continues\n", getpid());

				/*
					NOTE: in Chind process do close the Parent "fd"
				*/
				close(trans.trans_fd);

				if (notify_newevent)
					notify_newevent(TransEvent_NEWCONNECTION, trans.conn_end, NULL, 0);

				// --------------------------------
				// Main receive body
				transact(trans.conn_end);
				// --------------------------------

				if (notify_newevent)
					notify_newevent(TransEvent_ON_DISCONNECT,trans.conn_end, NULL, 0);

				// not need the "conn" anymore
				// TODO: call closesock()
				close(trans.conn_end->conn_fd);

				// TODO: add a diagnostic way to check opened filed, allocated mem...and dump here

				/* NOTE: child (client) ends, so must exit;
				otherwise the client will loop to next accept(), which not we want */
				debug("--> Server pid=%d recvtask ends\n\n", getpid());
				exit(0);
			}
			else if (pid < 0)
			{
				// TODO: closesock() to release added conn!!!

				error("Failed to fork to accept!!\n");
				say_errno();
			}
			else
			{
				debug("After fork, Server parent pid=%d (accept) continues\n", getpid());

				/*
					NOTE: in Parent process do close the Child "fd"
				*/
				close(trans.conn_end->conn_fd);

				// TODO: waitpid??
				// TODO end the zombie
				// TODO: when a conn disconnected, call closeconn(), removed from conn-pool
			}
#endif
		}
#endif
	}

	// TODO: RELEASE the resource before leaving (return or exit)!!!

	return -3;
}

Transaction* trans_create(int mode, char* dst)
{
/*
	Transaction init
*/
	Transaction trans;
	memset(&trans, 0, sizeof(Transaction));

	opensock(TRANS_TCP, &trans);
	if (mode)
	{
		trans.trans_mode = 1; // non-zero value means Client
//TODO: NOW the code can work!! Go to get local port from cmdline
//		if (bindsock(&trans, "127.0.0.1:9002") !=0 ) // NOTE: +bind() to use the dedicated port
//			return -2;

		if (connectsock(&trans, dst) != 0) // TODO: test the case not on local
			return NULL;
	}
	else // server mode
	{
		if (bindsock(&trans, dst) != 0)
			return NULL;

		if (listensock(&trans) != 0)
			return NULL;
	}

	//
	Transaction* ptrans = malloc(sizeof(Transaction));
	memcpy(ptrans, &trans, sizeof(Transaction));

	return ptrans;
}

void trans_destory(Transaction* p)
{
	closesock(p);

	free(p);
}

// *******************************************************************
void send_stress(char* dst)
{
	while (1)
	{
		pid_t pid = fork();

		if (pid < 0)
			return;

		if (pid == 0)
		{
			Transaction* p = trans_create(1, dst); // "1" is Client

			perform_send(p);

			trans_destory(p);

			exit(0); // must end the process
		}
		else
		{
			int sta;
			waitpid(pid, &sta, WNOHANG); // TODO: correct the usage here
		}

		usleep(300);
	}
}