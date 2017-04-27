#include "trans.h"
#include "util.h"

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

#include <sys/timer.h> // for timerclear

typedef enum
{
	TRANS_IO_TIMEOUT,
	TRANS_IO_READABLE,
	TRANS_IO_WRITEABLE,
	TRANS_IO_EXCEPTION,
} TRANS_IO;

void io_add(int fd)
{

}

void io_addlisten()
{

}

void io_execute()
{
	BOOL is = TRUE;
	int fd;
	fd_set rfds;
	timeval tv;

	timerclear(&tv);

	while(is)
	{
		FD_ZERO(&rfds)
		FD_SET(fd, &rfds);

		int max_fd = fd;

		int n = select(max_fd+1, &rfds, NULL, NULL, &tv);
		if (n < 0)
		{
			error("Failed to select, err=%d\n", n);
			say_errno();
			return; // TODO: any release below
		}
		else if (n == 0) // TO
		{
			error("select timeout\n");
			continue;
		}
		else
		{
			if (FD_ISSET(fd, &rfds))
			{

			}
		}
	}
}