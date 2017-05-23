#ifndef ENVLIST_H
#define ENVLIST_H

#include "cgicall.h" // for notify error "CGI_NOTIFY"

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
CGI_NOTIFY envlist_add(Envlist* l, const char* str);
void envlist_dump(Envlist* list);

#endif