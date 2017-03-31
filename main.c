#include <stdio.h>
#include <errno.h>

// TODO: double check how debug macro impl in tmm projects and some opensource, such as nginx
#define error(x...) do {\
						printf("[ERROR] %s,%d: ", __func__, __LINE__);\
						printf(x);\
					 } while(0)
#define inform(x...)  do {\
						printf("[INFORM] %s,%d: ", __func__, __LINE__);\
						printf(x);\
					 } while(0)
#define debug(x...)  do {\
						printf("[DEBUG] %s,%d: ", __func__, __LINE__);\
						printf(x);\
					 } while(0)

extern int errno;

void say_errno()
{
	error("error %d - %s\n", errno, strerror(errno));
}

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

#include <arpa/inet.h> // dedicate to inet_ntoa();otherwise, crash!!!

#include <sys/types.h>
#include <sys/socket.h>

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

typedef struct trans_struct
{
	int trans_fd;	
	TRANS_DOMAIN trans_domain;
	TRANS_TYPE trans_type;
	int max_conn;
} Transaction;

typedef void* TransHandler;

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
	    int nbr = sscanf(str, "%[0-9,.]:%d", ip_str, &ip_port); // AMAZING sscanf()!!
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
	debug("inet_ntoa(sockaddr) %s=%u(%#x), and port=%d(%#x)\n", inet_ntoa(addr->sin_addr),
		addr->sin_addr.s_addr, addr->sin_addr.s_addr, addr->sin_port, addr->sin_port);

	return 0;
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
	debug("Open the trans <%d>\n", fd);

	memset(trans, 0, sizeof(Transaction));
	trans->trans_domain = DOMAIN_IPv4;
	trans->trans_type   = type;
	trans->max_conn     = 3;
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
	int err = listen(trans->trans_fd, trans->max_conn);
	if (err < 0)
	{
		error("Failed to listen, err=%d\n", err);
		say_errno();
		return -2;
	}
	debug("Succeed to listen\n");

	return 0;
}

int accpetsock(Transaction* trans)
{
	// TODO: domain/AP_* comares with 'sockaddr' to judge. Now IPv4 ONLY
	struct sockaddr_in sockaddr;
	socklen_t socklen;

	debug("Accepting...\n");
	int cli_fd = accept(trans->trans_fd, &sockaddr, &socklen); // TODO: added into Trans
	if (cli_fd < 0)
	{
		error("Failed to accept, err=%d\n", cli_fd);
		say_errno();
		return -2;
	}

	debug("New comming connection=%d (%s:%d)\n", cli_fd, 
		inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));

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
	Transaction trans;

	if (system_le())
		debug("System Little-endian\n");

	inform("argc=%d, argv[0]=%s\n", argc, argv[0]);
	char * dst = "ANY";//"192.168.100.218:3000" "ANY" // TODO: read from cmdline

	opensock(TRANS_TCP, &trans);
	if (argc > 1)
	{
		connectsock(&trans, dst); // TODO: not on local 192.168.1.1:33
	}
	else // server mode
	{
		bindsock(&trans, dst);
		listensock(&trans);
		accpetsock(&trans);
	}

    return 0;
}