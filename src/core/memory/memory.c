#include "memory.h"

#include <logger.h>

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined (__linux__)
#include <sys/mman.h>
#else
#endif


#define BULK_DATA_SIZE GIGABYTES(1);

#include "memory_internal.c"
#include "heap_alloc.c"
#include "memory_arena.c"

static const char *memory_tag_string(memory_tag_t tag)
{
    const char *result = NULL;
    switch(tag)
    {
        case(MEM_TAG_PERMANENT):
        {
            result = "MEMORY_TAG_PERMANENT";
            break;
        }
        case(MEM_TAG_SIM):
        {
            result = "MEMORY_TAG_SIM";
            break;
        }
        case(MEM_TAG_RENDER):
        {
            result = "MEMORY_TAG_RENDER";
            break;
        }
        case(MEM_TAG_BULK_DATA):
        {
            result = "MEM_TAG_BULK_DATA";
            break;
        }
        case(MEM_TAG_HEAP):
        {
            result = "MEM_TAG_HEAP";
            break;
        }
        default:
        {
            result = "NULL";
            break;
        }
    }
    return result;
}

void memory_init(void)
{
    memory_internal_init();
    memory_arena_init();
    heap_init();
}

void memory_uninit(void)
{
    memory_arena_uninit();
    heap_uninit();
    memory_internal_uninit();
}

void *memory_alloc(uint32_t size, memory_tag_t tag)
{
    void *mem = NULL;
    switch(tag)
    {
        case MEM_TAG_BULK_DATA:
            mem = memory_map(size);
            assert(mem);
            return mem;
        case MEM_TAG_SIM:
            mem = memory_arena_push_size(&sim_arena, size);
            break;
        case MEM_TAG_PERMANENT:
            mem = memory_arena_push_size(&permanent_arena, size);
            break;
        case MEM_TAG_RENDER:
            mem = memory_arena_push_size(&render_arena, size);
            break;
        case MEM_TAG_HEAP:
            mem = heap_alloc(size);
            break;
        default:
            LOGE("Unknown allocation type!!");
            break;
    }
    assert(mem);
    memset(mem, 0, size);
    return mem;
}

void memory_dealloc(void *mem)
{
    heap_free(mem);
}

void memory_arena_begin(memory_arena_t *arena)
{
    arena->used = 0;
}

void memory_begin(memory_tag_t tag)
{
    switch(tag)
    {
        case MEM_TAG_SIM:
            memory_arena_begin(&sim_arena);
            break;
        case MEM_TAG_PERMANENT:
            memory_arena_begin(&permanent_arena);
            break;
        case MEM_TAG_RENDER:
            memory_arena_begin(&render_arena);
            break;
        default:
            LOGE("Invalid request to begin memory of type: %s", memory_tag_string(tag));
            break;
    }
}

