#include "memory_arena.h"

#include <sys/mman.h>
#include <assert.h>

#define SCRATCH_MEMORY_SIZE MEGABYTES(256)

static memory_arena_t scratch_arena = {0};

memory_arena_t *scratch_allocator(void)
{
    return &scratch_arena;
}

void memory_arena_init(void)
{
    scratch_arena.base = mmap(0, 
                         SCRATCH_MEMORY_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS,
                         -1,
                         0);
    assert(scratch_arena.base);
    scratch_arena.capacity = SCRATCH_MEMORY_SIZE;
    scratch_arena.used     = 0;
}

void memory_arena_begin(memory_arena_t *arena)
{
    arena->used = 0;
}

void *memory_arena_push_size(memory_arena_t *arena, size_t size)
{
    void *result = NULL;
    if (arena->used + size <= arena->capacity)
    {
        result = arena->base + arena->used;
        arena->used += size;
    }
    return result;
}
