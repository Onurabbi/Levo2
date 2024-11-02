#ifndef CONTAINERS_H_
#define CONTAINERS_H_

#include "../memory/memory.h"

typedef struct 
{
    uint64_t key;
    uint32_t value;
} index_hash_entry_t;

typedef struct 
{
    uint64_t key;
    uint64_t value;
}signature_hash_entry_t;

typedef struct 
{
    const char* key;
    uint32_t    value;
} string_hash_entry_t;

#define hash_map_init(hm,v)  \
do                           \
{                            \
    (hm) = NULL;             \
    hmdefault((hm),(v));     \
} while (0)


#endif

