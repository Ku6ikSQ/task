#include "task.h"
#include "strlib.h"
#include "memlib.h"
#include "fslib.h"
#include "path.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define TNAME_FLD "name"
#define TINFO_FLD "info"
#define TCOMPLETED_FLD "completed"
#define TFROM_FLD "from"
#define TTO_FLD "to"
#define TTYPE_FLD "type"

#define TASK_FILTER_TEMPLATE TNAME_FLD "\n" TINFO_FLD "\n"
#define TASK_TEMPLATE TNAME_FLD "\n" TINFO_FLD "\n" TCOMPLETED_FLD \
	" false\n" TFROM_FLD "\n" TTO_FLD "\n"

typedef enum {
    task_default,
    task_filter,
} task_t;

struct deadlines {
    char *from;
    char *to;
};

struct task {
    task_t type;
    char *name;
    char *info;
	char edited;
    char completed;
    struct deadlines *dlines;
};

static void task_init(struct task *task)
{
    if(!task)
        return;
    task->name = NULL;
    task->info = NULL;
    task->dlines = NULL;
    task->completed = 0;
	task->edited = 0;
    task->type = task_filter;
}

static char **get_record(const char *s)
{
	char **rec;
	long long offset;
	if(!s)
		return NULL;
	rec = malloc(sizeof(*rec)*3);
	if(*s != ' ') {
		rec[0] = get_word(s, &offset);
		s += offset;
	} else
		rec[0] = NULL; /* there's no any record name */
	while(*s == ' ')
		s++;
	rec[1] = string_duplicate(s);
	rec[2] = NULL;
	return rec;
}

static char *field_extend(const char *str, const char *ext)
{
	char *newstr;
	long long slen, elen, nlen;
	if(!str)
		return NULL;
	slen = strlen(str);
	elen = strlen(ext);
	newstr = malloc(sizeof(*newstr)*(slen+elen+2)); /* +1: \n, +1: \0 */
	strcpy(newstr, str);
	nlen = strlen(newstr);
	newstr[nlen] = '\n';
	newstr[nlen+1] = 0;
	strcat(newstr, ext);
	return newstr;
}

char set_newlines(char *str)
{
	char *match;
	if(!str)
		return -1;
	while((match = strstr(str, "\\n")) != NULL) {
		//string_shl(match, 1);
		match[0] = '\n';
		match[1] = ' ';
	}
	return 0;
}

static void task_set_text_field(char **pfield, const char *value, 
	char rewrite)
{
	char *newval;
	if(!pfield || !*pfield || rewrite)
		newval = string_duplicate(value);
	else {
		newval = field_extend(*pfield, value);
		free(*pfield);
	}
	*pfield = newval;
	set_newlines(*pfield);
}

static void task_set_name(struct task *task, const char *value,
	char rewrite)
{
	task_set_text_field(&task->name, value, rewrite);
}

static void task_set_info(struct task *task, const char *value, 
	char rewrite)
{
	task_set_text_field(&task->info, value, rewrite);
}

static void task_set_completed(struct task *task, const char *value)
{
	if((strcmp(value, "true") == 0) || (strcmp(value, "1") == 0))
		task->completed = 1;
	else if((strcmp(value, "false") == 0) || (strcmp(value, "0") == 0))
		task->completed = 0;
	else 
		task->completed = -1;
	task->type = task->completed >= 0 ? task_default : task_filter;
}

static void task_set_deadlines(struct task *task, const char *name, 
	const char *value)
{
	if(!task->dlines) {
		task->dlines = malloc(sizeof(*(task->dlines)));
		task->dlines->from = NULL;
		task->dlines->to = NULL;
	}
	if(!value || !*value)
		return;
	if(strcmp(name, "from") == 0)
		task->dlines->from = string_duplicate(value);
	else if(strcmp(name, "to") == 0)
		task->dlines->to = string_duplicate(value);
}

static void task_set_type(struct task *task, const char *type)
{
	if(!task || !type)
		return;
	if((strcmp(type, "filter") == 0) || strcmp(type, "f") == 0)
		task->type = task_filter;
	else
		task->type = task_default;
}

char task_set_field(struct task *task, const char *name, const char *value,
	char rewrite)
{
	if(!task || !name)
		return -1;
	task->edited = 1;
    if(strcmp(name, TNAME_FLD) == 0)
		task_set_name(task, value, rewrite);
    else if(strcmp(name, TINFO_FLD) == 0)
		task_set_info(task, value, rewrite);
    else if(strcmp(name, TCOMPLETED_FLD) == 0)
		task_set_completed(task, value);
    else if((strcmp(name, TFROM_FLD) == 0) || (strcmp(name, TTO_FLD) == 0))
		task_set_deadlines(task, name, value);
	else if(strcmp(name, TTYPE_FLD) == 0) 
		task_set_type(task, value);
	return 0;
}

void task_free(struct task *task)
{
    if(!task)
        return;
    if(task->name)
        free(task->name); 
    if(task->info)
        free(task->info);
    if(task->dlines) {
        if(task->dlines->from)
            free(task->dlines->from);
        if(task->dlines->to)
            free(task->dlines->to);
        free(task->dlines);
    }
    free(task);
}

