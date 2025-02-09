#ifndef CSHELL_FILE_STATE_H
#define CSHELL_FILE_STATE_H

#include "utils.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    int fd;
    int value;
    bool isEndOfFile;
} FileState;

void initializeFileState(FileState *fileState, const char *filename, ErrorCatcher *catcher);

void updateFileState(FileState *fileState);

void closeFileState(FileState *fileState);

#endif// CSHELL_FILE_STATE_H
