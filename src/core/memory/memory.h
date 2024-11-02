#ifndef MEMORY_H_
#define MEMORY_H_

#include "memory_types.h"
#include <stddef.h>

/**
 * @brief: Initialise the memory system. There are three memory arenas: 
 * - Permanent arena: For allocations with the lifetime of entire duration of the game (or level).
 * - Simulation  arena: For allocations with the lifetime of game update cycle (1/60 seconds).
 * - Render arena: For allocations with the lifetime of game render cycle (Depends on Frames Per Second)
 */
void  memory_init(void);
/**
 * @brief: Deallocate all memory.
 */
void memory_uninit(void);
/**
 * @brief: For things that go on an arena, this works like malloc.
 *         For bulk data, it works like mmap
 */
void *memory_alloc(uint32_t size, memory_tag_t tag); 
/**
 * @brief: Only pass memory allocated on heap here.
 */
void memory_dealloc(void *mem);
/**
 * @brief: For arenas, this works like a reset. For other types of memory, this does nothing
 */
void  memory_begin(memory_tag_t tag);
#endif

