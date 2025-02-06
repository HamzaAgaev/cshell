#include "utils.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

static bool isRandomInitialized = false;

static void initializeRandom(unsigned int seed) {
    if (!isRandomInitialized) {
        srand(seed);
        isRandomInitialized = true;
    }
}

void generateRandomString(char *str, int length, unsigned int seed) {
    const unsigned long seedByThreadAndProcess = (unsigned long)pthread_self() ^ seed ^ getpid();
    initializeRandom((unsigned int)seedByThreadAndProcess);
    const char start = 'a';
    const char end = 'z';
    const int range = end - start + 1;

    for (int i = 0; i < length - 1; i++) {
        str[i] = (char)(start + (char)(rand() % range));
    }

    str[length - 1] = '\0';
}

int compare(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}
