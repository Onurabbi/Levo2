#ifndef STRING_H_
#define STRING_H_

#include "../memory/memory_types.h"

const char *format_string(memory_tag_t tag,const char *format, ...);
const char *string_duplicate(const char* string, memory_tag_t tag);
const char *string_concatenate(const char *str1, const char *str2, memory_tag_t tag);

const char *find_last_of(const char *str, const char *delim);
const char *file_name_wo_extension(const char *file_nam, memory_tag_t tag);

#endif

