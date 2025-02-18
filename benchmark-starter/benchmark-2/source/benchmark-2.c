#include "benchmark-2.h"

#include "benchmark-utils.h"
#include "lab2.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B2_OUTPUT_FILENAME_FORMAT "b2-" OUTPUT_FILENAME_FORMAT

RunResult benchmark2() {
    int inputFd = lab2Open(INPUT_FILENAME);
    if (inputFd == -1) {
        return (RunResult){"Can't open input file.", errno};
    }

    int numbersCount;
    readIntFromFile(inputFd, &numbersCount);

    int *inputNumbers = (int *)malloc(numbersCount * sizeof(int));
    if (inputNumbers == NULL) {
        lab2Close(inputFd);
        return (RunResult){"Can't allocate memory for numbers.", errno};
    }
    for (int i = 0; i < numbersCount; i++) {
        readIntFromFile(inputFd, &inputNumbers[i]);
    }
    qsort(inputNumbers, numbersCount, sizeof(int), compare);

    char fileNamePrefix[GEN_STR_LEN + 1];
    generateRandomString(fileNamePrefix, GEN_STR_LEN + 1, DEFAULT_SEED);

    char outputFileName[MAX_FILENAME_LEN];
    snprintf(outputFileName, sizeof(outputFileName), B2_OUTPUT_FILENAME_FORMAT, fileNamePrefix);
    int outputFd = lab2Open(outputFileName);
    if (outputFd == -1) {
        free(inputNumbers);
        lab2Close(inputFd);
        return (RunResult){"Can't open output file.", errno};
    }
    char numbersCountBuffer[MAX_CHARACTERS_FOR_INT];
    snprintf(numbersCountBuffer, MAX_CHARACTERS_FOR_INT * sizeof(char), "%d\n", numbersCount);
    size_t numbersCountLength = strlen(numbersCountBuffer);
    lab2Write(outputFd, numbersCountBuffer, numbersCountLength);

    for (int i = 0; i < numbersCount; i++) {
        if (i == 0 || inputNumbers[i] != inputNumbers[i - 1]) {
            char buffer[MAX_CHARACTERS_FOR_INT];
            snprintf(buffer, MAX_CHARACTERS_FOR_INT * sizeof(char), "%d ", inputNumbers[i]);
            size_t length = strlen(buffer);
            lab2Write(outputFd, buffer, length);
        }
    }
    free(inputNumbers);
    lab2Close(inputFd);
    lab2Close(outputFd);
    return (RunResult){"Success!", SUCCESS_CODE};
}
