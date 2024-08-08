#include "shell.h"
#include "strlib.h"
#include "memlib.h"
#include "fslib.h"
#include "path.h"
#include "task.h"
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
#define CMD_LS "ls"

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
    cmd_ls,
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
    err_failed_ls,
} status;

#define HELP_INFO "task -- manage todo lists.\n" \
"Commands:\n" \
"init -- initialize the project.\n" \
"exit -- exit from the program.\n" \
"help -- display this information.\n" \
"mk [tname] -- making an object of task.\n" \
"mk [tname -f -- making an object of filter.\n" \
"rm [tname] -- removing an object.\n" \
"go [path] -- moving between objects.\n" \
"ls -- display content of current object.\n\n" \
"To edit tasks/filters you can to use any text editor that open the project.\n" \
"For example, you have a task \"task1\" within the project then you can to open the file \"task/main.tsk\" in any text editor. This file contains all information about this object of task.\n"

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
    if(fd == -1)
        return err_failed_init;
    return 0;
}

static status ls_action()
{
    char cdir[bsize];
    char ok;
    getcwd(cdir, sizeof(cdir));
    ok = print_task(cdir);
    if(ok == -1)
        return err_failed_ls;
    return 0;
}

static status go_action(const char *params[], path_aux *aux)
{
    char ok;
    if(!params)
        return err_invalid_params;
    ok = change_dir(params[0], aux);
    if(ok != 0)
        return err_failed_go;
    return ls_action();
}

static status mk_action(const char *params[])
{
    char is_filter = 0;
    char ok;
    int fd;
    if(!params)
        return err_invalid_params;
    ok = create_block(params[0], TASK_CORE_FILE, &fd);
    if(ok != 0)
        return err_failed_mk;
    if(params[1])
        is_filter = strcmp(params[1], FILTER_TASK_FLAG) == 0;
    ok = task_write(fd, is_filter);
    close(fd);
    return ok;
}

static status rm_action(const char *params[])
{
    char ok;
    if(!params) 
        return err_invalid_params;
    ok = remove_dir(params[0]);
    if(ok != 0) 
        return err_failed_rm;
    return 0;
}

static status cmd_exec(cmd_type ctype, const char *params[],
    path_aux *aux)
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
            return go_action(params, aux);
        case cmd_ls:
            return ls_action();
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
    if(strcmp(cmd, CMD_LS) == 0)
        return cmd_ls;
    return cmd_err;
}

static void error_log(status st)
{
    switch(st) {
        case err_invalid_cmd:
            fprintf(stderr, "Unknown command\n");
            break;
        case err_invalid_params:
            fprintf(stderr, "Invalid list of params\n");
            break;
        case err_failed_mk:
            fprintf(stderr, "Failed to create the new object\n");
            break;
        case err_failed_go:
            fprintf(stderr, "Failed to go\n");
            break;
        case err_failed_rm:
            fprintf(stderr, "Failed to remove\n");
            break;
        case err_failed_ls:
            fprintf(stderr, "Failed to print content\n");
            break;
        case err_failed_init:
            fprintf(stderr, "Failed to init the project\n");
            break;
        case st_exit:
        case st_cont:
        case success:
            break;
    }
}

static status process_cmd(const char *cmd, path_aux *aux)
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
    st = cmd_exec(ctype, (const char **)(params+1), aux);
    mem_free((void **)params);
    return st;
}

char shell_run(int argc, const char *argv[])
{
    char buf[bsize];
    char *cmd;
    path_aux *aux = path_init();
    if(!aux)
        return -1;
    print_path(aux);
    while((cmd = fgets_m(buf, bsize, stdin)) != NULL) {
        status st;
        st = process_cmd(cmd, aux);
        if(st == st_exit)
            break;
        print_path(aux);
        if(st == st_cont)
            continue;
        if(st != 0)
            error_log(st);
    }
    return 0;
}
