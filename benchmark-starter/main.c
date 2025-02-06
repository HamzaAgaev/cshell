#include "benchmark-1/include/benchmark-1.h"
#include "benchmark-2/include/benchmark-2.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MODE_NAME_LEN 10

static void *threadB1Function(void *arg) {
    RunResult *result = (RunResult *)malloc(sizeof(RunResult));
    *result = benchmark1();
    pthread_exit(result);
}

static void *threadB2Function(void *arg) {
    RunResult *result = (RunResult *)malloc(sizeof(RunResult));
    *result = benchmark2();
    pthread_exit(result);
}

static void startBenchmarksUsingThreads(int threadsB1Count, int threadsB2Count) {
    pthread_t *threadsB1 = malloc(threadsB1Count * sizeof(pthread_t));
    pthread_t *threadsB2 = malloc(threadsB2Count * sizeof(pthread_t));
    int *threadB1Ids = malloc(threadsB1Count * sizeof(int));
    int *threadB2Ids = malloc(threadsB2Count * sizeof(int));

    for (int i = 0; i < threadsB1Count; i++) {
        threadB1Ids[i] = i;
        pthread_create(&threadsB1[i], NULL, threadB1Function, &threadB1Ids[i]);
    }
    for (int i = 0; i < threadsB2Count; i++) {
        threadB2Ids[i] = i;
        pthread_create(&threadsB2[i], NULL, threadB2Function, &threadB2Ids[i]);
    }

    RunResult **resultsB1 = malloc(threadsB1Count * sizeof(RunResult *));
    RunResult **resultsB2 = malloc(threadsB2Count * sizeof(RunResult *));

    for (int i = 0; i < threadsB1Count; i++) {
        pthread_join(threadsB1[i], (void **)&resultsB1[i]);
        printf(
                "B1Thread[%d]: %s, statusCode = %s.\n", i + 1, resultsB1[i]->message, strerror(resultsB1[i]->statusCode));
        free(resultsB1[i]);
    }
    for (int i = 0; i < threadsB2Count; i++) {
        pthread_join(threadsB2[i], (void **)&resultsB2[i]);
        printf(
                "B2Thread[%d]: %s, statusCode = %s.\n", i + 1, resultsB2[i]->message, strerror(resultsB2[i]->statusCode));
        free(resultsB2[i]);
    }
    free(resultsB1);
    free(resultsB2);

    free(threadsB1);
    free(threadsB2);
    free(threadB1Ids);
    free(threadB2Ids);
}

static void startBenchmark(RunResult (*benchmark)(void), int i, char *bName) {
    RunResult result = benchmark();
    printf("%sProcess[%d]: %s, statusCode = %s.\n", bName, i + 1, result.message, strerror(result.statusCode));
    int statusCode = result.statusCode;
    exit(statusCode);
}

static void startBenchmarksUsingProcesses(int processesB1Count, int processesB2Count) {
    for (int i = 0; i < processesB1Count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            startBenchmark(benchmark1, i, "B1");
        }
    }
    for (int i = 0; i < processesB2Count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            startBenchmark(benchmark2, i, "B2");
        }
    }
    int status;
    while (wait(&status) > 0) {
        if (WIFEXITED(status)) {
            printf("Process exited with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Process killed by signal: %d\n", WTERMSIG(status));
        }
    }
}

int main(int argc, char **argv) {
    char mode[MODE_NAME_LEN];
    int entitiesB1Count, entitiesB2Count;
    if (argc != 4) {
        fprintf(stdout, "Enter mode: ");
        fscanf(stdin, "%s", mode);
        fprintf(stdout, "Enter count of entities for benchmark-1: ");
        fscanf(stdin, "%d", &entitiesB1Count);
        fprintf(stdout, "Enter count of entities for benchmark-2: ");
        fscanf(stdin, "%d", &entitiesB2Count);
    } else {
        memcpy(mode, argv[1], MODE_NAME_LEN);
        entitiesB1Count = atoi(argv[2]);
        entitiesB2Count = atoi(argv[3]);
    }
    if (strcmp(mode, "threads") == 0) {
        startBenchmarksUsingThreads(entitiesB1Count, entitiesB2Count);
    } else if (strcmp(mode, "processes") == 0) {
        startBenchmarksUsingProcesses(entitiesB1Count, entitiesB2Count);
    }
}
