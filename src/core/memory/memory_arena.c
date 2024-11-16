#include <string.h>
#include <stdint.h>

typedef struct 
{
    uint8_t *base;
    uint32_t used;
    uint32_t capacity;
}memory_arena_t;

#define RENDER_MEMORY_SIZE MEGABYTES(64)
#define SIM_MEMORY_SIZE MEGABYTES(64)
#define PERMANENT_MEMORY_SIZE MEGABYTES(64)

static memory_arena_t permanent_arena;
static memory_arena_t sim_arena;
static memory_arena_t render_arena;

void *memory_arena_push_size(memory_arena_t *arena, uint32_t size)
{
    void *result = NULL;
    if (arena->used + size <= arena->capacity)
    {
        result = arena->base + arena->used;
        arena->used += size;
    }
    return result;
}

void memory_arena_init(void) 
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
}

void memory_arena_uninit(void)
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
}
