#include "trans.h"
#include "util.h"

extern char** envlist_init(); // NOTE: (on MACOS??) it's MANDATORY;otherwise, it will crash when to read it here. WHY????????????

void on_cgi_notified(int fd, int err, char* s, int n)
{
	debug("On new CGI notified=%s\n", cgi_get_notifyname(err));

	int i;
	for (i = 0; i < n; i ++)
		printf("%c", s[i]);
	printf("(end)\n");

	// status-line
	char* html_field="HTTP/1.1 200 OK\r\n";
	send(fd, html_field, strlen(html_field), 0);

	// respond-header
	html_field="Server: miniweb\r\nContent-Type: text/html; charset=utf-8\r\n";
	send(fd, html_field, strlen(html_field), 0);
	// "CONTENT_LENGTH" not mandatory???

	// content body
	char* html_format = "\r\n"; // content starts here - Mandatory
								// ,for some format, without webpage cannot identify
	send(fd, html_format, strlen(html_format), 0);

	int len = send(fd, s, n, 0); // TODO: flags
	if (len != n)
	{
		error("Failed to send, err=%d\n", len);
		say_errno();
		return;
	}

	debug("pid=%d Server responds %d bytes>>\n", getpid(), len);
}

void request_cgi(int fd, const char* msg, int msglen) // TODO: design a "listner" mechanism??? to notify, such as HTTP coming event
{
	debug("Do request CGI:\n");
	if ((msg == NULL) || (msglen <= 0))
	{
		error("Bad parameters!!\n");
		return;
	}

	int i;
#if 0 // TEST the entire received bytes
	for (i = 0; i < msglen; i++)
		printf("%c", msg[i]);
	printf("(end)\n");
#endif

	cgi_addlisten(fd, on_cgi_notified);
	cgi_request(msg, msglen);
}

void on_trans_notified(TransEvent evt, TransConn* conn, char* s, unsigned int l)
{
	if ((conn == NULL) || (evt >= TransEvent_UNKNOWN))
	{
		error("Invalid trans %x message(%d)\n", conn, evt);
		return;
	}
	debug("On new trans event=%s\n", trans_get_eventname(evt));

	switch (evt)
	{
		case TransEvent_NEWCONNECTION:
		case TransEvent_INCOMING_MSG:
		if ((s == NULL) || (l == 0)) // no actual data carried
		{
			return;
		}

		request_cgi(conn->conn_fd, s, l);
		break;

		case TransEvent_ON_DISCONNECT: // just before its exit
		break;
	}
}