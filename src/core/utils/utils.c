#include "utils.h"
#include <memory.h>
#include <logger.h>

#include <stdio.h>
#include <assert.h>

uint8_t *read_whole_file(const char *file_path, long *size, memory_tag_t tag)
{
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        LOGE("Unable to load %s\n", file_path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *file_buf = memory_alloc(file_size, tag);
    assert(file_buf);

    fread(file_buf, 1, file_size, file);
    fclose(file);
    
    if (size) *size = file_size;

    return file_buf;
}
