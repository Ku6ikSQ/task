#include "shell.h"
#include "params.h"
#include "strlib.h"
#include "memlib.h"
#include "fslib.h"
#include "task.h"
#include "path.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

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
"set [field] [value] -- set a value of task's field.\n"

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
    if(param_search(params, FILTER_TASK_FLAG) != -1)
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
	char *target, *linkpath;
	if(!params || !params[0] || !params[1])
		return err_invalid_params;
	target = process_path(params[0], state);
	linkpath = process_path(params[1], state);
	if(!is_abspath(target)) {
		char *tmp = target;
		/* TODO: check for what I did union of paths */
		target = paths_union(state->cwd, target);
		free(tmp);
	}
	ok = symlink(target, linkpath);
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
	char *oldpath, *newpath;
	if(!params || !params[0] || !params[1])
		return err_invalid_params;
	oldpath = process_path(params[0], state);
	newpath = process_path(params[1], state);
	ok = rename(oldpath, newpath);
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
    st = cmd_exec(ctype, (const char **)(params+1), state);
    mem_free((void **)params);
    return st;
}

char shell_run()
{
	struct state state;
    char buf[bsize];
    char *cmd;
	char ok;
	ok = state_init(&state);
    print_shortcwd(&state);
	if(ok == -1)
		return err_failed_run;
    while((cmd = fgets_m(buf, bsize, stdin)) != NULL) {
        status st = process_cmd(cmd, &state);
        if(state.last_cmd  == cmd_exit)
            break;
        if(state.last_cmd == cmd_empty) {
        	print_shortcwd(&state);
            continue;
		}
        if(st != 0)
            error_log(st);
       	print_shortcwd(&state);
    }
    return 0;
}
