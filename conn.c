#include "trans.h"
#include "util.h"
#include "cgicall.h"

extern char* trans_get_eventname(TransEvent evt); // on MACOS, crash without it
extern char* cgi_get_notifyname(int evt); // on MACOS, crash without it
extern CGI_StatusCode cgi_get_statuscode(int err); // on MACOS compiling error: mandatory??????

void on_cgi_notified(int fd, int err, char* s, int n) // "s" is the Content inform
{
	debug("On new CGI notified=%s(%d), %d bytes\n", cgi_get_notifyname(err), err, n);
/*
	int i;
	for (i = 0; i < n; i ++)
		printf("%c", s[i]);
	printf("(end)\n");
*/

	// TODO: do a malloc wrapper??? any optimise can applied below???
	unsigned int httplen = 0; // TODO: for out-of-range check before memcpy below
	unsigned int httpoff = 0;

	httplen = n+1024; // 1024 reserved for http fields except "Content"
	char* httpbuff = malloc(httplen);
	if (httpbuff == NULL)
	{
		// TODO: generate "507"????
		error("Out of memory!!");
		return;
	}

	// -----------------------------
	// status-line
	char* status_line[128];
	CGI_StatusCode sta = cgi_get_statuscode(err);
	sprintf(status_line, "%s %d %s\r\n", "HTTP/1.1", sta.sta_code, sta.sta_name);
	debug("Status-line:%s\n", status_line);
	memcpy(httpbuff+httpoff, status_line, strlen(status_line)); // TODO: check out-of-range before copy
	httpoff += strlen(status_line);

	// -----------------------------
	// respond-header
	char* html_field="Server: miniweb\r\nContent-Type: text/html; charset=utf-8\r\n";
	memcpy(httpbuff+httpoff, html_field, strlen(html_field)); // TODO: check out-of-range before copy
	httpoff += strlen(html_field);

	// "CONTENT_LENGTH" not mandatory???

	// -----------------------------
	// content body
	char* html_format = "\r\n"; // Content starts here - Mandatory
								// ,for some format, without webpage cannot identify
	memcpy(httpbuff+httpoff, html_format, strlen(html_format)); // TODO: check out-of-range before copy
	httpoff += strlen(html_format);

	//take it (error case) for example:
	/*
	<html>
		<head>
			<title>404 not found</title>
		</head>
		<body>
			<h2>404 not found</h2>
			<p>Error: the requested is not found</p>
		</body>
	</html>
	*/
	char* text = "<html><head><title>"; // format a Webpage style for error
	if (err != CGI_NOTIFY_OK)
	{
		memcpy(httpbuff+httpoff, text, strlen(text)); // TODO: check out-of-range before copy
		httpoff += strlen(text);

		sprintf(status_line, "%d %s", sta.sta_code, sta.sta_name);
		memcpy(httpbuff+httpoff, status_line, strlen(status_line)); // TODO: check out-of-range before copy
		httpoff += strlen(status_line);

		text = "</title></head><body><h2>";
		memcpy(httpbuff+httpoff, text, strlen(text)); // TODO: check out-of-range before copy
		httpoff += strlen(text);

		memcpy(httpbuff+httpoff, status_line, strlen(status_line)); // TODO: check out-of-range before copy
		httpoff += strlen(status_line);

		text = "</h2><p>Error: ";
		memcpy(httpbuff+httpoff, text, strlen(text)); // TODO: check out-of-range before copy
		httpoff += strlen(text);
	}

	if ((s != NULL) && (n != 0))
	{
		memcpy(httpbuff+httpoff, s, n); // TODO: check out-of-range before copy
		httpoff += n;
	}
	else
	{
		if (err != CGI_NOTIFY_OK)
		{
			text = "No more error information";
			memcpy(httpbuff+httpoff, text, strlen(text)); // TODO: check out-of-range before copy
			httpoff += strlen(text);
		}
		else
		{
			// No "Content" carried
		}
	}

	if (err != CGI_NOTIFY_OK)
	{
		text = "</p></body></html>";
		memcpy(httpbuff+httpoff, text, strlen(text)); // TODO: check out-of-range before copy
		httpoff += strlen(text);
	}

	// -----------------------------
	// Send Http-datagram
	int len = send(fd, httpbuff, httpoff, 0); // TODO: flags
	if (len != httpoff)
	{
		error("Failed to send, err=%d\n", len); // TODO: in case failure turn to another HTTP error sent?!!
		say_errno();
	}
	debug("pid=%d Server responds %d bytes>>\n", getpid(), len);

	free(httpbuff);
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
	debug("On new Trans event=%s(%d), %d bytes\n", trans_get_eventname(evt), evt, l);

	switch (evt)
	{
		case TransEvent_REQUEST_TIMEOUT:
		case TransEvent_RECEIVE_FAILURE:
		case TransEvent_OUT_OF_MEMORY:
		// TODO: fix me
		break;

		case TransEvent_NEWCONNECTION:
		case TransEvent_INCOMING_MSG:
		if ((s == NULL) || (l == 0)) // no actual data carried
		{
			return;
		}

		request_cgi(conn->conn_fd, s, l);  // TODO: i hate to directly give "fd" here. IMPROVE!!
		break;

		case TransEvent_ON_DISCONNECT: // just before its exit
		break;
	}
}