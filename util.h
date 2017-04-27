#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// TODO: double check how debug macro impl in tmm projects and some opensource, such as nginx
#define error(x...) do {\
						printf("ERROR (%d) %s,%d|||", getpid(), __func__, __LINE__);\
						printf(x);\
					 } while(0)
#define inform(x...)  do {\
						printf("INFO  (%d) %s,%d|||", getpid(), __func__, __LINE__);\
						printf(x);\
					 } while(0)
#define debug(x...)  do {\
						printf("DEBUG (%d) %s,%d|||", getpid(), __func__, __LINE__);\
						printf(x);\
					 } while(0)

typedef enum
{
	FALSE = 0,
	TRUE  = 1
} BOOL;

typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;
typedef int   int32;
typedef short int16;
typedef char  int8;

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