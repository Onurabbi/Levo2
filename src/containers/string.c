#include "string.h"
#include "../memory/memory_arena.h"

#include <assert.h>
#include <string.h>

const char *string_duplicate(const char* str)
{
    char *result = NULL;
    size_t len = strlen(str);

    result = memory_arena_push_array(string_allocator(), char, len + 1);
    assert(result);

    memmove(result, str, len);
    result[len] = '\0';

    return result;
}

