#ifndef OS_LAB_1_FILE_STATE_H
#define OS_LAB_1_FILE_STATE_H

#include "utils.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    int fd;
    int value;
    bool isEndOfFile;
} FileState;

void initializeFileState(FileState *fileState, const char *filename, ErrorHandler *handler);

void updateFileState(FileState *fileState);

void closeFileState(FileState *fileState);

#endif// OS_LAB_1_FILE_STATE_H
