#include "readline.h"
#include "strlib.h"
#include "memlib.h"
#include "fslib.h"
#include "list.h"
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

void input_init(struct input *input)
{
	input->value[0] = 0;
	input->shift = 0;
}

static char *get_prefix(struct input *input)
{
	long long len, i;
	char *inval;
	if(!input)
		return NULL;
	inval = strdup(input->value);
	len = strlen(inval);
	inval[len+input->shift] = 0;
	for(i = len-1+input->shift; i >= 0; i--) {
		if(inval[i] == ' ') {
			char *result = strdup(&(inval[i+1]));
			free(inval);
			return result;
		}
	}
	return inval;
}

enum {
	tab_chr 		= '\t',
	backspace_chr 	= '\b',
	newline_chr		= '\n',
	del_chr 		= 127,
};

typedef enum 
	{ not_arrow, arrow_top, arrow_bottom, arrow_right, arrow_left } arrow_t;

static arrow_t get_arrow_type(char keycode)
{
	switch(keycode) {
		case 'A':
			return arrow_top;
		case 'B':
			return arrow_bottom;
		case 'C':
			return arrow_right;
		case 'D':
			return arrow_left;
		default:
			return not_arrow;
	}
}

static void process_arrow_left(struct input *input)
{
	if(strlen(input->value) == abs(input->shift))
		return;
	(input->shift)--;
	putchar('\b');
}

static void process_arrow_right(struct input *input)
{
	long long inlen = strlen(input->value);
	char next_char;
	if(input->shift == 0)
		return;
	(input->shift)++;
	next_char = input->value[inlen-1+input->shift];
	putchar(next_char);
}

static void process_arrow(arrow_t artype, struct input *input)
{
	switch(artype) {
		case arrow_left:
			process_arrow_left(input);
			break;
		case arrow_right:
			process_arrow_right(input);
			break;
		case arrow_top:
		case arrow_bottom:
		case not_arrow:
			break;
	}
}

static char is_end_process(arrow_t artype, struct input *input)
{
	long long len = strlen(input->value);
	char nxtch;
	char addcond = 1;
	switch(artype) {
		case arrow_left:
			nxtch = input->value[len-1+input->shift];
			addcond = input->shift == -1*len;
			break;
		case arrow_right:
			nxtch = input->value[len+input->shift];
			addcond = input->shift == 0;
			break;
		case arrow_top:
		case arrow_bottom:
		case not_arrow:
			break;
	}
	return addcond || (nxtch == ' ') || (nxtch == 0);
}

static void process_special_arrow(arrow_t artype, struct input *input)
{
	void (*fn)(struct input *) = NULL;
	switch(artype) {
		case arrow_left:
			fn = process_arrow_left;
			break;
		case arrow_right:
			fn = process_arrow_right;
			break;
		case arrow_top:
		case arrow_bottom:
		case not_arrow:
			break;
	}
	if(!fn)
		return;
	do {
		fn(input);
	} while(!is_end_process(artype, input));
}

static void process_escape_sequence(char *seq, struct input *input,
	int *offset)
{
	arrow_t artype;
	*offset = 0;
	if((*seq) != 27)
		return;
	seq++;
	*offset = 1;
	if(!seq[0] || !seq[1])
		return;
	*offset = 2;
	artype = get_arrow_type(seq[1]);
	if(artype != not_arrow) {
		process_arrow(artype, input);
		return;
	}
	if(seq[0] == '[')
		(*offset)++;
	if(is_number(seq[1]))
		(*offset)++;
	if(seq[2] != ';')
		return;
	(*offset) += 2;
	artype = get_arrow_type(seq[4]);
	process_special_arrow(artype, input);
}

static void remove_char(struct input *input)
{
	long long len = strlen(input->value);
	int wcount, i;
	char *dest;
	if(len == 0 || (len+input->shift) == 0)
		return;
	printf("\b \b");
	dest = &(input->value[(len-1)+input->shift]);
	string_shift(dest, -1);
	wcount = printf("%s ", dest);
	for(i = 0; i < wcount; i++)
		putchar('\b');
}

static void update_input(char ch, struct input *input)
{
	long long len = strlen(input->value);
	char *dest;
	int wcount, i;
	if(len >= sizeof(input->value))
		return;
	dest = input->value+len+input->shift;
	string_shift(dest, 1);
	*dest = ch;
	if(input->shift == 0) {
		putchar(ch);
		return;
	}
	wcount = printf("%s", dest);
	for(i = 0; i < wcount-1; i++)
		putchar('\b');
}

static void autocomplete_input(struct input *input, const char *prefix,
	const char *match)
{
	long long inlen = strlen(input->value);
	long long mlen = strlen(match);
	long long plen = strlen(prefix);
	long long wcount, i;
	char shifted;
	char *dest = &(input->value[inlen+input->shift]);
	if(inlen+mlen-plen >= sizeof(input->value))
		return;
	shifted = input->shift != 0;
	if(shifted)
		string_shift(dest, mlen-plen);
	memcpy(dest, &(match[plen]), mlen-plen);
	if(!shifted)
		dest[mlen-plen] = 0; /* to set null-terminating str */
	wcount = printf("%s", dest);
	for(i = 0; i < wcount-(mlen-plen); i++)
		putchar('\b');
}

