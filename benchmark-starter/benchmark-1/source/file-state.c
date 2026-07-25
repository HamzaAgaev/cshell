#include "file-state.h"

#include "benchmark-utils.h"
#include "cio.h"

#include <errno.h>

void initializeFileState(FileState *fileState, const char *filename, ErrorHandler *handler) {
    fileState->fd = cioOpen(filename);
    if (fileState->fd == -1) {
        handler->statusCode = errno;
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
    cioClose(fileState->fd);
}