struct task *task_read(const char *path)
{
    char *corename, *recname, *s;
    struct task *task;
    char buf[4096];
    FILE *f;
    if(!path)
        return NULL;
    corename = paths_union(path, TASK_CORE_FILE);
    f = fopen(corename, "r");
    free(corename);
    if(!f)
        return NULL;
    task = malloc(sizeof(*task));
    task_init(task);
	recname = NULL;
    while((s = fgets_m(buf, sizeof(buf), f)) != NULL) {
		char **rec = get_record(s);
		if(!rec[1]) {
			mem_free((void **)rec);
			free(rec);
			continue;
		}
		if(rec[0]) {
			if(recname)
				free(recname);
			recname = string_duplicate(rec[0]);
		}
		task_set_field(task, recname, rec[1], 0);
    }
	task->edited = 0;
	fclose(f);
    return task;
}

static char *get_record_str(const char *name, const char *value)
{
	if(!name)
		return NULL;
	if(!value)
		return strings_concatenate(name, "\n", NULL);
	return strings_concatenate(name, " ", value, "\n", NULL);
}

static char write_text_field_record(FILE *f, const char *name, 
	const char *value)
{
	char ok;
	char *rec = get_record_str(name, value);
	ok = fputs(rec, f) != EOF;
	free(rec);
	return ok;
}

static char write_name_record(FILE *f, const char *name)
{
	return write_text_field_record(f, TNAME_FLD, name);
}

static char write_info_record(FILE *f, const char *info)
{
	return write_text_field_record(f, TINFO_FLD, info);
}

static char write_completed_record(FILE *f, char completed)
{
	char ok;
	char *rec = get_record_str(TCOMPLETED_FLD, 
		completed ? "true" : "false");
	ok = fputs(rec, f) != EOF;
	free(rec);
	return ok;
}

static char write_deadlines_record(FILE *f, const struct deadlines *dlines)
{
	char ok;
	char *rec;
	if(!dlines)
		return -1;
	rec = strings_concatenate(TFROM_FLD, " ", dlines->from, "\n", 
		TTO_FLD, " ", dlines->to, "\n", NULL);
	ok = fputs(rec, f) != EOF;
	free(rec);
	return ok;
}

char task_write(const char *path, const struct task *task)
{
	FILE *f;
	char *corename;
	if(!task || !task->edited)
		return 0;
    corename = paths_union(path, TASK_CORE_FILE);
	f = fopen(corename, "w");
	free(corename);
	if(!f)
		return -1;
	write_name_record(f, task->name);
	write_info_record(f, task->info);
	if(task->type == task_default) {
		write_completed_record(f, task->completed);
		write_deadlines_record(f, task->dlines);
	}
	fclose(f);
	return 0;
}

char task_make_file(int fd, char is_filter)
{
    const char *template;
    char ok;
    if(fd < 0)
        return -1;
    template = is_filter ? TASK_FILTER_TEMPLATE : TASK_TEMPLATE;
    ok = write(fd, template, strlen(template));
    if(ok == -1)
        return -1; 
    return 0;
}

static char get_task_status(const struct task *task)
{
    return task->completed ? 'v' : 'x';
}

static void align_str(char *s)
{
	while(*s) {
		if(*s == '\n') {
			s++;
			while(*s == ' ')
				string_shl(s, 1);
			continue;
		}
		s++;
	}
}

static void print_header(const struct task *task)
{
	char status;
	char *info, *name;
	if(!task->name || !(task->name[0]))
		return;
	name = strdup(task->name);
	info = task->info ? strdup(task->info) : "";
	align_str(name);
	align_str(info);
	status = get_task_status(task);
    if(task->type == task_filter) {
        printf("====%s====\n%s\n\n", task->name, info);
    } else {
        printf("====[%c] %s====\n%s\n\n", status, task->name, info);
    }
}

#define DLINES_SEP " -- "
#define DLINES_TITLE "====DEADLINES====\n"
static void print_deadlines(struct deadlines *dls)
{
    char *str;
	if(!dls || (!dls->from && !dls->to))
        return;
	if(dls->from && dls->to)
		str = strings_concatenate(DLINES_TITLE, dls->from, DLINES_SEP,
			dls->to, NULL);
	else
		str = strings_concatenate(DLINES_TITLE,
			dls->from ? dls->from : dls->to, NULL);
    printf("%s\n\n", str);
    free(str);
}

static void print_addinfo(const struct task *task)
{
    if(task->type == task_filter)
        return;
    print_deadlines(task->dlines);
}

static void print_subtask(struct task *task, const char *shortname)
{
	char status;
    if(!task)
        return;
    status = get_task_status(task);
    if(task->type == task_default)
        printf("[%c] %s (%s)\n", status, task->name, shortname);
    else
        printf("%s (%s)\n", task->name, shortname);
}

static void print_subtasks(const char *path)
{
    DIR *dir;
    struct dirent *dent;
	char taskpath[4096];
    if(!path)
        return;
    dir = opendir(path);
    if(!dir)
        return;
    while((dent = readdir(dir)) != NULL) {
        struct task *task;
        if((strcmp(dent->d_name, ".") == 0) || 
           (strcmp(dent->d_name, "..") == 0) || 
           (strcmp(dent->d_name, TASK_CORE_FILE) == 0))
            continue;
		strcpy(taskpath, path);
		path_extend(taskpath, dent->d_name);
        task = task_read(taskpath);
        print_subtask(task, dent->d_name);
        task_free(task);
    }
	closedir(dir);
    putchar('\n');
}

char task_print(const struct task *task, const char *taskpath)
{
    if(task && task->name) {
        print_header(task);
        print_addinfo(task);
    }
    print_subtasks(taskpath);
    return 0;
}
