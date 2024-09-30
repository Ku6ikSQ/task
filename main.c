#include "readline.h"
#include "shell.h"
#include "params.h"
#include <stdio.h>

#define TASKP_VERSION "task: v1.1.7\n"

static int process_version_param()
{
	fputs(TASKP_VERSION, stdout);
	return 0;
}

static int process_params(int argc, const char *argv[], char *terminate)
{
	long long pindex;
	*terminate = 0;
	if((argc-1) < 0)
		return 0;
	pindex = param_search(argv, "-v", "--version", NULL);
	if(pindex != -1) {
		*terminate = 1;
		return process_version_param();
	}
	return 0;
}

int main(int argc, const char *argv[])
{
	char status, terminate;
	status = process_params(argc, argv, &terminate);
	if(terminate)
		return status;
	readline_start();
    status = shell_run();
	readline_end();
    return status;
}
