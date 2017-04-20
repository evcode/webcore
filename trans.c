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

	debug("\n");
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
	optvalue = 1; // NOTE: fix "address in use" due to TIME_WAIT
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));
	optvalue = 1;
	setsockopt(fd, IPPROTO_IP, TCP_NODELAY, &optvalue, sizeof(optvalue));

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