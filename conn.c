#include "trans.h"
#include "util.h"

extern char** envlist_init(); // NOTE: (on MACOS??) it's MANDATORY;otherwise, it will crash when to read it here. WHY????????????

void respond(int fd, int err, char* s, int n)
{
	debug("To respond at fd=%d:\n", fd);
	if (fd < 0) // TODO: check "err"
		return;

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

void request(int fd, const char* msg, int msglen) // TODO: design a "listner" mechanism??? to notify, such as HTTP coming event
{
	debug("To request:\n");
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

	// construct env. variant retrieved from "msg"
	char** envlist = envlist_init();

	int envstart = 0;
	for (i = 0; i < msglen; i++) // Parse the line one by one
	{
		if ((msg[i] == '\r') || (msg[i] == '\n')) // one line ends
		{
			int envlen = i - envstart; // not contains '\0'
			if (envlen > 0) // to avoid that many '\r' or '\n' following together
			{
				char* envstr = malloc(envlen + 1); // TODO: any optimization can do here?? - not malloc always
				memcpy(envstr, msg + envstart, envlen);
				envstr[envlen] = '\0';

				envlist_add(envstr); // TODO: one more param for "envlist"

				free(envstr);
				envstr = NULL;
			}

			envstart = i + 1; // substring to parse starting from next byte
		}
	}

	envlist_dump(envlist, envlist_num());

	// run cgi
	if (envlist[0] != NULL) // 1st one is NULL, means no env variant
	{
		cgi_addlisten(fd, respond);
		cgi_run(envlist); // TODO: now it directly cb "respond", so still "syn" call! Improve!!
	}
}

void process_events(TransEvent evt, TransConn* conn, char* s, unsigned int l)
{
	if ((conn == NULL) || (evt >= TransEvent_UNKNOWN))
	{
		error("Invalid trans %x message(%d)\n", conn, evt);
		return;
	}
	debug("New event=%s\n", get_eventname(evt));

	switch (evt)
	{
		case TransEvent_NEWCONNECTION:
		case TransEvent_INCOMING_MSG:
		if ((s == NULL) || (l == 0)) // no actual data carried
		{
			return;
		}

		request(conn->conn_fd, s, l);

		break;

		case TransEvent_ON_DISCONNECT: // just before its exit
		break;
	}
}