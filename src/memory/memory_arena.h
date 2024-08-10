#ifndef MEMORY_ARENA_H_
#define MEMORY_ARENA_H_

#include <stdint.h>
#include <stddef.h>

#define KILOBYTES(a) (1024UL * (a))
#define MEGABYTES(a) (1024UL * KILOBYTES((a)))
#define GIGABYTES(a) (1024UL * MEGABYTES((a)))

typedef struct 
{
    uint8_t *base;
    size_t   used;
    size_t   capacity;
}memory_arena_t;

memory_arena_t *scratch_allocator(void);
memory_arena_t *string_allocator(void);

void  memory_arena_init(void);
void  memory_arena_begin(memory_arena_t *arena);
void *memory_arena_push_size(memory_arena_t *arena, size_t size);

#define memory_arena_push_array(arena, type, count) (memory_arena_push_size((arena), (count) * sizeof(type)))
#define memory_arena_push_struct(arena, type) (memory_arena_push_array((arena), (type), 1))
#endif

