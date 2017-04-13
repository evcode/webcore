#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// TODO: double check how debug macro impl in tmm projects and some opensource, such as nginx
#define error(x...) do {\
						printf("ERROR %s,%d|||", __func__, __LINE__);\
						printf(x);\
					 } while(0)
#define inform(x...)  do {\
						printf("INFORM %s,%d|||", __func__, __LINE__);\
						printf(x);\
					 } while(0)
#define debug(x...)  do {\
						printf("DEBUG %s,%d|||", __func__, __LINE__);\
						printf(x);\
					 } while(0)

typedef enum
{
	FALSE = 0,
	TRUE  = 1
} BOOL;

// *******************************************************************
#include <errno.h>

extern int errno;

void say_errno()
{
	error("errno %d - %s\n", errno, strerror(errno));
}

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

// *******************************************************************
#include <netinet/in.h> // 'sockaddr' struct
/* const struct sockaddr *指针，指向要绑定给sockfd的协议地址。
这个地址结构根据地址创建socket时的地址协议族的不同而不同:
// ipv4:
struct sockaddr_in {
    sa_family_t    sin_family; // address family: AF_INET 
    in_port_t      sin_port;   // port in network byte order 
    struct in_addr sin_addr;   // internet address 
};
// Internet address
struct in_addr {
    uint32_t       s_addr;     // address in network byte order 
};

//ipv6对应的是： 
 struct sockaddr_in6 { 
    sa_family_t     sin6_family;   // AF_INET6 
    in_port_t       sin6_port;     // port number 
    uint32_t        sin6_flowinfo; // IPv6 flow information 
    struct in6_addr sin6_addr;     // IPv6 address 
    uint32_t        sin6_scope_id; // Scope ID (new in 2.4) 
};
struct in6_addr { 
    unsigned char   s6_addr[16];   // IPv6 address 
};

//Unix域对应的是： 
#define UNIX_PATH_MAX    108
struct sockaddr_un { 
    sa_family_t sun_family;               // AF_UNIX 
    char        sun_path[UNIX_PATH_MAX];  // pathname 
};
*/

#include <arpa/inet.h> // NOTE dedicate to inet_ntoa();otherwise, crash!!!

#include <sys/types.h>
#include <sys/socket.h>

//#define TEST_SEND_STRESS 1

#define TRANS_RECVBUF_SIZE 120
#define TRANS_SENDBUF_SIZE (TRANS_RECVBUF_SIZE+8)

typedef enum
{
    TRANS_IP = 0,
    TRANS_UDP,
    TRANS_TCP	
} TRANS_TYPE;

typedef enum
{
	DOMAIN_IPv4 = AF_INET, // TODO: not direclty use AF_*
	DOMAIN_IPv6 = AF_INET6,
	DOMAIN_LOCAL= AF_UNIX
} TRANS_DOMAIN;

typedef struct transconn
{
	int conn_fd; // Client (local) socket fd
	struct sockaddr conn_addr; // TODO: now only IPv4
	socklen_t conn_len;
	struct transconn* nextconn;
} TransConn;

typedef struct trans_struct
{
	char trans_mode; // server or client
	TRANS_DOMAIN trans_domain; // inet, inet6, ap_unix...
	TRANS_TYPE trans_type; // tcp(stream), udp(dgram)...

	int trans_fd;
	struct sockaddr trans_addr;
	socklen_t trans_len;

	// Server properties
	TransConn* conn_start; // that from Start to End maintains a Trans pool
	TransConn* conn_end;
	int conn_max; // allowed connected clients
	int conn_nbr; // connected clients count

	int backlog; // max backlog for listen()
} Transaction;

typedef void* TransDesc;

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

#include <netinet/in.h> // for such as "IPPROTO_IP"
#include <netinet/tcp.h> // for such as "TCP_NODELAY"
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

	getsockopt(fd, IPPROTO_IP, TCP_NODELAY, &otpvalue, &optlen);
	debug("TCP_NODELAY:%d\n", otpvalue);
	getsockopt(fd, IPPROTO_IP, TCP_CORK, &otpvalue, &optlen);
	debug("TCP_CORK:%d\n", otpvalue);

	debug("\n");
}

void construct_msg(char* msg, int msglen) // TODO: design a "listner" mechanism??? to notify, such as HTTP coming event
{
	debug("A new message constructed:\n");

	int i;
	for (i = 0; i < msglen; i++)
		printf("%c", msg[i]);
	//printf("%s", transrecv);
	printf("(end)\n");
}

void trans_recvtask(int conn_fd) // TODO: transfer a Trans struct not just a "fd"???
{
	char transrecv[TRANS_RECVBUF_SIZE];
	int bufflen = sizeof(transrecv), len;

#define TOTOAL_MSG_LEN 1500
	char totalmsg[TOTOAL_MSG_LEN]; // TODO: replace with realloc(), or direclty apply it to "transrecv" - char* transrecv;
	int totalrecv;

	debug("Start to receive at trans=%d\n", conn_fd);
	while (1)
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
					construct_msg(totalmsg, totalrecv); // Submit received bytes
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
				error("Too large message requested!!");
				return;
			}
			memcpy(totalmsg+totalrecv, transrecv, len);
			totalrecv += len;
		} while (len == bufflen); // MOSTLY there're still bytes remaind in kernel

		// Submit received bytes
		construct_msg(totalmsg, totalrecv);
	}
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
	int sndbuffsize = 64, rcvbuffsize = 64;
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuffsize, sizeof(sndbuffsize));
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuffsize, sizeof(rcvbuffsize));

	debug("Open the trans=%d\n", fd);	
	// NOTE: here buff means the local fd - how about the conn_fd from remote:
	//       they're same!! refer to acceptsock()
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

	err = bind(trans->trans_fd, &sockaddr, sizeof(struct sockaddr_in));
	if (err < 0)
	{
		error("Failed to bind, err=%d\n", err);
		say_errno();
		return -2;
	}
	debug("Succeed to bind\n");

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
	int cli_fd = accept(trans->trans_fd, &sockaddr, &socklen); // TODO: added into Trans
	if (cli_fd < 0)
	{
		error("Failed to accept, err=%d\n", cli_fd);
		say_errno();
		return -2;
	}

	debug("--> New comming trans=%d (%s:%d)\n", cli_fd, 
		inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));
	dumpsock(cli_fd);

	// Add the new client to the connection pool
	TransConn* conn = (TransConn*)malloc(sizeof(TransConn));
	memset(conn, 0, sizeof(TransConn));
	conn->conn_fd = cli_fd;
	memcpy(&conn->conn_addr, &sockaddr, socklen);
	conn->conn_len = socklen;

	if (trans->conn_start == NULL)
	{
		trans->conn_start = conn;
	}
	else
	{
		trans->conn_end->nextconn = conn;
	}
	trans->conn_end = conn;

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
// TODO: fix me
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

int main (int argc, char* argv[])
{
	if (system_le())
		debug("System Little-endian\n");

	//debug("argc=%d, argv[0]=%s\n", argc, argv[0]);
	int i = 0;
	for (i = 0; i < argc; i ++)
		printf("%s ", argv[i]);
	printf("\n");
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
			else
			{
				debug("After fork, Server parent pid=%d (accept) continues\n", getpid());

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