#ifndef STRLIB_H_SENTRY
#define STRLIB_H_SENTRY

char **get_tokens(const char *s);
char *strings_concatenate(const char *s, ...);
long long string_length(const char *s);
char *string_duplicate(const char *s);
char *string_catenate(char *dest, const char *src);
char *get_word(const char *s, long long *offset);

#endif
