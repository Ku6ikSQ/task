#ifndef STRLIB_H_SENTRY
#define STRLIB_H_SENTRY

char **get_tokens(const char *s);
char *strings_concatenate(const char *s, ...);
long long string_length(const char *s);
char *string_duplicate(const char *s);
char *string_catenate(char *dest, const char *src);
char *get_word(const char *s, long long *offset);
//$char string_shl(char *str, unsigned long long shift);
char set_special_chars(char *str);
char string_shift(char *str, long long shift);
char is_number(int ch);
const char **get_strings_by_prefix(const char **strs, long long count,
	const char *prefix);

#endif
