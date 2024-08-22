#include "string.h"
#include "../memory/memory.h"

#include <assert.h>
#include <string.h>

const char *string_duplicate(const char* str)
{
    char *result = NULL;
    size_t len = strlen(str);

    result = memory_alloc(len + 1, MEM_TAG_PERMANENT);
    assert(result);

    memmove(result, str, len);
    result[len] = '\0';

    return result;
}

