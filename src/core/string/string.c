#include "string.h"
#include "../memory/memory.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

const char *format_string(memory_tag_t tag,const char *format,  ...)
{
    char buf[1024];
    memset(buf, 0, 1024);
    va_list args;
    va_start(args, format);
    vsnprintf(buf, 1024, format, args);
    va_end(args);

    size_t len = strlen(buf);
    char *result = memory_alloc(len + 1, tag);
    memmove(result, buf, len);
    result[len] = '\0';
    return result;
}

const char *string_concatenate(const char *str1, const char *str2, memory_tag_t tag)
{
    char *result = NULL;
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    result = memory_alloc(len1 + len2 + 1, tag);
    assert(result);

    memmove(result, str1, len1);
    memmove(result + len1, str2, len2);
    result[len1+len2] = '\0';

    return result;
}

const char *string_duplicate(const char* str, memory_tag_t tag)
{
    char *result = NULL;
    size_t len = strlen(str);

    result = memory_alloc(len + 1, tag);
    assert(result);

    memmove(result, str, len);
    result[len] = '\0';

    return result;
}

static inline bool find_in_delim(const char *delim, char c)
{
    const char *ptr = delim;
    while(*ptr) {
        if (*ptr++ == c) return true;
    }
    return false;
}

const char *find_last_of(const char *str, const char *delim)
{
    const char *last_delim = NULL;
    const char *ptr        = str;

    while (*ptr) {
        if (find_in_delim(delim, *ptr)) last_delim = ptr;
        ptr++;
    }
    if (last_delim) return last_delim + 1;
    return NULL;
}

const char *file_name_wo_extension(const char *file_name, memory_tag_t tag)
{
    const char *start = find_last_of(file_name, "/\\");
    const char *end   = start;

    while (*end) {
        if (*end == '.') break;
        end++;
    }
    
    size_t len = end - start;

    char *result = memory_alloc(len + 1, tag);
    memmove(result, start, len);
    result[len] = '\0';

    return result;
}