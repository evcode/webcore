#include "envlist.h"
#include "util.h"
#include "vheap.h"

static int vheap_env = 3; // TODO: a temp coding to get vheap id

#define HTTP_URL_LEN 256
#define HTTP_LOCAL_PATH "../www" // TODO: move into CGI program to remain less dependency!!

typedef struct
{
	char* http_field;
	char* env_name;
} EnvMapping;

static EnvMapping cgi_envmapping[] = // mapping of HTTP fields NOT including Request-line
{
	{"User-Agent",			"HTTP_USER_AGENT"},
	{"Accept-Language",		"ACCEPT_LANGUAGE"},
	{"Accept-Encoding",		"ACCEPT_ENCODING"},
	{"Accept",				"HTTP_ACCEPT"},
	{"",""},
	{"",""},
	{"",""},
	{"",""},
	{"",""},
	{"",""},
	{"",""},
	{"",""}
};

static char* http_methods[] = {"GET", "POST", "PUT", "DELETE"};

static BOOL addenv(Envlist* l, const char* key, const char* value) // Combine 2 str as the format "<key>=<value>"
{
	if (l->envnum == TRNAS_ENV_MAX-1) // the last one is reserved to NULL as per CGI spec.???
	{
		error("ENV: reached the max number of environment variants!!\n");
		return FALSE;
	}

	int len1 = strlen(key)+1;
	int len2 = strlen(value)+1;
	if ((len1 <= 1) || (len2 <= 1))
	{
		error("ENV: invalid env configuration!!\n");
		return FALSE;
	}

	char* env = vheap_malloc(vheap_env, len1+len2);
	memcpy(env, key, len1);
	env[len1-1]='='; // replace '/0' at end of "key"
	memcpy(env+len1, value, len2);

	// added one
	l->envs[l->envnum] = env;
	l->envnum ++;

	debug("ENV: added env. variant [%s]\n", env);
	return TRUE;
}

// TODO: where to destory!!
Envlist* envlist_init() // It returns a string (char*) array
{
	Envlist* envlist = vheap_malloc(vheap_env, sizeof(Envlist));
	memset(envlist, 0, sizeof(Envlist)); // NOTE: make sure last item is NULL

	return envlist;
}

void envlist_uninit(Envlist* envlist)
{
	if (envlist == NULL)
	{
		debug("Warning: a NULL envlist to release");
		return;
	}

	debug("ENV: unint the envlist, which had %d envs\n", envlist->envnum);

	// release all env. variables in the list
	int i;
	for (i = 0; i < envlist->envnum; i ++)
	{
		vheap_free(vheap_env, envlist->envs[i]);
	}

	// release the envlist object
	vheap_free(vheap_env, envlist);
	envlist = NULL;
}

CGI_NOTIFY envlist_add(Envlist* l, const char* str)
{
	int i;

	// Check HTTP Request-line firslty
	/*
	The input "str" format here, such as "Get /index HTTP/1.1"
	*/
	/* TODO: merge the impl into Header processing as below????
	For example:
	{"Get",			"REQUEST_METHOD"},
	{"POST",		"REQUEST_METHOD"},
	...
	*/
	for (i = 0; i < sizeof(http_methods)/sizeof(http_methods[0]); i ++)
	{
		if (str_startwith(str, http_methods[i]))
		{
			char* k = "REQUEST_METHOD";
			char* v = http_methods[i];

			if (strcmp(v, "GET") == 0) // index 0 is "GET"
			{
				char* substr = str+4; // skip "GET "
				int pathlen = 0;

				char* end = strstr(substr, " ");
				if (end == NULL)
				{
					debug("ERROR: failed to parse URL\n");
					return CGI_NOTIFY_BAD_REQUEST;
				}
				else
				{
					pathlen = end - substr;
				}

				int localpath_len = strlen(HTTP_LOCAL_PATH); // TODO: left the local absolute Base in CGI program, not here!!!
				if (pathlen >= HTTP_URL_LEN-localpath_len) // reserve for local path and '\0'
				{
					debug("ERROR: URL is too long\n");
					return CGI_NOTIFY_TOOLONG_REQUEST;
				}
				else
				{
					char url[HTTP_URL_LEN];
					memcpy(url, HTTP_LOCAL_PATH, localpath_len); // "../www"
					memcpy(url+localpath_len, substr, pathlen);  // "../www/index.html"
					url[localpath_len+pathlen]='\0';
					debug("Request URL(%d)=%s\n", pathlen, url);

					addenv(l, "PATH_INFO", url);

					// TODO: http version
				}
			}
			else // TODO: POST and others...
			{
				debug("ERROR: not support the method(%s)\n", v);
				return CGI_NOTIFY_METHOD_NOT_SUPPORT;
			}

			addenv(l, k, v);
			return CGI_NOTIFY_OK;
		}
	}

	// Start scan the Header fields...
	/*
	The input "str" format as Header is: "<http filed>: <filed value>"
	, the result should be env "<env name>=<field value> added into env list"
	*/
	char* env_name = NULL;

	// Get Http-Env mapping
	for (i = 0; i < sizeof(cgi_envmapping)/sizeof(cgi_envmapping[0]); i ++)
	{
		if (str_startwith(str, cgi_envmapping[i].http_field))
		{
			env_name = cgi_envmapping[i].env_name;
			break;
		}
	}

	// Get Env's value from a HTTP line, and add into the env list
	if (env_name != NULL)
	{
		int valstart = strlen(cgi_envmapping[i].http_field) + 2; // skip the following ':' and a space in a HTTP line
		
		addenv(l, env_name, &str[valstart]);
		return CGI_NOTIFY_OK;
	}

	// Here regard it as a Warning, return "OK"
	debug("Warning: not identified <%s>\n", str);
	return CGI_NOTIFY_OK;
}

char* envlist_getenv(Envlist* list, char* key)
{
	if (list == NULL)
		return NULL;

	char** e = list->envs;
	int l    = list->envnum;

	int i;
	for (i = 0; i < l; i ++)
	{
		if (str_startwith(e[i], key))
		{
			return e[i]; // return "<key>=xxxxx"
		}
	}

	return NULL;
}

void envlist_dump(Envlist* list)
{
	char** e = list->envs;
	int l    = list->envnum;

	debug("----------------------------------\n");
	debug("Environment variants:\n");
	int i;
	for (i = 0; i < l; i ++)
	{
		debug("%s\n", e[i]);
	}
	debug("\n");
}
// TODO: release all
