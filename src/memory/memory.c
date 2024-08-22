#include "memory.h"
#include "../logger/logger.h"

#include <string.h>
#include <assert.h>

#if defined (__linux__)
#include <sys/mman.h>
#else
#endif

#define RENDER_MEMORY_SIZE MEGABYTES(64)
#define SIM_MEMORY_SIZE MEGABYTES(64)
#define PERMANENT_MEMORY_SIZE MEGABYTES(64)
#define BULK_DATA_SIZE GIGABYTES(1);
#define MAX_ALLOCATION_COUNT 256

typedef struct 
{
    uint8_t *base;
    size_t   used;
    size_t   capacity;
}memory_arena_t;

typedef struct 
{
    void        *base;
    size_t       size;
    memory_tag_t type;
}allocation_t;

static memory_arena_t permanent_arena;
static memory_arena_t sim_arena;
static memory_arena_t render_arena;

static allocation_t allocations[MAX_ALLOCATION_COUNT];
static uint32_t allocation_count;

static void *memory_map(size_t size)
{
    void *mem = NULL;
#if defined (__linux__)
    mem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#else
    //VirtualAlloc(bla,bla);
#endif
    return mem;
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
    memset(&permanent_arena, 0, sizeof(permanent_arena));
    memset(&sim_arena, 0, sizeof(sim_arena));
    memset(&render_arena, 0, sizeof(render_arena));

    permanent_arena.base     = memory_map(PERMANENT_MEMORY_SIZE);
    permanent_arena.capacity = PERMANENT_MEMORY_SIZE;
    permanent_arena.used     = 0;

    sim_arena.base     = memory_map(SIM_MEMORY_SIZE);
    sim_arena.capacity = SIM_MEMORY_SIZE;
    sim_arena.used     = 0;

    render_arena.base     = memory_map(RENDER_MEMORY_SIZE);
    render_arena.capacity = RENDER_MEMORY_SIZE;
    render_arena.used     = 0;

    allocation_count      = 0;
}

void memory_uninit(void)
{
    if (munmap(permanent_arena.base, permanent_arena.capacity) < 0)
    {
        LOGE("Unable to deallocate permanent arena");
    }
    if (munmap(sim_arena.base, sim_arena.capacity) < 0)
    {
        LOGE("Unable to deallocate sim arena");
    }
    if (munmap(render_arena. base, render_arena.capacity) < 0)
    {
        LOGE("Unable to deallocate render arena");
    }

    memset(&permanent_arena, 0, sizeof(permanent_arena));
    memset(&sim_arena, 0, sizeof(sim_arena));
    memset(&render_arena, 0, sizeof(render_arena));

    for (uint32_t i = 0; i < allocation_count; i++)
    {
        allocation_t allocation = allocations[i];
        if (munmap(allocation.base, allocation.size) < 0)
        {
            LOGE("Unable to deallocate bulk allocation of type: %s", memory_tag_string(allocation.type));
        }
    }

    memset(allocations, 0, sizeof(allocations));
    LOGI("Uninitialised all memory");
}

void *memory_alloc(size_t size, memory_tag_t tag)
{
    void *mem = NULL;
    switch(tag)
    {
        case MEM_TAG_BULK_DATA:
        {
            mem = memory_map(size);
            assert(mem);
            break;
        }
        case MEM_TAG_SIM:
        {
            mem = memory_arena_push_size(&sim_arena, size);
            break;
        }
        case MEM_TAG_PERMANENT:
        {
            mem = memory_arena_push_size(&permanent_arena, size);
            break;
        }
        case MEM_TAG_RENDER:
        {
            mem = memory_arena_push_size(&render_arena, size);
            break;
        }
        default:
        {
            LOGE("Unknown allocation type!!");
            break;
        }
    }
    assert(mem);
    return mem;
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
        {
            memory_arena_begin(&sim_arena);
            break;
        }
        case MEM_TAG_PERMANENT:
        {
            memory_arena_begin(&permanent_arena);
            break;
        }
        case MEM_TAG_RENDER:
        {
            memory_arena_begin(&render_arena);
            break;
        } 
        default:
        {
            LOGE("Invalid request to begin memory of type: %s", memory_tag_string(tag));
            break;
        }
    }
}

