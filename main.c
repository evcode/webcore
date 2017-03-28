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
int opensock(TRANS_TYPE trans_type)
{
	int domain, socktype, proto;

	domain = AF_INET;
	socktype = (trans_type == TRANS_TCP)?SOCK_STREAM:SOCK_DGRAM;
	proto  = 0; // TODO: how to match in kernel
	debug("domain=%d, socktype=%d, proto=%d\n", domain, socktype, proto);

	int trans = socket(domain, socktype, proto);
	debug("trans=%d opened\n", trans);

	return trans;
}

int main (int argc, char* argv[])
{
    printf("Hello GitHub!!\n");
    printf("one change here\n");

    int trans_fd=opensock(TRANS_TCP);

    return 0;
}
