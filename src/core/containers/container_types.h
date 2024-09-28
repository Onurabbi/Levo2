#ifndef CONTAINER_TYPES_H_
#define CONTAINER_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t id;
    uint32_t generation;
}weak_pointer_t;

typedef struct 
{
    uint32_t next;
}freelist_item_t;

typedef enum
{
    FREELIST_ITEM,
    OBJECT_ITEM
}data_type_t;

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

#define BulkDataTypes(type)                                         \
typedef struct {                                                    \
    type data;                                                      \
}object_##type;                                                     \
                                                                    \
typedef struct {                                                    \
    uint32_t generation;                                            \
    union {                                                         \
        object_##type;                                              \
        freelist_item_t;                                            \
    };                                                              \
    data_type_t data_type;                                          \
}item_##type;                                                       \
                                                                    \
typedef struct {                                                    \
    item_##type     *items;                                         \
    uint32_t slot;                                                  \
    uint32_t count;                                                 \
}bulk_data_##type
#endif

