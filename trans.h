#ifndef TRANS_H
#define TRANS_H

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h> // 'sockaddr' struct, + for such as "IPPROTO_IP"
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
#include <netinet/tcp.h> // for such as "TCP_NODELAY"

#include <arpa/inet.h> // NOTE dedicate to inet_ntoa();otherwise, crash!!!

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

#endif