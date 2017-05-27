#ifndef CGICALL_H
#define CGICALL_H

#include "util.h"
#include "envlist.h"

typedef struct
{
	int   sta_code;
	char* sta_name;
	char* sta_desc;
} CGI_StatusCode;

typedef struct cgi_request_t
{
	int cgi_type; // http, etc...

	Envlist* cgi_envs;
} CgiRequest;

typedef struct cgi_response_t
{
	int cgi_type; // http, etc...

	char* msg;
	int msglen;
	char url[256];
} CgiResponse;

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