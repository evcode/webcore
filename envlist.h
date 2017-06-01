#ifndef ENVLIST_H
#define ENVLIST_H

/* TODO:
	actually the error should belong to Envlist not CGI - 
		remain less dependency as much as possible!! */
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

#ifndef DEBUG_ENVLIST
#undef debug(x...)

#define debug(x...) do {} while (0)
#endif

#define TRNAS_ENV_MAX 32
typedef struct
{
	char* envs[TRNAS_ENV_MAX];
	int envnum;
} Envlist;

Envlist* envlist_init();
void envlist_uninit(Envlist* envlist);
CGI_NOTIFY envlist_add(Envlist* l, const char* str);
char* envlist_getenv(Envlist* list, char* key);
void envlist_dump(Envlist* list);
#endif