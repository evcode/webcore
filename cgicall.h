#ifndef CGICALL_H
#define CGICALL_H

#include "util.h"

#define CGI_ERROR "CGI_ERROR"

typedef enum // TODO: do edit "cgi_get_notifyname()" if update here!!
{
	CGI_NOTIFY_OK = 0,

	CGI_NOTIFY_BAD_ENV,
	CGI_NOTIFY_CGI_ERR,

	CGI_NOTIFY_TIMEOUT,
	CGI_NOTIFY_INVALID_DATA,

	CGI_NOTIFY_UNKNOWN_ERR
} CGI_NOTIFY;

#endif