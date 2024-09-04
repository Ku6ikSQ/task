#include "strlib.h"
#include <stdlib.h>
#include <string.h> 
#include <stdarg.h>

enum {
    comma_chr = '"',
    space_chr = ' '
};

static char is_space(char c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v';
}

static char string_is_empty(const char *s)
{
    if(s)
        return 0;
    while(*s) {
        if(!is_space(*s))
            return 0;
        s++;
    }
    return 1;
} 

static char *get_token(const char *s, long long *offset)
{
    char *token;
    long long i, len;
    const char *tmp = s;
    char quoted = 0;
    *offset = 0;
    if(string_is_empty(s))
        return NULL;
    if(*s == space_chr) {
        while(is_space(*s))
            s++;
    }
    if(*s == comma_chr) {
        quoted = 1;
        s++;
    }
    len = strlen(s);
    for(i = 0; i < len; i++) {
        if(quoted && s[i] == comma_chr)
            break;
        if(!quoted && s[i] == space_chr)
            break;
    }
    if(i == 0)
        return NULL;
    token = malloc(sizeof(*token)*(i+1));
    memcpy(token, s, i);
    token[i] = 0;
    *offset = s-tmp+i;
    return token;
}

char **get_tokens(const char *s)
{
    char **items;
    char *token;
    long long i, offset;
    if(!s)
        return NULL;
    items = malloc(sizeof(*items)*(strlen(s)+1));
    i = 0;
    while((token = get_token(s, &offset)) != NULL) {
        items[i] = token;
        s += offset;
        i++;
    }
    items[i] = NULL;
    return items;
}

static long long strings_length(const char *fstr, va_list *vl)
{
    long long result;
    char *s;
	if(!fstr || !vl)
		return -1;
	result = strlen(fstr);
    while((s = va_arg(*vl, char *)) != NULL)
        result += strlen(s);
    return result;
}

char *strings_concatenate(const char *s, ...)
{
    char *result, *tmp;
    va_list vl;
    long long rlen;
    if(!s)
        return NULL;
    va_start(vl, s);
    rlen = strings_length(s, &vl);
    va_end(vl);
	if(rlen <= 0)
		return NULL;
    va_start(vl, s);
    result = malloc(sizeof(*result)*(rlen+1));
    tmp = result;
    do {
        long long len = strlen(s);
        memcpy(tmp, s, len);
        tmp += len;
    } while((s = va_arg(vl, char *)) != NULL);
    *tmp = 0;
    va_end(vl);
    return result;
}

long long string_length(const char *s)
{
    const char *tmp;
    if(!s)
        return 0;
    tmp = s;
    while(*s)
        s++;
    return s-tmp;
}

char *string_duplicate(const char *s)
{
    char *result;
    long long len, i;
    if(!s)
        return NULL;
    len = strlen(s);
    result = malloc(sizeof(*s)*(len+1));
    for(i = 0; s[i]; i++)
        result[i] = s[i];
    result[i] = 0;
    return result;
}

char *string_catenate(char *dest, const char *src)
{
    long long len, i;
    if(!dest || !src)
        return NULL;
    len = strlen(src);
    dest += strlen(dest);
    for(i = 0; i < len; i++)
        dest[i] = src[i];
    dest[i] = 0;
    return dest;
}

char *get_word(const char *s, long long *offset)
{
	char *word;
	long long i, len;
	if(!s)
		return NULL;
	len = strlen(s);
	word = malloc(sizeof(*word)*(len+1));
	for(i = 0; (i < len) && (s[i] != ' '); i++)
		word[i] = s[i];
	word[i] = 0;
	word = realloc(word, i+1);
	*offset = i;
	return word;
}

#if 1
static void string_shl_aux(char *str)
{
	while(*str) {
		str[0] = str[1];
		str++;
	}
}

char string_shl(char *str, unsigned long long shift)
{
	unsigned long long i;
	if(!str)
		return -1;
	for(i = 0; i < shift; i++)
		string_shl_aux(str);
	return 0;
}
#endif