static void print_matches(struct input *input, const char **matches)
{
	putchar('\n');
	while(*matches) {
		if((strcmp(*matches, "..") == 0) || (strcmp(*matches, ".") == 0)) {
			matches++;
			continue;
		}
		printf("%s\n", *matches);
		matches++;
	}
}

static void print_input(struct input *input)
{
	long long i;
	printf("%s", input->value);
	for(i = input->shift; i < 0; i++)
		putchar('\b');
}

static void autocomplete_run(struct input *input,
	const struct readline_list **lists, readline_before_action_t fn,
	void *usrdata)
{
	struct list *list = NULL;
	const char **matches;
	char *prefix;
	char *prefix_init;
	long long i;
	if(!input || !lists)
		return;
	prefix = get_prefix(input);
	prefix_init = prefix;
	for(i = 1; lists[i]; i++) {
		if(!lists[i]->check_rule || !lists[i]->before_action)
			continue;
		if(lists[i]->check_rule(prefix)) {
			lists[i]->before_action(lists[i]->value, prefix);
			list = lists[i]->value;
			prefix = get_shortname(prefix_init);
			break;
		}
	}
	if(!list)
		list = lists[0]->value;
	matches = get_strings_by_prefix((const char **)list->words,
		list->count, prefix);
	if(!matches || !matches[0])
		goto quit;
	if(matches[0] && !matches[1]) {
		autocomplete_input(input, prefix, matches[0]);
		goto quit;
	}
	print_matches(input, matches);
	fn(usrdata);
	print_input(input);
quit:
	free(matches);
	free(prefix_init);
	if(prefix_init != prefix)
		free(prefix);
}

static void finish_input(struct input *input, char *exit)
{
	long long shift = input->shift;
	input->shift = 0;
	update_input('\n', input);
	input->shift = shift;
	*exit = 1;
}

static void clear_input(struct input *input)
{
	long long i, len = strlen(input->value);
	input->value[0] = 0;
	for(i = 0; i < len; i++)
		printf("\b \b");
}

static void remove_word(struct input *input)
{
	char ch;
	do {
		remove_char(input);
		ch = input->value[strlen(input->value)-1+input->shift];
	} while((ch != ' ') && (ch != 0));
}

static void move_to_begin(struct input *input)
{
	long long len = strlen(input->value);
	while(input->shift != -1*len)
		process_arrow_left(input);
}

static void move_to_end(struct input *input)
{
	while(input->shift < 0)
		process_arrow_right(input);
}

static char is_processed_char(char ch)
{
	return (ch > 27) || (ch == 1) || (ch == 5) || (ch == 21) || (ch == 23) ||
		(ch == '\n') || (ch == '\t') || (ch == '\b');
} 

static void process_char(char ch, struct input *input,
	const struct readline_list **lists, char *exit, readline_before_action_t fn,
	void *usrdata)
{
	*exit = 0;
	if(!is_processed_char(ch))
		return;
	switch(ch) {
		case newline_chr:
			finish_input(input, exit);
			break;
		case tab_chr:
			autocomplete_run(input, lists, fn, usrdata);
			break;
		case del_chr:		
		case backspace_chr:
			remove_char(input);
			break;
		case 1:
			move_to_begin(input);
			break;
		case 5:	
			move_to_end(input);
			break;
		case 21:
			clear_input(input);
			break;
		case 23:
			remove_word(input);
			break;
		default:
			update_input(ch, input);
	}
}

char *readline(struct input *input, const struct readline_list **lists,
	readline_before_action_t fn, void *usrdata)
{
	char exit = 0;
	int rcount, i, offset;
	char *buf = malloc(sizeof(*buf)*(sizeof(input->value)));
	fn(usrdata);
	while((rcount = read(0, buf,
			sizeof(input->value)-strlen(input->value))) > 0) {
		buf[rcount] = 0;
		for(i = 0; i < rcount; i++) {
			if(buf[i] != 27)
				process_char(buf[i], input, lists, &exit, fn, usrdata);
			else {
				process_escape_sequence(buf+i, input, &offset);
				i += offset;
			}
			if(exit)
				break;
		}
		fflush(stdout);
		if(exit)
			break;
	}
	free(buf);
	return input->value;
}

static struct termios mem_tconf;
int readline_start()
{
	struct termios tconf;
	if(!isatty(0)) {
		fprintf(stderr, "It works only in the terminal\n");
		return 1;
	}
	tcgetattr(0, &tconf);
	memcpy(&mem_tconf, &tconf, sizeof(mem_tconf));
	tconf.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(0, TCSANOW, &tconf);
	return 0;
}

int readline_end()
{
	tcsetattr(0, TCSANOW, &mem_tconf);
	return 0;
}
