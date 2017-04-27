#include "trans_io.h"

/*
SYNOPSIS
       // According to POSIX.1-2001
       #include <sys/select.h>

       // According to earlier standards
       #include <sys/time.h>
       #include <sys/types.h>
       #include <unistd.h>

       int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);

       void FD_CLR(int fd, fd_set *set);
       int  FD_ISSET(int fd, fd_set *set);
       void FD_SET(int fd, fd_set *set);
       void FD_ZERO(fd_set *set);

   The timeout
       The time structures involved are defined in <sys/time.h> and look like

           struct timeval {
               long    tv_sec;         // seconds
               long    tv_usec;        // microseconds
           };

RETURN VALUE
       On  success,  select()  and  pselect()  return the number of file descriptors contained in the three returned
       descriptor sets (that is, the total number of bits that are set in readfds, writefds, exceptfds) which may be
       zero  if the timeout expires before anything interesting happens.  On error, -1 is returned, and errno is set
       appropriately; the sets and timeout become undefined, so do not rely on their contents after an error.

ERRORS
       EBADF  An invalid file descriptor was given in one of the sets.  (Perhaps a file descriptor that was  already
              closed, or one on which an error has occurred.)

       EINTR  A signal was caught; see signal(7).

       EINVAL nfds is negative or the value contained within timeout is invalid.

       ENOMEM unable to allocate memory for internal tables.
*/
#include <sys/select.h>

//#include <sys/timer.h> // for timerclear (Ubuntu only)

static fd_set io_rfds;
static uint32 io_maxfd = 0;
static uint32 io_fds[1024]; // FD_MAXSIZE on Ubuntu

static TransIoReadyCb io_ready_cb = NULL;

BOOL io_add(uint32 fd)
{
	int i;
	for (i = 0; i < sizeof(io_fds)/sizeof(io_fds[0]); i ++)
	{
		if (io_fds[i] == 0)
		{
			io_fds[i] = fd;

			return TRUE;
		}
	}

	return FALSE;
}

BOOL io_remove(uint32 fd)
{
	int i;
	for (i = 0; i < sizeof(io_fds)/sizeof(io_fds[0]); i ++)
	{
		if (io_fds[i] == fd)
		{
			io_fds[i] = 0;

			return TRUE;
		}
	}

	return FALSE;
}

void io_addlisten(TransIoReadyCb cb)
{
	io_ready_cb = cb;
}

static void _reset_fds()
{
	FD_ZERO(&io_rfds);
	io_maxfd = 0;

	int i;
	for (i = 0; i < sizeof(io_fds)/sizeof(io_fds[0]); i ++)
	{
		if (io_fds[i] != 0)
		{
			FD_SET(io_fds[i], &io_rfds);

			if (io_fds[i]>io_maxfd)
				io_maxfd = io_fds[i];
		}
	}
}

static void _trigger_fds(int n)
{
	int i, count = 0;
	for (i = 0; i < sizeof(io_fds)/sizeof(io_fds[0]); i ++)
	{
		if ((io_fds[i] != 0) && FD_ISSET(io_fds[i], &io_rfds))
		{
			if (io_ready_cb)
				io_ready_cb(io_fds[i], TRANS_IO_READABLE);

			count ++;
			if (count >= n)
				break;
		}
	}
}

void io_execute()
{
	BOOL is = TRUE;

	struct timeval tv;
	//timerclear(&tv); // on Ubuntu
	tv.tv_sec  = 5;
	tv.tv_usec = 0;

	while(is)
	{
		_reset_fds();

		int n = select(io_maxfd+1, &io_fds, NULL, NULL, &tv); // TODO: now only handle "read" detection
		if (n < 0)
		{
			error("Failed to select, err=%d\n", n);
			say_errno();

			return;
		}
		else if (n == 0) // TO
		{
			if (io_ready_cb)
				io_ready_cb(0, TRANS_IO_TIMEOUT);

			continue;
		}
		else
		{
			_trigger_fds(n);
		}
	}
}