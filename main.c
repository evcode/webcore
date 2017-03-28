#include <stdio.h>

#include <sys/socket.h>

// TODO: double check how debug macro impl in tmm projects and some opensource, such as nginx
#define inform(x...) printf("[INFORM, %s, %d] ", __func__, __LINE__);printf(x);
#define debug(x...)  printf("[DEBUG, %s, %d] ", __func__, __LINE__);printf(x);

typedef enum
{
    TRANS_IP = 0,
    TRANS_UDP,
    TRANS_TCP	
} TRANS_TYPE;

typedef struct trans_struct
{
	TRANS_TYPE trans_type;
	int trans_fd;
} Transaction;



/*
int socket(int domain, int type, int protol)
*/
int opensock(TRANS_TYPE type, Transaction* trans)
{
	int domain, socktype, proto;

	domain = AF_INET; // TODO: now only IPv4 domain supported
	socktype = (type == TRANS_TCP)?SOCK_STREAM:SOCK_DGRAM;
	proto  = 0; // TODO: how to match in kernel
	debug("Creation of domain=%d, socktype=%d, proto=%d\n", domain, socktype, proto);

	int fd = socket(domain, socktype, proto);
	if (fd < 0)
	{
		return -2;
	}
	debug("Open the trans fd <%d>\n", fd);

	memset(trans, 0, sizeof(Transaction));
	trans->trans_type = type;
	trans->trans_fd   = fd;

	return 0;
}

#include <netinet/in.h> // 'addr' struct
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

int bindsock(Transaction* trans, const char* addr)
{
	if ((trans == NULL) || (addr == NULL) || (strlen(addr) == 0))
	{
		return -1;
	}

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(struct sockaddr_in));
	sockaddr.sin_family = AF_INET;

    unsigned char ipaddr[4] = {127, 0, 0, 1};
	unsigned int ipport = 9000;

	if (strcmp(addr, "local") == 0)
	{
		// as 127.0.0.1
	}
	else
	{
	    int nbr = sscanf(addr, "%d.%d.%d.%d:%d", 
	    	&ipaddr[0], &ipaddr[1], &ipaddr[2], &ipaddr[3], &ipport);
	    if (nbr != 5)
	    {
	    	return -2;
	    }
	    debug("Input ip addr=%d.%d.%d.%d:%d\n",
	    	ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3], ipport);
	}

	debug("    --> test INADDR_ANY=%d\n", INADDR_ANY);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(ipport);
	debug("Bind the IP %d:%d\n", sockaddr.sin_addr.s_addr, sockaddr.sin_port);

	return 0;
}

int main (int argc, char* argv[])
{
	Transaction trans;

	opensock(TRANS_TCP, &trans);
	bindsock(&trans, "192.168.1.1:33");

    return 0;
}
