#include "file-state.h"

#include "benchmark-utils.h"
#include "lab2.h"

#include <errno.h>

void initializeFileState(FileState *fileState, const char *filename, ErrorCatcher *catcher) {
    fileState->fd = lab2Open(filename);
    if (fileState->fd == -1) {
        catcher->statusCode = errno;
        return;
    }
    fileState->isEndOfFile = false;
    updateFileState(fileState);
}

void updateFileState(FileState *fileState) {
    if (readIntFromFile(fileState->fd, &fileState->value) == 0) {
        fileState->isEndOfFile = true;
    }
}

void closeFileState(FileState *fileState) {
    lab2Close(fileState->fd);
}
