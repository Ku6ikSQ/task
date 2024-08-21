#include "params.h"
#include <string.h>

enum {
	param_separator = '=',
};

long long param_search(const char *params[], const char *param)
{
	long long i, len;
	if(!params || !param)
		return -1;
	len = strlen(param);
	for(i = 0; params[i]; i++) {
		char *separator = strchr(params[i], param_separator);
		long long curlen;
		if(separator)
			curlen = separator-params[i];
		else
			curlen = strlen(params[i]);
		if(curlen != len)
			continue;
		if(strncmp(params[i], param, len) == 0)
			return i;
	}
	return -1;
}
