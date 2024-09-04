#include "params.h"
#include <stdarg.h>
#include <string.h>

enum {
	param_separator = '=',
};

long long param_search(const char *params[], const char *paramname, ...)
{
	long long i, len;
	va_list vl;
	if(!params || !paramname)
		return -1;
	va_start(vl, paramname);
	do {
		len = strlen(paramname);
		for(i = 0; params[i]; i++) {
			char *separator = strchr(params[i], param_separator);
			long long curlen = 0;
			if(separator)
				curlen = separator-params[i];
			else
				curlen = strlen(params[i]);
			if(curlen != len)
				continue;
			if(strncmp(params[i], paramname, len) == 0)
				return i;
		}
	} while ((paramname = va_arg(vl, const char *)) != NULL);
	va_end(vl);
	return -1;
}
