#include <stdint.h>
#if defined (__linux__)
#include <sys/mman.h>
#else
#endif

#define MAX_MAPPING_COUNT 1024

enum
{
    MMAP,
    HEAP,
};

typedef struct 
{
    void        *base;
    uint64_t     size;
    int          type;
}mapping_t;

static mapping_t mappings[MAX_MAPPING_COUNT];
static uint32_t mapping_count;

void memory_internal_init(void)
{
    mapping_count = 0;
}

void memory_internal_uninit(void)
{
    for (uint32_t i = 0; i < mapping_count; i++)
    {
        mapping_t mapping = mappings[i];
        if (mapping.type == MMAP ||
            mapping.type == HEAP)
        {
            if (munmap(mapping.base, mapping.size) < 0) {
                LOGE("Unable to unmap memory");
            }
        } else {
            assert(false && "Unknown allocation type");
        }
    }

    memset(mappings, 0, sizeof(mappings));
    LOGI("Uninitialised all memory");
}

void *memory_map(uint64_t size)
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

