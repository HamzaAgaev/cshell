#ifndef CSHELL_UTILS_H
#define CSHELL_UTILS_H

#define SUCCESS_CODE 0
#define DEFAULT_ERROR_CODE (-1)

typedef struct {
    char *message;
    int statusCode;
} RunResult;

typedef struct {
    int statusCode;
} ErrorHandler;

void generateRandomString(char *str, int length, unsigned int seed);

int compare(const void *a, const void *b);

#endif// CSHELL_UTILS_H
