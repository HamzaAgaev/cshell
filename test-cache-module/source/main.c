#include "lab2.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define TEST_FILE_NAME "test.txt"
#define FD_COUNT 10

static pthread_mutex_t commonMutex;

typedef struct {
    int threadId;
    int fd;
} ThreadArg;

void testOneFileEditedByManyFd() {
    initializeLRUCache();
    int fd = lab2Open(TEST_FILE_NAME);
    int fdArray[FD_COUNT];
    char expectedText[FD_COUNT + 1];
    for (char i = 0; i < FD_COUNT; i++) {
        char symbol = 'a' + i;
        fdArray[i] = lab2Open(TEST_FILE_NAME);
        lab2Lseek(fdArray[i], i, LAB2_SEEK_SET);
        lab2Write(fdArray[i], &symbol, 1);
        expectedText[i] = symbol;
    }
    expectedText[FD_COUNT] = '\0';
    char writtenText[FD_COUNT + 1];
    lab2Lseek(fd, 0, LAB2_SEEK_SET);
    lab2Read(fd, writtenText, FD_COUNT);
    writtenText[FD_COUNT] = '\0';
    for (char i = 0; i < FD_COUNT; i++) {
        lab2Close(fdArray[i]);
    }
    lab2Close(fd);
    assert(strcmp(expectedText, writtenText) == 0);
    destroyLRUCache();
    remove(TEST_FILE_NAME);
}

void *threadFunc(void *arg) {
    ThreadArg arg_ = *(ThreadArg *)arg;
    char symbol = 'a' + arg_.threadId;
    pthread_mutex_lock(&commonMutex);
    lab2Lseek(arg_.fd, arg_.threadId, LAB2_SEEK_SET);
    assert(lab2Write(arg_.fd, &symbol, 1) != -1);
    pthread_mutex_unlock(&commonMutex);
    return NULL;
}


void testMultiThreadedFileWrite() {
    initializeLRUCache();
    int fd = lab2Open(TEST_FILE_NAME);
    pthread_t threads[FD_COUNT];
    ThreadArg threadArgs[FD_COUNT];
    pthread_mutex_init(&commonMutex, NULL);
    for (int i = 0; i < FD_COUNT; i++) {
        threadArgs[i] = (ThreadArg){.fd = fd, .threadId = i};
        pthread_create(&threads[i], NULL, threadFunc, &threadArgs[i]);
    }
    for (int i = 0; i < FD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&commonMutex);
    char expectedText[FD_COUNT + 1];
    for (int i = 0; i < FD_COUNT; i++) {
        expectedText[i] = 'a' + i;
    }
    expectedText[FD_COUNT] = '\0';
    char writtenText[FD_COUNT + 1];
    lab2Lseek(fd, 0, LAB2_SEEK_SET);
    lab2Read(fd, writtenText, FD_COUNT);
    writtenText[FD_COUNT] = '\0';
    lab2Close(fd);
    assert(strcmp(expectedText, writtenText) == 0);
    destroyLRUCache();
    remove(TEST_FILE_NAME);
}

int main() {
    testOneFileEditedByManyFd();
    testMultiThreadedFileWrite();
    printf("Tests passed successfully!");
    return 0;
}