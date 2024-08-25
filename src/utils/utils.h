#ifndef UTILS_H_
#define UTILS_H_

#define bit_set(num,bit) ((1ULL << (bit)) | (num))
#define bit_clear(num,bit) (~(1ULL << (bit)) & (num))
#define bit_check(num,bit) ((1ULL << (bit)) & (num))
#define bit_flip(num,bit) ((1ULL << (bit)) ^ (num))

char *read_whole_file(const char *file_path, long *size);

#endif


