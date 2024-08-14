#include "shell.h"
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
    cmd_empty, 
    cmd_err,
} cmd_type;

typedef enum {
    success = 0,
    st_exit,
    st_cont,
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
} status;

struct state {
	char root[bsize];
};

static char state_init(struct state *state)
{
	char *tmp;
	if(!state)
		return -1;
	tmp = getcwd(state->root, sizeof(state->root));
	if(!tmp)
		return -1;
	return 0;
}

static void print_cwd(const char *root)
{
	char cwd[bsize];
	char *shortpath;
	getcwd(cwd, sizeof(cwd));
	shortpath = get_shortpath(root, cwd);
	if(!shortpath)
		return;
	printf("~%s$ ", shortpath);
}

static char *process_path(const char *path, const struct state *state)
{
	char *newpath;
	if(path[0] != '~')
		return strdup(path);
	path++;
	newpath = paths_union(state->root, path);
	return newpath;
}

#define HELP_INFO "task -- manage todo lists.\n" \
"Commands:\n" \
"in	it -- initialize the project.\n" \
"exit -- exit from the program.\n" \
"help -- display this information.\n" \
"mk [tname] -- making an object of task.\n" \
"mk [tname -f -- making an object of filter.\n" \
"rm [tname] -- removing an object.\n" \
"go [path] -- moving between objects.\n" \
"show -- display content of current object.\n" \
"ln [target] [linkpath] -- link an object to another object.\n" \
"mv [oldpath] [newpath] -- move or rename task.\n" \
"\nTo edit tasks/filters you can to use any text editor that open the project.\n"

static status help_action()
{
    puts(HELP_INFO);
    return 0;
}

static status exit_action()
{
    return st_exit;
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

static status show_action()
{
    char cdir[bsize];
    char ok;
    getcwd(cdir, sizeof(cdir));
    ok = print_task(cdir);
    if(ok == -1) {
		perror(CMD_SHOW);
        return err_failed_show;
	}
    return 0;
}

static status go_action(const char *params[], struct state *state)
{
	char *path;
	char ok;
	if(!params || !params[0])
		return err_invalid_params;
	path = process_path(params[0], state);
	ok = chdir(path);
	free(path);
	if(ok == -1) {
		perror(CMD_GO);
		return err_failed_go;
	}
    return show_action();
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
    if(params[1])
        is_filter = strcmp(params[1], FILTER_TASK_FLAG) == 0;
    ok = task_write(fd, is_filter);
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
	char *abspath;
	char cwd[4096];
	if(!params || !params[0] || !params[1])
		return err_invalid_params;
	getcwd(cwd, sizeof(cwd));
	abspath = paths_union(cwd, params[0]);
	ok = symlink(abspath, params[1]);
	free(abspath);
	if(ok == -1) {
		perror(CMD_LN);
		return err_failed_ln;
	}
	return 0;
}

static status mv_action(const char *params[])
{
	char ok;
	if(!params || !params[0] || !params[1])
		return err_invalid_params;
	ok = rename(params[0], params[1]);
	if(ok == -1) {
		perror(CMD_MV);
		return err_failed_mv;
	}
	return 0;
}

static status cmd_exec(cmd_type ctype, const char *params[], 
	struct state *state)
{
    switch(ctype) {
        case cmd_help:
            return help_action();
        case cmd_exit:
            return exit_action();
        case cmd_init:
            return init_action();
        case cmd_mk:
            return mk_action(params);
        case cmd_rm:
            return rm_action(params);
        case cmd_go:
            return go_action(params, state);
        case cmd_show:
            return show_action();
		case cmd_ln:
			return ln_action(params, state);
		case cmd_mv:
			return mv_action(params);
        case cmd_empty:
            return st_cont;
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
        case err_failed_init:
            fprintf(stdout, "Failed to init the project\n");
            break;
		case err_failed_run:
			fprintf(stdout, "Failed to run the program\n");
			break;
        case st_exit:
        case st_cont:
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

char shell_run(int argc, const char *argv[])
{
	struct state state;
    char buf[bsize];
    char *cmd;
	char ok;
	ok = state_init(&state);
    print_cwd(state.root);
	if(ok == -1)
		return err_failed_run;
    while((cmd = fgets_m(buf, bsize, stdin)) != NULL) {
        status st;
        st = process_cmd(cmd, &state);
        if(st == st_exit)
            break;
        if(st == st_cont) {
        	print_cwd(state.root);
            continue;
		}
        if(st != 0)
            error_log(st);
       	print_cwd(state.root);
    }
    return 0;
}
