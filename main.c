#include "shell.h"
#include <stdio.h>


int main(int argc, const char *argv[])
{
    char status = shell_run(argc, argv);
    return status;
}
