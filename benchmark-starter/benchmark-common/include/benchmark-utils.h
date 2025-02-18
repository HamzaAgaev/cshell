#ifndef OS_LAB_2_BENCHMARK_UTILS_H
#define OS_LAB_2_BENCHMARK_UTILS_H

#define INPUT_FILENAME "input.txt"
#define TEMP_FILENAME_FORMAT "%s-temp-%d.txt"
#define OUTPUT_FILENAME_FORMAT "%s-output.txt"

#include <stddef.h>
#include <time.h>

#define DEFAULT_SEED ((unsigned int)time(NULL))
#define GEN_STR_LEN 4
#define MAX_FILENAME_LEN 20
#define MAX_CHARACTERS_FOR_INT 16

size_t readIntFromFile(int fd, int *buf);

#endif//OS_LAB_2_BENCHMARK_UTILS_H
