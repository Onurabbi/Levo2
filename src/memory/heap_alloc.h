#ifndef HEAP_ALLOC_H_
#define HEAP_ALLOC_H_

#include <stdint.h>

void *heap_alloc(uint32_t size);
void  heap_free(void *block, uint32_t size);

#endif

