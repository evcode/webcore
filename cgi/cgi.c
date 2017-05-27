#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h> // for "open"
#include <unistd.h> // stdin/out fd
#include <sys/stat.h>

static int ishex(int x)
{/* Copyright: http://rosettacode.org/wiki/URL_decoding#C */
	return (x >= '0' && x <= '9') ||
		(x >= 'a' && x <= 'f') ||
		(x >= 'A' && x <= 'F');
}

static int url_decode(const char *s, char *dec)
{/* Copyright: http://rosettacode.org/wiki/URL_decoding#C */
	char *o;
	const char *end = s + strlen(s);
	int c;

	for (o = dec; s <= end; o++)
	{
		c = *s++;
		if (c == '+')
			c = ' ';
		else if (c == '%' && (!ishex(*s++) || !ishex(*s++) ||
			!sscanf(s - 2, "%2x", &c)))
			return -1;

		if (dec)
			*o = c;
	}

	return (o - dec);
}

static void cgierr(int err, char* desc)
{
	char cgistr[256]; // format: CGI_ERROR:<http status code>:<err description>"

	sprintf(cgistr, "CGI_ERROR:%d:%s", err, desc);
	printf(cgistr);
}

static char* cgi_envs[] = 
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
		// convert decoding of url: http://www.w3school.com.cn/tags/html_ref_urlencode.html
		char cpath[256];
		memset(cpath, 0, sizeof(cpath));

		if (url_decode(path, cpath) < 0) // process the char starting with "%"
		{
			cgierr(400, "Invalid URL information");

			return 1;
		}
		strcpy(path, cpath);

		// retrieve the file
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
