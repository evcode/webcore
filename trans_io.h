#ifndef TRANS_IO_H
#define TRANS_IO_H

#include "util.h"

typedef enum
{
	TRANS_IO_TIMEOUT=0,
	TRANS_IO_READABLE,
	TRANS_IO_WRITEABLE,
	TRANS_IO_EXCEPTION
} TRANS_IO_EVENT;

typedef BOOL (*TransIoReadyCb)(int32 fd, TRANS_IO_EVENT evt);

BOOL io_add(int32 fd);
BOOL io_remove(int32 fd);
//BOOL io_remove_all();
void io_addlisten(TransIoReadyCb cb);
void io_scan(int32 fd);

#endif