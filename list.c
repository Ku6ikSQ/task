#include "list.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

enum { default_lst_size = 512 };

static void lst_init(struct list **plst)
{
	struct list *lst;
	*plst = malloc(sizeof(*lst));
	lst = *plst;
	lst->count = 0;
	lst->size = default_lst_size;
	lst->words = malloc(sizeof((*(lst->words)))*lst->size);
}

char list_append(struct list *lst, const char *word)
{
	char *copy;
	if(!lst || !word)
		return -1;
	if(lst->count == lst->size) {
		lst->size *= 2;
		lst->words = realloc(lst->words, sizeof(*(lst->words))*lst->size);
	}
	copy = strdup(word);
	lst->words[lst->count] = copy;
	lst->count++;
	return 0;
}

struct list *list_create(const char *word, ...)
{
	struct list *lst;
	va_list vl;
	char ok;
	lst_init(&lst);
	if(!word)
		return lst;
	va_start(vl, word);
	do {
		ok = list_append(lst, word);
		if(ok == -1)
			break;
	} while((word = va_arg(vl, const char *)) != NULL);
	va_end(vl);
	if(ok == -1) {
		list_free(lst);
		return NULL;
	}
	return lst;
}   

void list_free(struct list *lst)
{
	long long i;
	for(i = 0; i < lst->count; i++)
		free(lst->words[i]);
	free(lst->words);
	free(lst);
}
