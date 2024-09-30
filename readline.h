#ifndef READLINE_H_SENTRY
#define READLINE_H_SENTRY
#include "list.h"

enum { input_max_size = 4096 };

struct readline_list {
	struct list *value;
	char (*check_rule)(const char *);
	void (*before_action)(struct list *, const char *);
};

struct input {
	char value[input_max_size];
	int shift;
};

typedef void (*readline_before_action_t)(void *);
void input_init(struct input *input);
char *readline(struct input *input, const struct readline_list **lists,
	readline_before_action_t fn, void *usrdata);
int readline_start();
int readline_end();
#endif
