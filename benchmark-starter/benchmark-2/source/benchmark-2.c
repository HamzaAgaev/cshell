#include "benchmark-2.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

RunResult benchmark2() {
    FILE *inputFile;
    FILE *outputFile;
    inputFile = fopen(INPUT_FILENAME, "r");
    if (inputFile == NULL) {
        return (RunResult){"Can't open input file.", errno};
    }

    int numbersCount;
    fscanf(inputFile, "%d", &numbersCount);

    int *inputNumbers = (int *)malloc(numbersCount * sizeof(int));
    if (inputNumbers == NULL) {
        fclose(inputFile);
        return (RunResult){"Can't allocate memory for numbers.", errno};
    }
    for (int i = 0; i < numbersCount; i++) {
        fscanf(inputFile, "%d", &inputNumbers[i]);
    }
    qsort(inputNumbers, numbersCount, sizeof(int), compare);

    char fileNamePrefix[GEN_STR_LEN + 1];
    generateRandomString(fileNamePrefix, GEN_STR_LEN + 1, DEFAULT_SEED);

    char outputFileName[MAX_FILENAME_LEN];
    snprintf(outputFileName, sizeof(outputFileName), OUTPUT_FILENAME_FORMAT, fileNamePrefix);
    outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL) {
        free(inputNumbers);
        fclose(inputFile);
        return (RunResult){"Can't open output file.", errno};
    }
    fprintf(outputFile, "%d\n", numbersCount);

    for (int i = 0; i < numbersCount; i++) {
        if (i == 0 || inputNumbers[i] != inputNumbers[i - 1]) {
            fprintf(outputFile, "%d ", inputNumbers[i]);
        }
    }
    free(inputNumbers);
    fclose(inputFile);
    fclose(outputFile);
    return (RunResult){"Success!", SUCCESS_CODE};
}
