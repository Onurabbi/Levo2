#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define LOGE(string, ...) fprintf(stderr, ANSI_COLOR_RED "%s:%u " string"\n" ANSI_COLOR_RESET, __func__, __LINE__, ##__VA_ARGS__)
#define LOGI(string, ...) fprintf(stdout, ANSI_COLOR_GREEN "%s:%u " string"\n" ANSI_COLOR_RESET, __func__, __LINE__, ##__VA_ARGS__)

#endif
