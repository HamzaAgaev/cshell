#include "benchmark-1.h"

#include "file-state.h"
#include "heap-lib.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    FILE *inputFile;
    FILE *outputFile;
    inputFile = fopen(INPUT_FILENAME, "r");
    if (inputFile == NULL) {
        return (RunResult){"Can't open input file.", errno};
    }

    int numbersCount;
    fscanf(inputFile, "%d", &numbersCount);

    const int blockSize = (numbersCount / BLOCK_SCALE != 0) ? (numbersCount / BLOCK_SCALE) : numbersCount;
    int *const block = (int *)malloc(blockSize * sizeof(int));
    if (block == NULL) {
        fclose(inputFile);
        return (RunResult){"Can't allocate memory.", errno};
    }

    const int filesCount = ceilDiv(numbersCount, blockSize);
    char **fileNames = malloc(filesCount * sizeof(char *));//[filesCount][MAX_FILENAME_LEN]
    if (fileNames == NULL) {
        fclose(inputFile);
        free(block);
        return (RunResult){"Can't allocate memory.", errno};
    }
    char fileNamePrefix[GEN_STR_LEN + 1];
    generateRandomString(fileNamePrefix, GEN_STR_LEN + 1, DEFAULT_SEED);

    for (int i = 0; i < numbersCount; i++) {
        fscanf(inputFile, "%d", &block[i % blockSize]);
        if ((i != 0 && (i + 1) % blockSize == 0) || i == numbersCount - 1) {
            char tempFileName[MAX_FILENAME_LEN];
            const int fileIndex = ceilDiv((i + 1), blockSize) - 1;
            fileNames[fileIndex] = malloc(MAX_FILENAME_LEN * sizeof(char));
            if (fileNames[fileIndex] == NULL) {
                for (int j = 0; j < fileIndex; j++) {
                    free(fileNames[j]);
                }
                fclose(inputFile);
                free(block);
                free(fileNames);
                return (RunResult){"Can't allocate memory.", errno};
            }
            snprintf(tempFileName, sizeof(tempFileName), TEMP_FILENAME_FORMAT, fileNamePrefix, fileIndex);
            memcpy(fileNames[fileIndex], tempFileName, sizeof(tempFileName));
            FILE *tempFile = fopen(tempFileName, "w");
            if (tempFile == NULL) {
                for (int j = 0; j < fileIndex + 1; j++) {
                    free(fileNames[j]);
                }
                fclose(inputFile);
                free(block);
                free(fileNames);
                return (RunResult){"Can't open temp file.", errno};
            }
            const int nel = getNel(i, blockSize, numbersCount);
            qsort(block, nel, sizeof(int), compare);
            for (int j = 0; j < nel; j++) {
                fprintf(tempFile, "%d ", block[j]);
            }
            fclose(tempFile);
        }
    }
    free(block);
    fclose(inputFile);

    FileState fileStates[filesCount];
    ErrorCatcher catcher = {SUCCESS_CODE};

    for (int i = 0; i < filesCount; i++) {
        initializeFileState(&fileStates[i], fileNames[i], &catcher);
        if (catcher.statusCode != SUCCESS_CODE) {
            for (int j = 0; j < i; j++) {
                closeFileState(&fileStates[j]);
                remove(fileNames[j]);
                free(fileNames[j]);
            }
            free(fileNames);
            return (RunResult){"Can't initialize File States.", catcher.statusCode};
        }
    }

    PriorityQueue *const pq = newPriorityQueue(filesCount, &catcher);
    if (catcher.statusCode != SUCCESS_CODE) {
        for (int i = 0; i < filesCount; i++) {
            closeFileState(&fileStates[i]);
            remove(fileNames[i]);
            free(fileNames[i]);
        }
        free(fileNames);
        return (RunResult){"Can't create Priority Queue.", catcher.statusCode};
    }

    for (int i = 0; i < filesCount; i++) {
        if (!fileStates[i].isEndOfFile) {
            offer(pq, fileStates[i], &catcher);
            if (catcher.statusCode != SUCCESS_CODE) {
                for (int j = 0; j < filesCount; j++) {
                    closeFileState(&fileStates[j]);
                    remove(fileNames[j]);
                    free(fileNames[j]);
                }
                free(fileNames);
                freePriorityQueue(pq);
                return (RunResult){"Can't offer element to Priority Queue.", catcher.statusCode};
            }
        }
    }

    char outputFileName[MAX_FILENAME_LEN];
    snprintf(outputFileName, sizeof(outputFileName), OUTPUT_FILENAME_FORMAT, fileNamePrefix);
    outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL) {
        for (int i = 0; i < filesCount; i++) {
            closeFileState(&fileStates[i]);
            remove(fileNames[i]);
            free(fileNames[i]);
        }
        free(fileNames);
        freePriorityQueue(pq);
        return (RunResult){"Can't open output file.", errno};
    }
    fprintf(outputFile, "%d\n", numbersCount);

    while (!isEmpty(pq)) {
        FileState fileState = poll(pq, &catcher);
        if (catcher.statusCode != SUCCESS_CODE) {
            for (int i = 0; i < filesCount; i++) {
                closeFileState(&fileStates[i]);
                remove(fileNames[i]);
                free(fileNames[i]);
            }
            free(fileNames);
            freePriorityQueue(pq);
            fclose(outputFile);
            return (RunResult){"Can't poll element from Priority Queue.", catcher.statusCode};
        }
        fprintf(outputFile, "%d ", fileState.value);
        updateFileState(&fileState);
        if (!fileState.isEndOfFile) {
            offer(pq, fileState, &catcher);
            if (catcher.statusCode != SUCCESS_CODE) {
                for (int i = 0; i < filesCount; i++) {
                    closeFileState(&fileStates[i]);
                    remove(fileNames[i]);
                    free(fileNames[i]);
                }
                free(fileNames);
                freePriorityQueue(pq);
                fclose(outputFile);
                return (RunResult){"Can't offer element to Priority Queue.", catcher.statusCode};
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
    fclose(outputFile);

    return (RunResult){"Success!", SUCCESS_CODE};
}
