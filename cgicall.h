#ifndef CGICALL_H
#define CGICALL_H

#include "util.h"

#define CGI_ERROR "CGI_ERROR"

typedef enum
{
	// TODO: do edit "cgi_get_notifyname()" if update here!!
	// TODO: do not forget to map to http-statuscode in cgi_get_statuscode()!!!!!!!!
	CGI_NOTIFY_OK = 0,

	CGI_NOTIFY_BAD_ENV,
	CGI_NOTIFY_CGI_ERR,
	CGI_NOTIFY_BAD_REQUEST,
	CGI_NOTIFY_TOOLONG_REQUEST,
	CGI_NOTIFY_METHOD_NOT_SUPPORT,

	CGI_NOTIFY_TIMEOUT,
	CGI_NOTIFY_INVALID_DATA,

	CGI_NOTIFY_UNKNOWN_ERR
} CGI_NOTIFY;

typedef struct
{
	int   sta_code;
	char* sta_name;
	char* sta_desc;
} CGI_StatusCode;

typedef void (*CgiListen_CB)(void* id, int err, char* msg, int len);

typedef struct cgi_listner
{
	void* listen_id;
	CgiListen_CB listen_cb;

	void* metadata;

	struct cgi_listner* next_listen;
} CgiListener;

#include <pthread.h>
typedef struct
{
	CgiListener* list;
	int list_num;

	pthread_mutex_t list_mutex;
} CgiListenerList;

CgiListener* cgi_addlisten(void* id, CgiListen_CB cb);
void cgi_request(CgiListener* l, const char* msg, int msglen);
void cgi_rmlisten(CgiListener* l);
CGI_StatusCode cgi_map_httpstatus(int err);
char* cgi_get_notifyname(int evt);
#endif