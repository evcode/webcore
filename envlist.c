#include "util.h"

typedef struct
{
	char* http_field;
	char* env_name;
} EnvMapping;

EnvMapping cgi_envmapping[] = 
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

#define TRNAS_ENV_MAX 128

static int envnum = 0;
static char* envs[TRNAS_ENV_MAX];

static BOOL addenv(const char* key, const char* value) // Combine 2 str as the format "<key>=<value>"
{
	if (envnum == TRNAS_ENV_MAX-1) // the last one is reserved to NULL as per CGI spec.???
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

	char* env = malloc(len1+len2);
	memcpy(env, key, len1);
	env[len1-1]='='; // replace '/0' at end of "key"
	memcpy(env+len1, value, len2);

	// added one
	envs[envnum] = env;
	envnum ++;

	debug("ENV: added env. variant [%s]\n", env);
	return TRUE;
}

char** envlist_init() // It returns a string (char*) array
{
	envnum = 0;
	memset(envs, 0, sizeof(envs)); // NOTE: make sure last item is NULL
	// TOOD: change to malloc: envs = malloc(TRNAS_ENV_MAX*sizeof(char*)); // the array has TRNAS_ENV_MAX datas of (char*) type

	return envs;
}

int envlist_num()
{
	return envnum;
}

/*
The input "str" format is: "<http filed>: <filed value>"
, the result should be env "<env name>=<field value> added into env list"
*/
BOOL envlist_add(const char* str)
{
	int i;

	// Check HTTP Request-line firslty
	for (i = 0; i < sizeof(http_methods)/sizeof(http_methods[0]); i ++)
	{
		if (str_startwith(str, http_methods[i]))
		{
			char* k = "REQUEST_METHOD";
			char* v = http_methods[i];

			return addenv(k, v);
		}
	}

	// Start scan the Header fields...
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
		
		return addenv(env_name, &str[valstart]);
	}

	error("ENV: failed to add <%s>\n", str);
	return FALSE;
}

void envlist_dump(char** e, int l)
{
	debug("Environment variants:\n");
	int i;
	for (i = 0; i < l; i ++)
	{
		printf("%s\n", e[i]);
	}
	printf("\n");
}
// TODO: release all