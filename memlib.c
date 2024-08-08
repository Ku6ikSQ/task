#include "memlib.h"
#include <stdlib.h>

void mem_free(void **mem)
{
    if(!mem)
        return;
    while(*mem) {
        free(*mem);
        mem++;
    }
}
