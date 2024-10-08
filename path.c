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

char path_extend(char *path, const char *ext)
{
	long long len;
	if(!path || !ext)
		return -1;
	len = strlen(path);
	if(path[len-1] != path_delim) {
		path[len] = path_delim;
		path[len+1] = 0;
	}
	strcat(path, ext);
	return 0;
}

char *paths_union(const char *path1, const char *path2)
{
	long long len1, len2;
	char ok;
	char *result;
	if(!path1 || !path2)
		return NULL;
	len1 = strlen(path1);
	len2 = strlen(path2);
	result = malloc(sizeof(*result)*(len1+len2+1+1)); /* +1:'/' +1:'\0' */
	strcpy(result, path1);
	ok = path_extend(result, path2);
	if(ok != 0) {
		free(result);
		return NULL;
	}
	return result;
}

char is_abspath(const char *path)
{
	return path[0] == path_delim;
}
