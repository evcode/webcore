#include "trans_io.h"

#include <sys/epoll.h>

// TODO; set "select", "epoll", etc. imple into a struct entity, using function-pointer list

/*
SYNOPSIS
       #include <sys/epoll.h>

       int epoll_create(int size);
       int epoll_create1(int flags);

RETURN VALUE
       On  success,  these  system calls return a nonnegative file descriptor.  On error, -1 is returned,
       and errno is set to indicate the error.

NOTES
       In  the initial epoll_create() implementation, the size argument informed the kernel of the number
       of file descriptors that the caller expected to add to the epoll instance.  The kernel  used  this
       information  as  a  hint for the amount of space to initially allocate in internal data structures
       describing events.  (If necessary, the kernel would allocate more  space  if  the  caller's  usage
       exceeded  the  hint given in size.)  Nowadays, this hint is no longer required (the kernel dynami‐
       cally sizes the required data structures without needing the hint), but size must still be greater
       than  zero, in order to ensure backward compatibility when new epoll applications are run on older
       kernels.
------------------------------------------------------------
SYNOPSIS
       #include <sys/epoll.h>

       int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);

RETURN VALUE
       When successful, epoll_ctl() returns zero.  When an error occurs, epoll_ctl() returns -1 and errno
       is set appropriately.

DESCRIPTION
       This  system  call  performs  control  operations on the epoll(7) instance referred to by the file
       descriptor epfd.  It requests that the operation op be performed for the target  file  descriptor,
       fd.

Valid values for the op argument are :

       EPOLL_CTL_ADD
              Register  the  target  file  descriptor  fd  on  the epoll instance referred to by the file
              descriptor epfd and associate the event event with the internal file linked to fd.

       EPOLL_CTL_MOD
              Change the event event associated with the target file descriptor fd.

       EPOLL_CTL_DEL
              Remove (deregister) the target file descriptor fd from the epoll instance  referred  to  by
              epfd.  The event is ignored and can be NULL (but see BUGS below).

The  event argument describes the object linked to the file descriptor fd.  The struct epoll_event
       is defined as :

           typedef union epoll_data {
               void        *ptr;
               int          fd;
               uint32_t     u32;
               uint64_t     u64;
           } epoll_data_t;

           struct epoll_event {
               uint32_t     events;      // Epoll events
               epoll_data_t data;        // User data variable
           };

The events member is a bit set composed using the following available event types:

       EPOLLIN
              The associated file is available for read(2) operations.

       EPOLLOUT
              The associated file is available for write(2) operations.

       EPOLLRDHUP (since Linux 2.6.17)
              Stream socket peer closed connection, or shut down writing half of connection.  (This  flag
              is  especially useful for writing simple code to detect peer shutdown when using Edge Trig‐
              gered monitoring.)

       EPOLLPRI
              There is urgent data available for read(2) operations.

       EPOLLERR
              Error condition happened on the associated file descriptor.  epoll_wait(2) will always wait
              for this event; it is not necessary to set it in events.

       EPOLLHUP
              Hang  up  happened  on  the associated file descriptor.  epoll_wait(2) will always wait for
              this event; it is not necessary to set it in events.

       EPOLLET
              Sets the Edge Triggered behavior for the associated file descriptor.  The default  behavior
              for  epoll  is  Level Triggered.  See epoll(7) for more detailed information about Edge and
              Level Triggered event distribution architectures.

       EPOLLONESHOT (since Linux 2.6.2)
              ...
------------------------------------------------------------
SYNOPSIS
       #include <sys/epoll.h>

       int epoll_wait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout);
       int epoll_pwait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout,
                      const sigset_t *sigmask);

DESCRIPTION
       The  epoll_wait()  system  call  waits  for events on the epoll(7) instance referred to by the file
       descriptor epfd.  
       The memory area pointed to by events will contain the events that will be  avail‐
       able for the caller.  Up to maxevents are returned by epoll_wait().  The maxevents argument must be
       greater than zero.

RETURN VALUE
       When successful, epoll_wait() returns the number of file descriptors ready for the  requested  I/O,
       or  zero  if  no  file  descriptor became ready during the requested timeout milliseconds.  When an
       error occurs, epoll_wait() returns -1 and errno is set appropriately.
*/

#define EPOLL_FD_MAXCOUNT 128
#define EPOLL_MAXEVENT 64
static int epoll_fd;

static TransIoReadyCb io_ready_cb = NULL;

BOOL io_add(int32 fd)
{
	return FALSE;
}

BOOL io_remove(int32 fd)
{
	return FALSE;
}

void io_addlisten(TransIoReadyCb cb)
{
	io_ready_cb = cb;
}

static void _init()
{
	static BOOL init = FALSE;

	if (init)
		return;

	epoll_fd = epoll_create(EPOLL_FD_MAXCOUNT); // since 2.6.8 the "size" is ignored in Kernel
	if (epoll_fd < 0)
	{
		error("Failed to request epoll\n");
		say_errno();
		return;
	}

	init = TRUE;
}

void io_scan(int32 fd)
{
	_init();

	//
	struct epoll_event ev;
	memset(&ev, 0, sizeof(struct epoll_event));
	ev.events = EPOLLIN;

    int err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
	if (err < 0)
	{
		error("Failed to add into epoll, fd=%d\n", fd);
		say_errno();
		return;
	}

	//
	int timeout = -1; // -1: blocking, 0: non-blocking, others in millsec
	int maxevents = EPOLL_MAXEVENT; // TODO: more tricks here???

	BOOL is = TRUE;
	while(is)
	{
		memset(&ev, 0, sizeof(struct epoll_event));
		int cnt = epoll_wait(epoll_fd, &ev, maxevents, timeout);
		if (cnt < 0)
		{
			error("Failed to epolling\n");
			say_errno();
			return;
		}

		if (cnt == 0) // no fd ready when TO reaches
		{
			if (io_ready_cb)
				io_ready_cb(-1, TRANS_IO_TIMEOUT); // here "0" means "no fd"
		}
		else // "cnt" means the count of read fds
		{
			int i;
			for (i = 0; i < cnt; i ++)
			{
				if (io_ready_cb)
					io_ready_cb(ev.data.fd, TRANS_IO_READABLE);
			}
		}
	}

	return FALSE;
}