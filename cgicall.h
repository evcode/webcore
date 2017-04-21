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

typedef struct
{
	char* sta_code;
	char* sta_name;
	char* sta_desc;
} CGI_StatusCode;

CGI_StatusCode http_statuslist[] = 
{
	/*
	http://www.baidu.com/link?url=sjNCIH7jFNEVlq8lA-vPCkfJJ-qyAGLwC6RqKYUs6srxCCna9khV0eb_qpGjB4gSr46dZj9WTPc07w5M2xqCxZE5LD_KFDVcmJUkxIih1eEzkIf1QNRI_S_sF6P9SuTD&wd=&eqid=ebc44488000276c70000000458f9b21e
	*/

	// Message
	{"100", "Continue", ""},

	// Success
	{"200", "OK", ""},
	{"201", "Created", ""},
	{"202", "Accepted", ""},
	{"204", "No Content", ""},
	{"205", "Reset Content", ""},

	// Redirection
	{"302", "Move temporarily", ""},
	{"303", "See Other", ""},
	{"305", "Use Proxy", ""},

	// Client request error
	{"400", "Bad Request", ""},
	{"401", "Unauthorized", ""},
	{"403", "Forbidden", ""},
	{"404", "Not Found", ""},
	{"405", "Method Not Allowed", ""},
	{"406", "Not Acceptable", ""},
	{"408", "Request Timeout", ""},
	{"413", "Request Entity Too Large", ""},
	{"414", "Request-URI Too Long", ""},
	{"415", "Unsupported Media Type", ""},

	// Server error
	{"500", "Internal Server Error", ""},
	{"501", "Not Implemented", ""},
	{"502", "Bad Gateway", ""},
	{"503", "Service Unavailable", ""},
	{"504", "Gateway Timeout", ""},
	{"505", "HTTP Version Not Supported", ""},

	// unknown
	{"999", "unknown-status", ""},
};

#endif