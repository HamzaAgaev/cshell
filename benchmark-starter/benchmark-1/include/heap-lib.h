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

PriorityQueue *newPriorityQueue(int capacity, ErrorHandler *handler);

void freePriorityQueue(PriorityQueue *pq);

bool isEmpty(PriorityQueue *pq);

void offer(PriorityQueue *pq, FileState value, ErrorHandler *handler);

FileState peek(PriorityQueue *pq, ErrorHandler *handler);

FileState poll(PriorityQueue *pq, ErrorHandler *handler);

#endif// OS_LAB_1_HEAP_LIB_H
