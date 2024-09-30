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

long long memory_items_count(const void **mem)
{
	const void **tmp = mem;
	while(*mem)
		mem++;
	return mem-tmp;
}
