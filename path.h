#ifndef PATH_H_SENTRY
#define PATH_H_SENTRY

char *get_shortpath(const char *root, const char *path);
char *paths_union(const char *path1, const char *path2);
char path_extend(char *path, const char *ext);
char is_abspath(const char *path);
#endif
