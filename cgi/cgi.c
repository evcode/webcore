#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* cgi_envs[] = 
{
	"REQUEST_METHOD",
	"HTTP_USER_AGENT",
	"ACCEPT_LANGUAGE",
	"ACCEPT_ENCODING",
	"HTTP_ACCEPT",
	"PATH" // only for test here
};

int main(int argc, char* argv[])
{
	int i;

	for (i = 0; i < argc; i ++)
	{
		printf("%s ", argv[i]);
	}
	printf("\n\n");

	// -----------------------------
	for (i = 0; i < sizeof(cgi_envs)/sizeof(cgi_envs[0]); i ++)
	{
		char* key = cgi_envs[i];
		printf("%s=", key);

		char* val = getenv(key);
		if (val != NULL)
		{
			printf("%s\n", val);
		}
		else
		{
			printf("<not set>\n");
		}
	}

	return 0;
}