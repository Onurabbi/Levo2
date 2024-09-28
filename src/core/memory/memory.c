#include "memory.h"
#include "heap_alloc.h"

#include "../logger/logger.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined (__linux__)
#include <sys/mman.h>
#else
#endif

#define RENDER_MEMORY_SIZE MEGABYTES(64)
#define SIM_MEMORY_SIZE MEGABYTES(64)
#define PERMANENT_MEMORY_SIZE MEGABYTES(64)
#define BULK_DATA_SIZE GIGABYTES(1);
#define MAX_MAPPING_COUNT 1024

enum
{
    MMAP,
    MALLOC
};

static uint32_t heap_sizes[] = {64, 128, 256, 512, 1024, 2048, 4096};

typedef struct
{
    int32_t  next;
    uint32_t size;
}block_header_t;

typedef struct
{
    uint8_t *data;
    int32_t  free_list;
    uint32_t block_count;
    uint32_t block_size;
}memory_heap_t;

typedef struct 
{
    uint8_t *base;
    uint32_t used;
    uint32_t capacity;
}memory_arena_t;

typedef struct 
{
    void        *base;
    uint32_t     size;
    int          type;
}mapping_t;

static memory_heap_t heaps[sizeof(heap_sizes)/sizeof(heap_sizes[0])];

static memory_arena_t permanent_arena;
static memory_arena_t sim_arena;
static memory_arena_t render_arena;

static mapping_t mappings[MAX_MAPPING_COUNT];
static uint32_t mapping_count;

static void *memory_map(uint32_t size)
{
    void *mem = NULL;
#if defined (__linux__)
    mem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#else
    //VirtualAlloc(bla,bla);
#endif
    mapping_t *mapping = &mappings[mapping_count++];
    mapping->base = mem;
    mapping->size = size;
    mapping->type = MMAP;
    return mem;
}

static void *memory_arena_push_size(memory_arena_t *arena, uint32_t size)
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
    mapping_count = 0;

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

    for (uint32_t i = 0; i < sizeof(heap_sizes)/sizeof(heap_sizes[0]); i++)
    {
        memory_heap_t *heap = &heaps[i];
        heap->block_count = 0;
        heap->block_size  = heap_sizes[i];
        heap->data        = memory_map(GIGABYTES(1));
        heap->free_list   = -1;
    }
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

    for (uint32_t i = 0; i < mapping_count; i++)
    {
        mapping_t mapping = mappings[i];
        if (mapping.type == MMAP)
        {
            if (munmap(mapping.base, mapping.size) < 0) {
                LOGE("Unable to unmap memory");
            }
        } else if (mapping.type == MALLOC) {
            LOGE("Forgot to deallocate malloced memory %u bytes in size", mapping.size);
            free(mapping.base);
        } else {
            assert(false && "Unknown allocation type");
        }
    }

    memset(mappings, 0, sizeof(mappings));
    LOGI("Uninitialised all memory");
}

static int32_t index_of_block(block_header_t *header, memory_heap_t *heap)
{
    ptrdiff_t diff_bytes = (uint8_t*)header - heap->data;
    return (int32_t)(diff_bytes / (sizeof(block_header_t) + heap->block_size));
}

static inline block_header_t *block_at_index(memory_heap_t *heap, int32_t index)
{
    return (block_header_t *)&heap->data[index * (sizeof(block_header_t) + heap->block_size)];
}

static inline void *data_at_index(memory_heap_t *heap, int32_t index)
{
    return (&heap->data[index * (sizeof(block_header_t) + heap->block_size) + sizeof(block_header_t)]);
}

#if 0
static inline uint32_t align(uint32_t size)
{
    return (size + sizeof(uintptr_t) - 1) & ~(sizeof(uintptr_t) - 1);
}
#endif

static inline block_header_t *get_header(void *ptr)
{
    return (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
}

static memory_heap_t *select_heap(uint32_t size)
{
    for (uint32_t i = 0; i < sizeof(heaps)/sizeof(heaps[0]); i++)
    {
        memory_heap_t *heap = &heaps[i];
        if (size <= heap->block_size)
        {
            return heap;
        }
    }

    return NULL; //malloc-free
}

void heap_free(void *ptr, uint32_t size)
{
    memory_heap_t *heap = select_heap(size);
    if (!heap)
    {
        bool found = false;
        //! NOTE: This uses linear search now. change this to a hash map or something if it is a problem
        for (uint32_t i = 0; i < mapping_count; i++)
        {
            mapping_t *mapping = &mappings[i];
            if (mapping->base == ptr)
            {
                found = true;
                *mapping = mappings[mapping_count - 1];
                mapping_count--;
                break;
            }
        }
        assert(found && "unable to find the memory provided in the allocation list");
        free(ptr);
        return;
    }

    block_header_t *header = get_header(ptr);
    header->next = heap->free_list;
    heap->free_list = index_of_block(header, heap);
}

void *heap_alloc(uint32_t size)
{
    //select heap first
    memory_heap_t *heap = select_heap(size);

    if (!heap)
    {
        mapping_t *mapping = &mappings[mapping_count++];
        mapping->base = malloc(size);
        mapping->size = size;
        mapping->type = MALLOC;
        return mapping->base;
    }

    void *ptr = NULL;

    if (heap->free_list > -1) {
        block_header_t *header = block_at_index(heap, heap->free_list);
        ptr = &header[1];
        heap->free_list = header->next;
        header->next = -1;
    } else {
        ptr = data_at_index(heap, heap->block_count);
        heap->block_count++;
    }
    return ptr;
}

void memory_dealloc(void *memory, uint32_t size, memory_tag_t tag)
{
    switch(tag)
    {
        case MEM_TAG_HEAP:
            heap_free(memory, size);
            break;
        default:
            LOGE("Unable to deallocate memory other than MEM_TAG_HEAP");
            break;
    }
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

