#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

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

#include <errno.h>
extern int errno;
/*
void say_errno()
{
	error("errno %d - %s\n", errno, strerror(errno));
}
*/
#define say_errno() error("errno %d - %s\n", errno, strerror(errno))

#endif