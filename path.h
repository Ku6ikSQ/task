#ifndef PATH_H_SENTRY
#define PATH_H_SENTRY

struct path_aux;
typedef struct path_aux path_aux;

path_aux *path_init();
char change_dir(const char *path, path_aux *aux);
char print_path(path_aux *aux);
char *paths_union(const char *p1, const char *p2);
#endif
