#ifndef OS_LAB_1_HEAP_LIB_H
#define OS_LAB_1_HEAP_LIB_H

#include "file-state.h"

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    FileState *data;
    int size;
    int capacity;
} PriorityQueue;

PriorityQueue *newPriorityQueue(int capacity, ErrorCatcher *catcher);

void freePriorityQueue(PriorityQueue *pq);

bool isEmpty(PriorityQueue *pq);

void offer(PriorityQueue *pq, FileState value, ErrorCatcher *catcher);

FileState peek(PriorityQueue *pq, ErrorCatcher *catcher);

FileState poll(PriorityQueue *pq, ErrorCatcher *catcher);

#endif// OS_LAB_1_HEAP_LIB_H
