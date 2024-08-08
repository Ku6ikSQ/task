#ifndef FSLIB_H_SENTRY
#define FSLIB_H_SENTRY
#include <stdio.h>

char create_block(const char *dirname, const char *filename, int *pfd);
char remove_dir(const char *name);
char *fgets_m(char *s, int size, FILE *stream);
#endif
