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
    task->type = task_filter;
}

static void task_free(struct task *task)
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

char task_write(int fd, char is_filter)
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
	rec[1] = strdup(s);
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

static void task_set_name(struct task *task, const char *value)
{
	char *name;
	if(!task->name) {
		task->name = strdup(value);
		return;
	}
#if 1
	name = field_extend(task->name, value);
	free(task->name);
	task->name = name;
#endif
}

static void task_set_info(struct task *task, const char *value)
{
	char *info;
	if(!task->info) {
		task->info = strdup(value);
		return;
	}
#if 1
	info = field_extend(task->info, value);
	free(task->info);
	task->info = info;
#endif
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
		task->dlines->from = strdup(value);
	else if(strcmp(name, "to") == 0)
		task->dlines->to = strdup(value);
}

static void task_set_field(struct task *task, const char *name, 
	const char *value)
{
	if(!task || !name || !value)
		return;
    if(strcmp(name, TNAME_FLD) == 0)
		task_set_name(task, value);
    else if(strcmp(name, TINFO_FLD) == 0)
		task_set_info(task, value);
    else if(strcmp(name, TCOMPLETED_FLD) == 0)
		task_set_completed(task, value);
    else if((strcmp(name, TFROM_FLD) == 0) || (strcmp(name, TTO_FLD) == 0))
		task_set_deadlines(task, name, value);
}

static struct task *task_read(const char *path)
{
    char *corename, *s, *recname;
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
			recname = strdup(rec[0]);
		}
		task_set_field(task, recname, rec[1]);
    }
	fclose(f);
    return task;
}

static const char *task_status_str(const struct task *task)
{
    return task->completed ? "v" : "x";
}

static void print_header(const struct task *task)
{
    const char *task_stat = task_status_str(task);
    char *task_info = task->info ? task->info : "";
    if(task->type == task_filter) {
        printf("====%s====\n%s\n\n", task->name, task_info);
    } else {
        printf("====[%s] %s====\n%s\n\n", task_stat, task->name,
            task_info);
    }
}

static void print_deadlines(struct deadlines *dls)
{
    char *str;
    long long len_from, len_to, len_sep, len_tit;
    static const char * const separator = " -- ";
    static const char * const title = "====DEADLINES====\n";
    if(!dls || !dls->from)
        return;
    len_sep = sizeof(separator)-1;
    len_tit = sizeof(title)-1;
    len_from = string_length(dls->from);
    len_to = string_length(dls->to);
    str = malloc(sizeof(*str)*(len_tit+len_from+len_sep+len_to+1));
    strcpy(str, title);
    string_catenate(str, dls->from); 
    if(dls->to) {
        string_catenate(str, separator);
        string_catenate(str, dls->to);
    }
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
    const char *task_stat;
    if(!task)
        return;
    task_stat = task_status_str(task);
    if(task->type == task_default)
        printf("[%s] %s (%s)\n", task_stat, task->name, shortname);
    else
        printf("%s (%s)\n", task->name, shortname);
}

static void print_content(const char *path)
{
    DIR *dir;
    struct dirent *dent;
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
        task = task_read(dent->d_name);
        print_subtask(task, dent->d_name);
        task_free(task);
    }
	closedir(dir);
    putchar('\n');
}

char print_task(const char *path)
{
    struct task *task = task_read(path);
    if(!task)
        return -1;
    if(task->name) {
        print_header(task);
        print_addinfo(task);
    }
    print_content(path);
	task_free(task);
    return 0;
}
