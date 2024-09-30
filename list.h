#ifndef DICTIONARY_H_SENTRY
#define DICTIONARY_H_SENTRY
struct list {
	char **words; 
	long long count;
	long long size;
};

struct list *list_create(const char *word, ...);
char list_append(struct list *lst, const char *word);
void list_free(struct list *lst);
#endif
