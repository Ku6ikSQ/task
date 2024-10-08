#include "shell.h"
#include "readline.h"
#include "params.h"
#include "strlib.h"
#include "memlib.h"
#include "fslib.h"
#include "task.h"
#include "path.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#define CMD_HELP "help"
#define CMD_EXIT "exit"
#define CMD_INIT "init"
#define CMD_MK "mk"
#define CMD_RM "rm"
#define CMD_GO "go"
#define CMD_SHOW "show"
#define CMD_LN "ln"
#define CMD_MV "mv"
#define CMD_SET "set"
#define CMD_CLEAR "clear"

#define FILTER_TASK_FLAG "-f"

enum {
    bsize = 4096,
};

typedef enum {
    cmd_help,
    cmd_exit,
    cmd_init,
    cmd_mk,
    cmd_rm,
    cmd_go,
    cmd_show,
	cmd_ln,
	cmd_mv,
	cmd_set,
	cmd_clear,
    cmd_empty, 
    cmd_err,
} cmd_type;

typedef enum {
    success = 0,
    err_failed_init,
    err_invalid_cmd,
    err_invalid_params,
    err_failed_mk,
    err_failed_go,
    err_failed_rm,
    err_failed_show,
	err_failed_run,
	err_failed_ln,
	err_failed_mv,
	err_failed_clear,
	err_failed_set,
} status;

struct state {
	char root[bsize];
	char cwd[bsize];
	cmd_type last_cmd;
	struct task *cur_task;
};

static int change_dir(const char *path, struct state *state)
{
	int ok = chdir(path);
	if(ok == -1)
		return -1;
	getcwd(state->cwd, sizeof(state->cwd));
	return 0;
}

static char state_init(struct state *state)
{
	char *tmp;
	if(!state)
		return -1;
	tmp = getcwd(state->root, sizeof(state->root));
	if(!tmp)
		return -1;
	strcpy(state->cwd, state->root);
	state->cur_task = NULL;
	return 0;
}

static void print_shortcwd(struct state *state)
{
	char *shortpath;
	shortpath = get_shortpath(state->root, state->cwd);
	if(!shortpath)
		return;
	printf("~%s$ ", shortpath);
	fflush(stdout);
}

static char *process_path(const char *path, const struct state *state)
{
	char *newpath;
	if(!path)
		return NULL;
	if(path[0] != '~')
		return strdup(path);
	if(!state)
		return NULL;
	path++; /* skip '~' */
	if(path[0] == '/')
		path++; /* skip '/' */
	newpath = paths_union(state->root, path);
	return newpath;
}

static char *get_full_destpath(const char *path, const char *ext)
{
	char *result, *shortname;
	long long len;
	len = strlen(path);
	if(path[len-1] != '/')
		return strdup(path);	
	shortname = get_shortname(ext);
	result = strings_concatenate(path, shortname, NULL);	
	free(shortname);
	return result;
}

#define HELP_INFO "task -- manage todo lists.\n" \
"Commands:\n" \
"help -- display this information.\n" \
"init -- initialize the project.\n" \
"exit -- exit from the program.\n" \
"mk [taskname] -- making an object of task.\n" \
"mk [taskname] -f -- making an object of filter.\n" \
"rm [taskname] -- removing an object.\n" \
"go [path] -- moving between objects.\n" \
"show -- display content of current object.\n" \
"show [path] -- display content of an object.\n" \
"ln [target] [linkpath] -- link an object to another object.\n" \
"mv [oldpath] [newpath] -- move or rename task.\n" \
"set [field] [value] -- set a value of task's field.\n" \
"clear -- clear the terminal screen.\n"

static status help_action()
{
    puts(HELP_INFO);
    return 0;
}

static status exit_action(struct state *state)
{
	char ok = task_write(state->cwd, state->cur_task);
	if(ok != 0)
		perror("task_write");
    return 0;
}

static status init_action()
{
    int fd = open(TASK_CORE_FILE, O_WRONLY|O_CREAT, 0666);
    if(fd == -1) {
		perror(CMD_INIT);
        return err_failed_init;
	}
    return 0;
}

static status show_action(const char *params[], struct state *state)
{
	struct task *task = state->cur_task;
    char cdir[bsize];
    char ok;
	if(params && params[0]) {
		char *path;
		path = process_path(params[0], state);
		strcpy(cdir, path);
		task = task_read(cdir);
		free(path);
	}
	else
		memcpy(cdir, state->cwd, sizeof(cdir));
    ok = task_print(task, cdir);
	if(state->cur_task != task)
		task_free(task);
    if(ok == -1) {
		perror(CMD_SHOW);
        return err_failed_show;
	}
    return 0;
}

