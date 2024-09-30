#include "strlib.h"
#include "memlib.h"
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
        if(quoted && (s[i] == comma_chr) && (s[i-1] != '\\'))
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
	word = realloc(word, sizeof(*word)*(i+1));
	*offset = i;
	return word;
}

static int get_special_char(char id)
{
	switch(id) {
		case 'n':
			return '\n';
		case 't':
			return '\t';
		case '\'':
		case '\"':
		case '\\':
			return id;
		default:
			return -1;
	}
}

char set_special_chars(char *str)
{
	long long i;
	if(!str)
		return -1;
	for(i = 0; str[i] && str[i+1]; i++) {
		int schr;
		if(str[i] != '\\')
			continue;
		schr = get_special_char(str[i+1]);
		if(schr == -1)
			continue;
		str[i] = schr;
		string_shift(&(str[i+1]), -1);
	}
	return 0;
}

static void string_shl_aux(char *str)
{
	while(*str) {
		str[0] = str[1];
		str++;
	}
}

static void string_shr_aux(char *str)
{
	long long i, len = strlen(str);
	char prev = str[0];
	for(i = 1; i <= len; i++) {
		char tmp = prev;
		prev = str[i];
		str[i] = tmp;
	}
	str[len+1] = 0;
}

char string_shift(char *str, long long shift)
{
	unsigned long long i;
	void (*aux)(char *);
	if(!str)
		return -1;
	if(shift < 0) {
		aux = string_shl_aux;
		shift *= -1;
	}
	else
		aux = string_shr_aux;
	for(i = 0; i < shift; i++)
		aux(str);
	return 0;
}

char is_number(int ch)
{
	return (ch >= '0') && (ch <= '9');
}

static char has_prefix(const char *str, const char *prefix)
{
	long long slen, plen, i;
	if(!str || !prefix)
		return 0;
	plen = strlen(prefix);
	slen = strlen(str);
	if(plen > slen)
		return 0;
	for(i = 0; i < plen; i++)
		if(str[i] != prefix[i])
			return 0;
	return 1;
}

const char **get_strings_by_prefix(const char **strs, long long count,
	const char *prefix)
{
	long long i, size;
	const char **matches, **tmp;
	if(!strs || !prefix || (count <= 0))
		return NULL;
	size = sizeof(*matches)*(count+1);
	matches = malloc(size);
	if(*prefix == 0) {
		memcpy(matches, strs, size);
		return matches;
	}
	tmp = matches;
	for(i = 0; i < count; i++) {
		if(!has_prefix(strs[i], prefix))
			continue;
		*tmp = strs[i];
		tmp++;
	}
	*tmp = NULL;
	count = tmp-matches;
	matches = realloc(matches, sizeof(*matches)*(count+1));
	return matches;
}
