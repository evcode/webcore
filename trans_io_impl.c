#include "trans.h"
#include "trans_io.h"

static Transaction* curr_trans;

static BOOL _process_trans_io(uint32 fd, TRANS_IO_EVENT evt)
{
	switch(evt)
	{
		case TRANS_IO_READABLE:
			if (curr_trans->trans_fd == fd) // listen (server) fd
			{
				int err = acceptsock(curr_trans);
				if (err != 0)
				{
					break;
				}

				io_add(curr_trans->conn_end->conn_fd);
			}
			else // (remote) conn fd
			{
				if (trans_find(fd) != NULL) // TODO: OPTIMIZE to get TransConn!!! it's O(n) now!!!
				{
					transact(curr_trans->conn_end);
					
					close(curr_trans->conn_end->conn_fd);
				}

				io_remove(fd);
			}
		break;

		default:
			debug("Not handle the fd %d's IO event %d\n", fd, evt);
		break;
	}

	return TRUE;
}

int trans_start_io(Transaction* p)
{
	curr_trans = p;

	io_addlisten(_process_trans_io);
	io_add(curr_trans->trans_fd);

	io_execute();

	return 0;
}

#if 0
int on_conn_connected(Transaction* trans)
{
	int err = acceptsock(&trans);
	if (err != 0)
	{
		return -2;
	}

	if (notify_newevent)
		notify_newevent(TransEvent_NEWCONNECTION, trans.conn_end, NULL, 0);
}

int on_conn_received(Transaction* trans, TransConn *conn)
{
	// do recv()

	// Submit received bytes
	if (totalrecv > 0)
	{
		if (notify_newevent)
			notify_newevent(TransEvent_INCOMING_MSG,
				conn, totalmsg, totalrecv);
	}
}

int on_conn_closing(Transaction* trans, TransConn *conn)
{
	if (notify_newevent)
		notify_newevent(TransEvent_ON_DISCONNECT,trans.conn_end, NULL, 0);
}



{
	while (1)
	{
		err = on_conn_connected();

		pid_t pid = fork();

		if (pid == 0)
		{
			// close "listen" fd

			on_conn_received()

			on_conn_closing()
		}
		else if (pid < 0)
		{

		}
		else
		{
			// close conn fd
		}

	}
}
#endif