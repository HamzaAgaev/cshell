#include "file-state.h"

#include <errno.h>

void initializeFileState(FileState *fileState, const char *filename, ErrorCatcher *catcher) {
    fileState->file = fopen(filename, "rb");
    if (fileState->file == NULL) {
        catcher->statusCode = errno;
        return;
    }
    fileState->isEndOfFile = false;
    updateFileState(fileState);
}

void updateFileState(FileState *fileState) {
    if (fscanf(fileState->file, "%d", &fileState->value) != 1) {
        fileState->isEndOfFile = true;
    }
}

void closeFileState(FileState *fileState) {
    fclose(fileState->file);
}
