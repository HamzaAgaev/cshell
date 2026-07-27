#include "benchmark-1.h"

#include "benchmark-utils.h"
#include "cio.h"
#include "file-state.h"
#include "heap-lib.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B1_TEMP_FILENAME_FORMAT "b1-" TEMP_FILENAME_FORMAT
#define B1_OUTPUT_FILENAME_FORMAT "b1-" OUTPUT_FILENAME_FORMAT

#define BLOCK_SCALE 16

static int ceilDiv(int a, int b) {
    return a / b + (a % b != 0);
}

static int getNel(int i, int blockSize, int numbersCount) {
    if ((i + 1) % blockSize == 0) {
        return blockSize;
    }
    return numbersCount % blockSize;
}

RunResult benchmark1() {
    int inputFd = cioOpen(INPUT_FILENAME);
    if (inputFd == -1) {
        return (RunResult){"Can't open input file.", errno};
    }

    int numbersCount;
    readIntFromFile(inputFd, &numbersCount);

    const int blockSize = (numbersCount / BLOCK_SCALE != 0) ? (numbersCount / BLOCK_SCALE) : numbersCount;
    int *const block = (int *)malloc(blockSize * sizeof(int));
    if (block == NULL) {
        cioClose(inputFd);
        return (RunResult){"Can't allocate memory.", errno};
    }

    const int filesCount = ceilDiv(numbersCount, blockSize);
    char **fileNames = malloc(filesCount * sizeof(char *));//[filesCount][MAX_FILENAME_LEN]
    if (fileNames == NULL) {
        cioClose(inputFd);
        free(block);
        return (RunResult){"Can't allocate memory.", errno};
    }
    char fileNamePrefix[GEN_STR_LEN + 1];
    generateRandomString(fileNamePrefix, GEN_STR_LEN + 1, DEFAULT_SEED);

    for (int i = 0; i < numbersCount; i++) {
        readIntFromFile(inputFd, &block[i % blockSize]);
        if ((i != 0 && (i + 1) % blockSize == 0) || i == numbersCount - 1 || blockSize == 1) {
            char tempFileName[MAX_FILENAME_LEN];
            const int fileIndex = ceilDiv((i + 1), blockSize) - 1;
            fileNames[fileIndex] = malloc(MAX_FILENAME_LEN * sizeof(char));
            if (fileNames[fileIndex] == NULL) {
                for (int j = 0; j < fileIndex; j++) {
                    free(fileNames[j]);
                }
                cioClose(inputFd);
                free(block);
                free(fileNames);
                return (RunResult){"Can't allocate memory.", errno};
            }
            snprintf(tempFileName, sizeof(tempFileName), B1_TEMP_FILENAME_FORMAT, fileNamePrefix, fileIndex);
            memcpy(fileNames[fileIndex], tempFileName, sizeof(tempFileName));
            int tempFd = cioOpen(tempFileName);
            if (tempFd == -1) {
                for (int j = 0; j < fileIndex + 1; j++) {
                    free(fileNames[j]);
                }
                cioClose(inputFd);
                free(block);
                free(fileNames);
                return (RunResult){"Can't open temp file.", errno};
            }
            const int nel = getNel(i, blockSize, numbersCount);
            qsort(block, nel, sizeof(int), compare);
            for (int j = 0; j < nel; j++) {
                char buffer[MAX_CHARACTERS_FOR_INT];
                snprintf(buffer, MAX_CHARACTERS_FOR_INT * sizeof(char), "%d ", block[j]);
                size_t length = strlen(buffer);
                cioWrite(tempFd, buffer, length);
            }
            cioClose(tempFd);
        }
    }
    free(block);
    cioClose(inputFd);

    FileState fileStates[filesCount];
    ErrorHandler handler = {SUCCESS_CODE};

    for (int i = 0; i < filesCount; i++) {
        initializeFileState(&fileStates[i], fileNames[i], &handler);
        if (handler.statusCode != SUCCESS_CODE) {
            for (int j = 0; j < i; j++) {
                closeFileState(&fileStates[j]);
                remove(fileNames[j]);
                free(fileNames[j]);
            }
            free(fileNames);
            return (RunResult){"Can't initialize File States.", handler.statusCode};
        }
    }

    PriorityQueue *const pq = newPriorityQueue(filesCount, &handler);
    if (handler.statusCode != SUCCESS_CODE) {
        for (int i = 0; i < filesCount; i++) {
            closeFileState(&fileStates[i]);
            remove(fileNames[i]);
            free(fileNames[i]);
        }
        free(fileNames);
        return (RunResult){"Can't create Priority Queue.", handler.statusCode};
    }

    for (int i = 0; i < filesCount; i++) {
        if (!fileStates[i].isEndOfFile) {
            offer(pq, fileStates[i], &handler);
            if (handler.statusCode != SUCCESS_CODE) {
                for (int j = 0; j < filesCount; j++) {
                    closeFileState(&fileStates[j]);
                    remove(fileNames[j]);
                    free(fileNames[j]);
                }
                free(fileNames);
                freePriorityQueue(pq);
                return (RunResult){"Can't offer element to Priority Queue.", handler.statusCode};
            }
        }
    }

    char outputFileName[MAX_FILENAME_LEN];
    snprintf(outputFileName, sizeof(outputFileName), B1_OUTPUT_FILENAME_FORMAT, fileNamePrefix);
    int outputFd = cioOpen(outputFileName);
    if (outputFd == -1) {
        for (int i = 0; i < filesCount; i++) {
            closeFileState(&fileStates[i]);
            remove(fileNames[i]);
            free(fileNames[i]);
        }
        free(fileNames);
        freePriorityQueue(pq);
        return (RunResult){"Can't open output file.", errno};
    }
    char numbersCountBuffer[MAX_CHARACTERS_FOR_INT];
    snprintf(numbersCountBuffer, MAX_CHARACTERS_FOR_INT * sizeof(char), "%d\n", numbersCount);
    size_t numbersCountLength = strlen(numbersCountBuffer);
    cioWrite(outputFd, numbersCountBuffer, numbersCountLength);

    while (!isEmpty(pq)) {
        FileState fileState = poll(pq, &handler);
        if (handler.statusCode != SUCCESS_CODE) {
            for (int i = 0; i < filesCount; i++) {
                closeFileState(&fileStates[i]);
                remove(fileNames[i]);
                free(fileNames[i]);
            }
            free(fileNames);
            freePriorityQueue(pq);
            cioClose(outputFd);
            return (RunResult){"Can't poll element from Priority Queue.", handler.statusCode};
        }

        char buffer[MAX_CHARACTERS_FOR_INT];
        snprintf(buffer, MAX_CHARACTERS_FOR_INT * sizeof(char), "%d ", fileState.value);
        size_t length = strlen(buffer);
        cioWrite(outputFd, buffer, length);

        updateFileState(&fileState);
        if (!fileState.isEndOfFile) {
            offer(pq, fileState, &handler);
            if (handler.statusCode != SUCCESS_CODE) {
                for (int i = 0; i < filesCount; i++) {
                    closeFileState(&fileStates[i]);
                    remove(fileNames[i]);
                    free(fileNames[i]);
                }
                free(fileNames);
                freePriorityQueue(pq);
                cioClose(outputFd);
                return (RunResult){"Can't offer element to Priority Queue.", handler.statusCode};
            }
        }
    }

    for (int i = 0; i < filesCount; i++) {
        closeFileState(&fileStates[i]);
        remove(fileNames[i]);
        free(fileNames[i]);
    }
    free(fileNames);
    freePriorityQueue(pq);
    cioClose(outputFd);

    return (RunResult){"Success!", SUCCESS_CODE};
}
