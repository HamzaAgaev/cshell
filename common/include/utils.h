#ifndef OS_LAB_1_UTILS_H
#define OS_LAB_1_UTILS_H

#define INPUT_FILENAME "input.txt"
#define TEMP_FILENAME_FORMAT "%s-temp-%d.txt"
#define OUTPUT_FILENAME_FORMAT "%s-output.txt"

#include <time.h>

#define DEFAULT_SEED ((unsigned int)time(NULL))
#define GEN_STR_LEN 4
#define MAX_FILENAME_LEN 20

#define SUCCESS_CODE 0
#define DEFAULT_ERROR_CODE (-1)

typedef struct {
    char *message;
    int statusCode;
} RunResult;

typedef struct {
    int statusCode;
} ErrorCatcher;

void generateRandomString(char *str, int length, unsigned int seed);

int compare(const void *a, const void *b);

#endif// OS_LAB_1_UTILS_H
