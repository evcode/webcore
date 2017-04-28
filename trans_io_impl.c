#include "trans.h"
#include "trans_io.h"

static Transaction* curr_trans; // TODO: thread-safe
// TODO: be carefull where the pointer points to, Stack or others????

static BOOL _process_trans_io(uint32 fd, TRANS_IO_EVENT evt)
{
	if (curr_trans == NULL)
	{
		error("Invalid Trans context\n");
		return FALSE;
	}

	switch(evt)
	{
		case TRANS_IO_READABLE:
			if (curr_trans->trans_fd == fd) // listen (server) fd
			{
				debug("+ IO Readable-event, master fd=%d\n", fd);

				int err = acceptsock(curr_trans);
				if (err != 0)
				{
					break;
				}

				// add the new conn into fd "select"
				io_add(curr_trans->conn_end->conn_fd);
			}
			else // (remote) conn fd
			{
				debug("+ IO Readable-event, guest fd=%d\n", fd);

				TransConn* p = trans_find(curr_trans, fd); // TODO: OPTIMIZE!!! it's O(n) now!!!
				if (p != NULL)
				{
					transact(p);
					
					// remove the conn for select
					close(p->conn_fd);
				}

				io_remove(fd);
			}
		break;

		case TRANS_IO_TIMEOUT: // TODO: close the TO fd
#if 1
			printf("z");
			fflush(stdout);
#else
			debug("Time out on all fds, and removed\n");
			// TODO: close all fds
			//io_remove_all(); // remove all clients that's time out
#endif
		break;

		default:
			debug("+ Not handle the fd %d's IO event %d\n", fd, evt);
		break;
	}

	return TRUE;
}

int trans_start_io(Transaction* p)
{
	debug("+ IO scanning: master fd=%d\n", p->trans_fd);

	curr_trans = p;

	io_addlisten(_process_trans_io);
	io_scan(curr_trans->trans_fd);

	return 0;
}