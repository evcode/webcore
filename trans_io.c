#ifdef TRANS_IO_SELECT
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

#define INVALID_FD (-1) // in theory "0" is valid fd

// TODO: BE CAREFULL for thread-safe since they're all "global"!!!!!
static fd_set io_rset, io_wset, io_eset;
// TODO: FD_MAXSIZE on Ubuntu,or check with sizeof(fd_set)
static int8* io_fds = NULL;
static uint32 io_fdlen = 0; // total max length
static uint32 io_fdnum = 0; // current count

static TransIoReadyCb io_ready_cb = NULL;

BOOL io_add(int32 fd)
{
	if (fd <= INVALID_FD)
	{
		error("Invalid IO fd=%d to add\n", fd);
		return FALSE;
	}

	int i;
	for (i = 0; i < io_fdlen; i ++)
	{
		if (io_fds[i] == INVALID_FD) // available position "i"
		{
			io_fds[i] = fd; // TODO: duplicate fds to add?????

			io_fdnum ++;
			debug("+ IO fd=%d added to listen\n", fd);
			return TRUE;
		}
	}

	error("+ No more available IO resource to add\n");
	return FALSE;
}

BOOL io_remove(int32 fd)
{
	if (fd <= INVALID_FD)
	{
		error("Invalid IO fd=%d to remove\n", fd);
		return FALSE;
	}

	int i;
	for (i = 0; i < io_fdlen; i ++)
	{
		if (io_fds[i] == fd)
		{
			io_fds[i] = INVALID_FD; // reset

			io_fdnum --;
			debug("+ IO fd=%d remvoed from listen\n", fd);
			return TRUE; // TODO: duplicate added fds to remove?????
		}
	}

	debug("+ Warning: no IO fd removed\n");
	return FALSE;
}

#if 0
// NOTE that all removed results in the task ending since no IO to scan

BOOL io_remove_all()
{
	memset(io_fds, INVALID_FD, io_fdlen);

	return TRUE;
}
#endif

void io_addlisten(TransIoReadyCb cb)
{
	io_ready_cb = cb;
}

static int _reset_fds(struct timeval* tv)
{
	int count = 0;

	// reset read-fds
	FD_ZERO(&io_rset);
	int io_maxfd = -1;

	int i;
	for (i = 0; i < io_fdlen; i ++)
	{
		if (io_fds[i] != INVALID_FD)
		{
			FD_SET(io_fds[i], &io_rset); // set it

			if (io_fds[i] > io_maxfd)
				io_maxfd = io_fds[i]; // get maxfd

			//
			count ++;
			if (count >= io_fdnum)
				break; // all valid(added) fds handled already

		}
	}

	// reset TO
	tv->tv_sec  = 5;
	tv->tv_usec = 0;

	return io_maxfd;
}

static void _trigger_fds(int n) // TODO: throw into a thread-pool, but take care about "thread-safe"
{
	int i, count = 0;
	for (i = 0; i < io_fdlen; i ++)
	{
		if ((io_fds[i] != INVALID_FD) &&
			FD_ISSET(io_fds[i], &io_rset))
		{
			if (io_ready_cb)
				io_ready_cb(io_fds[i], TRANS_IO_READABLE);

			count ++;
			if (count >= n)
				break; // all "selected" fd handled already
		}
	}
}

void io_scan(int32 fd)
{
	if (fd <= INVALID_FD)
	{
		error("Invalid IO fd=%d to start\n", fd);
		return;
	}

	/*
		Create fds-list and add the initial fd
	*/
	io_fdlen = sizeof(fd_set)*8;
	debug("+ IO fd_set length has %d BITs\n", io_fdlen);
	if (io_fds == NULL)
		io_fds = malloc(io_fdlen);
	memset(io_fds, INVALID_FD, io_fdlen); // mandatory init - all "-1"

	io_add(fd);

	/*
		IO-fds processing
	*/
	BOOL is = TRUE;
	struct timeval tv;
	//timerclear(&tv); // on Ubuntu
	while(is)
	{
		int io_maxfd = _reset_fds(&tv);
		if (io_maxfd < 0)
		{
			error("+ No IO resource being monitored. End the task!!\n");
			break;
		}

		int n = select(io_maxfd+1, &io_rset, NULL, NULL, &tv); // TODO: now only handle "read" detection
		if (n < 0)
		{
			error("Failed to select IO fds, err=%d\n", n);
			say_errno();

			return;
		}
		else if (n == 0) // TO
		{
			if (io_ready_cb)
				io_ready_cb(INVALID_FD, TRANS_IO_TIMEOUT); // here "0" means "no fd"

			continue;
		}
		else // "n" fds detected:
		{
			_trigger_fds(n);
		}
	}

	// TODO: release the resources
}
#endif