static status go_action(const char *params[], struct state *state)
{
	struct task *new_task;
	char *path;
	char ok;
	if(!params || !params[0])
		return err_invalid_params;
	path = process_path(params[0], state);
	ok = task_write(state->cwd, state->cur_task);
	if(ok != 0)
		perror("task_write");
	new_task = task_read(path);
	ok = change_dir(path, state);
	free(path);
	if(ok == -1) {
		perror(CMD_GO);
		task_free(new_task);
		return err_failed_go;
	}
	task_free(state->cur_task);
	state->cur_task = new_task;
    return show_action(NULL, state);
}

static status mk_action(const char *params[])
{
    char is_filter = 0;
    char ok;
    int fd;
    if(!params || !params[0])
        return err_invalid_params;
    ok = create_block(params[0], TASK_CORE_FILE, &fd);
    if(ok != 0) {
		perror(CMD_MK);
        return err_failed_mk;
	}
    if(param_search(params, FILTER_TASK_FLAG, NULL) != -1)
		is_filter = 1;
    ok = task_make_file(fd, is_filter);
    close(fd);
    return ok;
}

static status rm_action(const char *params[])
{
    char ok;
	if(!params || !params[0])
        return err_invalid_params;
	ok = unlink(params[0]);
	if(ok == 0) /* it means that it was a symbolic link on directory */
		return 0;
    ok = remove_dir(params[0]);
    if(ok != 0) {
		perror(CMD_RM);
        return err_failed_rm;
	}
    return 0;
}

static status ln_action(const char *params[], const struct state *state)
{
	char ok;
	char *target, *linkpath, *full_linkpath;
	if(!params || !params[0] || !params[1])
		return err_invalid_params;
	target = process_path(params[0], state);
	linkpath = process_path(params[1], state);
	full_linkpath = get_full_destpath(linkpath, target);
	if(!is_abspath(target)) {
		char *tmp = target;
		target = paths_union(state->cwd, target);
		free(tmp);
	}
	ok = symlink(target, full_linkpath);
	free(full_linkpath);
	free(target);
	free(linkpath);
	if(ok == -1) {
		perror(CMD_LN);
		return err_failed_ln;
	}
	return 0;
}

static status mv_action(const char *params[], const struct state *state)
{
	char ok;
	char *oldpath, *newpath, *completed_newpath;
	if(!params || !params[0] || !params[1])
		return err_invalid_params;
	oldpath = process_path(params[0], state);
	newpath = process_path(params[1], state);
	completed_newpath = get_full_destpath(newpath, oldpath);
	ok = rename(oldpath, completed_newpath);
	free(completed_newpath);
	free(oldpath);
	free(newpath);
	if(ok == -1) {
		perror(CMD_MV);
		return err_failed_mv;
	}
	return 0;
}

static status set_action(const char *params[], const struct state *state)
{
	char ok;
	if(!params || !params[0])
		return err_invalid_params;
	ok = task_set_field(state->cur_task, params[0], params[1], 1);
	if(ok == -1)
		return err_failed_set;
	return 0;
}

static status clear_action()
{
	int pid = fork();
	if(pid == -1)
		return err_failed_clear;
	if(pid == 0) {
		execlp(CMD_CLEAR, CMD_CLEAR, NULL);
		perror(CMD_CLEAR);
		return err_failed_clear;
	}
	wait(NULL);
	return 0;
}

static status cmd_exec(cmd_type ctype, const char *params[], 
	struct state *state)
{
	state->last_cmd = ctype;
    switch(ctype) {
        case cmd_help:
            return help_action();
        case cmd_exit:
            return exit_action(state);
        case cmd_init:
            return init_action();
        case cmd_mk:
            return mk_action(params);
        case cmd_rm:
            return rm_action(params);
        case cmd_go:
            return go_action(params, state);
        case cmd_show:
            return show_action(params, state);
		case cmd_ln:
			return ln_action(params, state);
		case cmd_mv:
			return mv_action(params, state);
		case cmd_set:
			return set_action(params, state);
		case cmd_clear:
			return clear_action();
        case cmd_empty:
            return 0;
        case cmd_err:
            return err_invalid_cmd;
    }
    return 0;
}

static cmd_type get_ctype(const char *cmd)
{
	if(!cmd)
        return cmd_empty;
	if(strcmp(cmd, CMD_HELP) == 0)
        return cmd_help;
	if(strcmp(cmd, CMD_EXIT) == 0)
        return cmd_exit;
	if(strcmp(cmd, CMD_INIT) == 0)
        return cmd_init;
	if(strcmp(cmd, CMD_MK) == 0)
        return cmd_mk;
	if(strcmp(cmd, CMD_RM) == 0)
        return cmd_rm;
	if(strcmp(cmd, CMD_GO) == 0)
        return cmd_go;
	if(strcmp(cmd, CMD_SHOW) == 0)
        return cmd_show;
	if(strcmp(cmd, CMD_LN) == 0)
		return cmd_ln;
	if(strcmp(cmd, CMD_MV) == 0)
		return cmd_mv;
	if(strcmp(cmd, CMD_SET) == 0)	
		return cmd_set;
	if(strcmp(cmd, CMD_CLEAR) == 0)
		return cmd_clear;
    return cmd_err;
}

