#include "path.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define PATH_DELIM_STR "/"
#define CRNT_LEVEL "."
#define TOP_LEVEL ".."

enum {
    path_max_len = 4096,
};

enum {
    path_delim = '/',
};

struct path_aux {
    char root_path[path_max_len];
    char crnt_path[path_max_len];
    char *shrt_path;
};

static char *get_short_path(const char *fullpath, path_aux *aux);
static void aux_set_crnt_path(const char *newpath, path_aux *aux)
{
    strcpy(aux->crnt_path, newpath);
    aux->shrt_path = get_short_path(aux->crnt_path, aux);
}

static char add_path_ending(char *path, long long mxsize)
{
    long long len;
    if(!path)
        return -1;
    len = strlen(path);
    if((len+1) > mxsize)
        return -1;
    if(path[len-1] == path_delim)
        return 0;
    path[len] = path_delim;
    path[len+1] = 0;
    return 0;
}
    
static char remove_path_ending(char *path)
{
    long long len;
    if(!path)
        return -1;
    len = strlen(path);
    if(path[len-1] == path_delim)
        path[len-1] = 0;
    return 0;
}

static void aux_free(path_aux *aux)
{
    if(!aux)
        return;
    free(aux);
}

path_aux *path_init()
{
    char *ptr;
    char ok;
    path_aux *aux = malloc(sizeof(*aux));
    ptr = getcwd(aux->root_path, sizeof(aux->root_path));
    if(!ptr) {
        aux_free(aux);
        return NULL;
    }
    aux_set_crnt_path(aux->root_path, aux);
    ok = add_path_ending(aux->crnt_path, sizeof(aux->crnt_path));
    if(ok != 0) {
        aux_free(aux);
        return NULL;
    }
    return aux;
}

char *paths_union(const char *p1, const char *p2)
{
    char *result;
    long long len1, len2;
    if(!p1 || !p2)
        return NULL;
    len1 = strlen(p1);
    len2 = strlen(p2);
    result = malloc(sizeof(*result)*(len1+len2+1+1)); /* +1:'\0', +1:'/' */
    strcpy(result, p1);
    if(len2 == 0)
        return result;
    if(result[len1-1] != path_delim) {
        result[len1] = path_delim;
        result[len1+1] = 0;
    }
    strcat(result, p2);
    return result;
}

static char *get_full_path(const char *path, path_aux *aux)
{
    char *tmp;
    char ok;
    long long clen, plen;
    if(!path)
        return NULL;
    clen = strlen(aux->crnt_path);
    plen = strlen(path);
    if(clen+plen > sizeof(aux->crnt_path))
        return NULL;
    tmp = malloc(sizeof(*tmp)*sizeof(aux->crnt_path));
    memcpy(tmp, aux->crnt_path, clen+1);
    strcat(tmp, path);
    ok = add_path_ending(tmp, sizeof(aux->crnt_path));
    if(ok != 0) {
        free(tmp);
        return NULL;
    }
    return tmp;
}

static char path_update(char *path, const char *dirname)
{
    long long plen;
    if(!path)
        return -1;
    plen = strlen(path);
    if(strcmp(dirname, CRNT_LEVEL) == 0)
        return 0;
    if(strcmp(dirname, TOP_LEVEL) == 0) {
        long long i;
        path[plen] = 0;
        for(i = plen-1; path[i] && path[i] != path_delim; i--)
            path[i] = 0;
        path[i] = 0;
    }
    path[plen] = path_delim;
    strcpy(path+plen+1, dirname);
    return 0;
}

static char format_path(char *path)
{
    char *dup, *word;
    long long plen;
    if(!path)
        return -1;
    dup = strdup(path);
    path[0] = 0;
    while((word = strsep(&dup, PATH_DELIM_STR)) != NULL) {
        char ok;
        if(!*word)
            continue;
        ok = path_update(path, word);
        if(ok != 0)
            break;
    }
    if(!path) {
        free(dup);
        return -1;
    }
    plen = strlen(path);
    path[plen] = path_delim;
    path[plen+1] = 0;
    return 0;
}

static char is_valid_path(const char *fullpath, path_aux *aux)
{
    if(!fullpath)
        return 0;
    return strstr(fullpath, aux->root_path) != NULL;
}

char change_dir(const char *path, path_aux *aux)
{
    char *fullpath;
    char ok;
    fullpath = get_full_path(path, aux);
    if(!fullpath)
        return -1;
    ok = format_path(fullpath);
    if(ok != 0) {
        free(fullpath);
        return -1;
    }
    if(!is_valid_path(fullpath, aux)) {
        free(fullpath);
        return -1;
    }
    if(ok != 0) {
        free(fullpath);
        return -1;
    }
    ok = chdir(fullpath);
    if(ok == 0)
        aux_set_crnt_path(fullpath, aux);
    free(fullpath);
    return ok;
}

static char *get_short_path(const char *fullpath, path_aux *aux)
{
    char *dup, *match;
    long long rlen, i;
    char ok;
    if(!fullpath || !aux)
        return NULL;
    dup = strdup(fullpath);
    match = strstr(fullpath, aux->root_path);
    if(!match) {
        free(dup);
        return NULL;
    }
    rlen = strlen(aux->root_path);
    for(i = 0; i < rlen; i++)
        dup[i] = 0;
    ok = remove_path_ending(dup+rlen);
    if(ok != 0) {
        free(dup);
        return NULL;
    }
    return dup+rlen;
}

char print_path(path_aux *aux)
{
    printf("~%s$ ", aux->shrt_path);
    return 0;
}
