#ifndef MEMORY_TYPES_H_
#define MEMORY_TYPES_H_

#include <stdint.h>
#include <stddef.h>

#define KILOBYTES(a) (1024UL * (a))
#define MEGABYTES(a) (1024UL * KILOBYTES((a)))
#define GIGABYTES(a) (1024UL * MEGABYTES((a)))

typedef enum 
{
    MEM_TAG_PERMANENT = 0,
    MEM_TAG_SIM,
    MEM_TAG_TEMP = MEM_TAG_SIM,
    MEM_TAG_RENDER,
    MEM_TAG_BULK_DATA,
    MEM_TAG_HEAP,
    MEM_TAG_UNKNOWN
}memory_tag_t;

#endif