static void error_log(status st)
{
    switch(st) {
        case err_invalid_cmd:
            fprintf(stdout, "Unknown command\n");
            break;
        case err_invalid_params:
            fprintf(stdout, "Invalid list of params\n");
            break;
        case err_failed_mk:
            fprintf(stdout, "Failed to create the new object\n");
            break;
        case err_failed_go:
            fprintf(stdout, "Failed to go\n");
            break;
        case err_failed_rm:
            fprintf(stdout, "Failed to remove\n");
            break;
        case err_failed_show:
            fprintf(stdout, "Failed to print content\n");
            break;
		case err_failed_ln:
			fprintf(stdout, "Failed to create the link\n");
			break;
		case err_failed_mv:
			fprintf(stdout, "Failed to move/rename the file\n");
			break;
		case err_failed_set:
			fprintf(stdout, "Failed to set the new value\n");
			break;
		case err_failed_clear:
			fprintf(stdout, "Failed to clear the screen\n");
			break;
        case err_failed_init:
            fprintf(stdout, "Failed to init the project\n");
            break;
		case err_failed_run:
			fprintf(stdout, "Failed to run the program\n");
			break;
        case success:
            break;
    }
}

static void process_params(char **params)
{
	while(*params) {
		set_special_chars(*params);
		params++;
	}
}

static status process_cmd(const char *cmd, struct state *state)
{
    status st;
    cmd_type ctype;
    char **params = NULL;
    if(!cmd)
        return err_invalid_cmd;
    params = get_tokens(cmd);
    ctype = get_ctype(params[0]);
	if(ctype == cmd_err)
        return err_invalid_cmd;
	process_params(params);
    st = cmd_exec(ctype, (const char **)(params+1), state);
    mem_free((void **)params);
    return st;
}

static void fill_by_path(struct list *lst, const char *path)
{
	DIR *dir;
	struct dirent *dent;
	struct list *tmp;
	long long i;
	if(path[0] != '.')
		return;
	if(lst->count != 0) {
		for(i = 0; i < lst->count; i++)
			free(lst->words[i]);
		free(lst->words);
		tmp = list_create(NULL);
		memcpy(lst, tmp, sizeof(*tmp));
	}
	if(path[1] == '/') 
		dir = opendir(".");
	else if(path[1] == '.')
		dir = opendir("..");
	for(i = 0; (dent = readdir(dir)) != NULL; i++)
		list_append(lst, dent->d_name);
	lst->words[i] = NULL;
	closedir(dir);
}

static void get_lists(struct readline_list *(*lists)[3])
{
	(*lists)[0] = malloc(sizeof((*lists)[0]));
	(*lists)[0]->value = list_create(CMD_HELP, CMD_EXIT, CMD_INIT, CMD_MK,
			CMD_RM, CMD_GO, CMD_SHOW, CMD_LN, CMD_MV, CMD_SET, CMD_CLEAR,
			TNAME_FLD, TINFO_FLD, TFROM_FLD, TTO_FLD, TTYPE_FLD, 
			TCOMPLETED_FLD, NULL);
	(*lists)[0]->before_action = NULL;
	(*lists)[0]->check_rule = NULL;
	(*lists)[1] = malloc(sizeof((*lists)[1]));
	(*lists)[1]->value = list_create(NULL);
	(*lists)[1]->before_action = fill_by_path;
	(*lists)[1]->check_rule = is_taskname;
	(*lists)[2] = NULL;
}

static void free_lists(struct readline_list *lists[3])
{
	list_free(lists[0]->value);
	free(lists[0]);
	list_free(lists[1]->value);
	free(lists[1]);
}

char shell_run()
{
	struct state state;
	struct input input;
	struct readline_list *(lists[3]);
	char ok;
	ok = state_init(&state);
	if(ok == -1)
		return err_failed_run;
	get_lists(&lists);
	input_init(&input);
	while(readline(&input, (const struct readline_list **)&lists,
			(readline_before_action_t)print_shortcwd,
			(void *)(&state)) != NULL) {
		status st;
		input.value[strlen(input.value)-1] = 0; /* to remove the newline */
        st = process_cmd(input.value, &state);
        if(state.last_cmd == cmd_exit)
            break;
        if(state.last_cmd == cmd_empty)
            continue;
        if(st != 0)
			error_log(st);
		input_init(&input);
	}
	free_lists(lists);
    return 0;
}
