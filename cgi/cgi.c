#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h> // for "open"
#include <unistd.h> // stdin/out fd
#include <sys/stat.h>

void cgierr(int err, char* desc)
{
	char cgistr[256]; // format: CGI_ERROR:<http status code>:<err description>"

	sprintf(cgistr, "CGI_ERROR:%d:%s", err, desc);
	printf(cgistr);
}

char* cgi_envs[] = 
{
	"REQUEST_METHOD",
	"PATH_INFO",
	"HTTP_USER_AGENT",
	"ACCEPT_LANGUAGE",
	"ACCEPT_ENCODING",
	"HTTP_ACCEPT",
};

static char readbuff[4*1024];
static unsigned int readlen = 0;

int main(int argc, char* argv[])
{
	int i;

#if 0
	// dump command line
	for (i = 0; i < argc; i ++)
	{
		printf("%s ", argv[i]);
	}
	printf("\n\n");

	// dump env. variants
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
#endif

	char* path = getenv("PATH_INFO");
	if (path == NULL)
	{
		cgierr(404, "No path information");
	}
	else
	{
		struct stat info;
		stat(path, &info);
		if (S_ISDIR(info.st_mode))
			path = "../www/index.html"; // TODO: absolute base + Relative addr

		int fd = open(path, O_RDONLY);
		if (fd < 0)
		{
			cgierr(404, "Not found");
		}
		else
		{
			do
			{
				readlen = read(fd, readbuff, sizeof(readbuff)); // TODO: it cannot handle a local Folder type!!!
				if (readlen > 0)
					write(STDOUT_FILENO, readbuff, readlen);

			} while (readlen == sizeof(readbuff));

			close(fd);
		}
	}

	return 0;
}
