#include "fslib.h"
#include "strlib.h"
#include "memlib.h"
#include <sys/stat.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define CRNT_LEVEL "."
#define TOP_LEVEL ".."

char create_block(const char *dirname, const char *filename, int *pfd)
{
    char ok;
    int fd;
    char *fullname;
    ok = mkdir(dirname, 0777);
    if(ok != 0)
        return -1;
    fullname = strings_concatenate(dirname, "/", filename, NULL);
    fd = open(fullname, O_WRONLY|O_CREAT, 0666);
    if(fd == -1)
        return -2;
    *pfd = fd;
    return 0;
}

static char *get_fullname(char *dest, unsigned long long dsize, 
    const char *name, const char *base)
{
    long long blen, nlen;
    if(!dest)
        return NULL;
    blen = strlen(base);
    nlen = strlen(name);
    if(blen+nlen > dsize)
        return NULL;
    strcpy(dest, base);
    if(blen > 0) {
        dest[blen] = '/';
        dest[blen+1] = 0;
    }
    strcat(dest, name);
    return dest;
}

enum { buffer_size = 2048 };
static char remove_dir_aux(const char *name, const char *base)
{
    char buf[buffer_size];
    struct dirent *dent;
    char *fullname;
    char stat;
    DIR *dir;
    if(!name || !base)
        return -1;
    if((strcmp(name, CRNT_LEVEL) == 0) || (strcmp(name, TOP_LEVEL) == 0))
        return 0;
    fullname = get_fullname(buf, sizeof(buf), name, base);
    if(!fullname)
        return -1;
    dir = opendir(fullname);
    if(!dir) /* we guess that it's a file then */
        return unlink(fullname);
    while((dent = readdir(dir)) != NULL) {
        stat = remove_dir_aux(dent->d_name, fullname);
        if(stat != 0)
            break;
    }
    closedir(dir);
    stat = rmdir(fullname);
    if(stat != 0)
        return -1;
    return 0;
}

char remove_dir(const char *name)
{
	if(!name)
        return -1;
    return remove_dir_aux(name, "");
}

char *fgets_m(char *s, int size, FILE *stream)
{
    char *result = fgets(s, size, stream);
    long long len;
    if(!result)
        return NULL;
    len = strlen(s);
    if(s[len-1] == '\n')
        s[strlen(s)-1] = 0;
    return result;
}
