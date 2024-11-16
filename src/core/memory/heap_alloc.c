#include <stdint.h>

#if defined (__linux__)
#include <sys/mman.h>
#else
#endif

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <logger.h>

#define MAX_ALLOC_COUNT 16384
#define ALIGNMENT_BYTES 8

static uint8_t *heap_base;
static uint8_t *heap_ptr;

typedef struct {
    uint8_t  *base;
    uint64_t size;
}heap_chunk_t;

typedef struct {
    uint64_t size;
}allocation_header_t;

static heap_chunk_t free_list[MAX_ALLOC_COUNT];
static uint32_t free_chunk_count;

static inline uintptr_t align_address(void* address, uint64_t alignment) 
{
    return ((uintptr_t)address + alignment - 1) & ~(alignment - 1);
}

static inline void add_chunk(heap_chunk_t *chunk) 
{
    free_list[free_chunk_count++] = *chunk;
}

static inline void remove_chunk(heap_chunk_t *chunk)
{
    //overwrite with the last chunk
    memcpy(chunk, &free_list[free_chunk_count - 1], sizeof(heap_chunk_t));
    free_chunk_count--; 
}

static inline allocation_header_t *get_header(uint8_t *mem) 
{
    //make sure the address is aligned correctly
    assert(((uintptr_t)mem % ALIGNMENT_BYTES) == 0);
    return &(((allocation_header_t *)mem)[-1]);
}

//this does not remove anything, just returns the first fit, that's it
static heap_chunk_t *find_first_fit(uint64_t size) {
    for (uint32_t i = 0; i < free_chunk_count; i++) {
        heap_chunk_t *chunk = &free_list[i];
        if (chunk->size >= size) {
            return chunk;
        }
    }
    return NULL;
}

void heap_free(void *mem)
{
    allocation_header_t *header = &(((allocation_header_t *)mem)[-1]);
    /**
     * look for adjacent chunks in either side of this one.
     * if just one found, update that one's header and chunk info with new info
     * if both are found, update the smaller one's data, remove the larger one
     * if nothing is found, create a new chunk and add to the list
     */

    heap_chunk_t *before = NULL;
    heap_chunk_t *after  = NULL;

    uint8_t *mem_min = (uint8_t*)header;
    uint8_t *mem_max = (uint8_t*)align_address((uint8_t*)mem + header->size, ALIGNMENT_BYTES);
    //look for before and after
    for (uint32_t i = 0; i < free_chunk_count; i++) {
        heap_chunk_t *chunk = &free_list[i];
        uint8_t *min = chunk->base - sizeof(allocation_header_t);
        uint8_t *max = (uint8_t *)align_address(chunk->base + chunk->size, ALIGNMENT_BYTES);

        if (min == mem_max) {
            after = chunk;
        } else if (max == mem_min) {
            before = chunk;
        }

        if (before && after) {
            break;
        }
    }
    //update chunks
    if (before && after) {
        //put everything into before, and remove the after.
        //calculate the end of the buffer
        uint8_t *end = (uint8_t *)align_address(after->base + after->size, ALIGNMENT_BYTES);
        allocation_header_t *before_header = &(((allocation_header_t *)before->base)[-1]);
        before_header->size = (uint64_t)(end - before->base);

        //before base can stay the same.
        //sync chunk and header sizes
        before->size = before_header->size;
        remove_chunk(after);
    } else if (!before && after) {
        //combine with after
        uint8_t *end = (uint8_t *)align_address(after->base + after->size, ALIGNMENT_BYTES);
        header->size = (uint64_t)(end - (uint8_t*)mem);
        after->base = mem;
        after->size = header->size;
    } else if (before && !after) {
        uint8_t *end = (uint8_t *)align_address((uint8_t*)mem + header->size, ALIGNMENT_BYTES);
        allocation_header_t *before_header = &(((allocation_header_t *)before->base)[-1]);
        before_header->size = (uint64_t)(end - before->base);
        before->size = before_header->size;
    } else {
        //create new chunk
        heap_chunk_t *chunk = &free_list[free_chunk_count++];
        chunk->size = header->size;
        chunk->base = mem;
    }
}

//8 byte chunk size embedded just before the data
void *heap_alloc(uint64_t size)
{
    allocation_header_t *result = NULL;
    heap_chunk_t *free_chunk = find_first_fit(size);
    if (free_chunk) {
        //cache the chunk end
        uint8_t *chunk_end = (uint8_t*)align_address(free_chunk->base + free_chunk->size, ALIGNMENT_BYTES);

        //cache the result and update the header
        result = &(((allocation_header_t *)free_chunk->base)[-1]);

        //decide to split
        if (free_chunk->size - size > sizeof(allocation_header_t)) {

            //update the chunk size
            result->size = size;
            /**
             * now split this chunk. move the base, reduce the size. create a new header just before the new base,
             */
            uint8_t *new_base = (uint8_t*)align_address(free_chunk->base + size + sizeof(allocation_header_t), ALIGNMENT_BYTES);
            uint64_t new_size = (uint64_t)(chunk_end - new_base);
            //create the new header
            allocation_header_t *new_header = &(((allocation_header_t *)new_base)[-1]);
            new_header->size = new_size;
            //update the chunk
            free_chunk->base = new_base;
            free_chunk->size = new_size;
        } else {
            //simply remove this chunk
            //size of the chunk is not updated since we are using all of it
            remove_chunk(free_chunk);
        }
    } else {
        //new allocation
        result = (allocation_header_t *)heap_ptr;
        result->size = size;
        //now push the ptr forward
        heap_ptr = (uint8_t *)align_address(heap_ptr + size + sizeof(allocation_header_t), ALIGNMENT_BYTES);
    }
    //clear the memory to zero
    return (&result[1]);
}

void heap_init(void)
{
    heap_base = memory_map(GIGABYTES(4ULL));
    heap_ptr = heap_base;
}

void heap_uninit(void)
{
    /**
     * TODO: For now, this works as a boolean memory leak or not detector.
     * If we free'd all memory_allocs, then we should have one free chunk with a size equal to 8 bytes (header) less than total
     * memory used by heap. At some point we need to have a map of each allocations to tell which allocation was forgotten.
     */
    //ptrdiff_t allocated = (ptrdiff_t)(heap_ptr - heap_base);
    //assert((free_chunk_count > 0) && (free_list[0].size == (allocated - 8)));
}


