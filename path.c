#include "path.h"
#include <stdlib.h>
#include <string.h>

enum {
	path_delim = '/',
};

char *get_shortpath(const char *root, const char *path)
{
	char *result;
	char *match;
	long long len;
	match = strstr(path, root);
	if(!match)
		return NULL;
	path += strlen(root);
	len = strlen(path);
	result = malloc(sizeof(*result)*(len+1));
	memcpy(result, path, len+1);
	return result;
}

char *paths_union(const char *path1, const char *path2)
{
	long long len1, len2;
	char *result;
	if(!path1 || !path2)
		return NULL;
	len1 = strlen(path1);
	len2 = strlen(path2);
	result = malloc(sizeof(*result)*(len1+len2+1+1)); /* +1:'/' +1:'\0' */
	memcpy(result, path1, len1);
	if(path1[len1-1] != path_delim) {
		result[len1] = path_delim;
		result[len1+1] = 0;
	}
	strcat(result, path2);
	return result;
}
