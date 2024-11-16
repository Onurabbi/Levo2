#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_

#include <memory_types.h>

const char *string_format(memory_tag_t tag,const char *format, ...);
const char *string_duplicate(const char* string, memory_tag_t tag);
const char *string_concatenate(const char *str1, const char *str2, memory_tag_t tag);

const char *string_find_last_of(const char *str, const char *delim);
const char *string_replace_character(const char *str, char from, char to, memory_tag_t tag);
const char *string_get_file_name_wo_extension(const char *file_name, memory_tag_t tag);

//! @brief returns the file directory of an asset INCLUDING the last slash (or backslash)
const char *string_get_file_directory(const char *file_path, memory_tag_t tag);
#endif

