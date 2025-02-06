#include "heap-lib.h"

#include <errno.h>
#include <stdlib.h>

#define SWAP(a, b, type) \
    type temp = *(a);    \
    *(a) = *(b);         \
    *(b) = temp;

int parentIndex(int i) {
    return (i - 1) / 2;
}

int leftIndex(int i) {
    return 2 * i + 1;
}

int rightIndex(int i) {
    return 2 * i + 2;
}

static void resize(PriorityQueue *pq, ErrorCatcher *catcher) {
    pq->capacity *= 2;
    pq->data = (FileState *)realloc(pq->data, pq->capacity * sizeof(FileState));
    if (pq->data == NULL) {
        catcher->statusCode = errno;
    }
}

PriorityQueue *newPriorityQueue(int capacity, ErrorCatcher *catcher) {
    PriorityQueue *pq = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    if (pq == NULL) {
        catcher->statusCode = errno;
        return pq;
    }
    pq->data = (FileState *)malloc(capacity * sizeof(FileState));
    if (pq->data == NULL) {
        catcher->statusCode = errno;
        return pq;
    }
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

void freePriorityQueue(PriorityQueue *pq) {
    free(pq->data);
    free(pq);
}

bool isEmpty(PriorityQueue *pq) {
    return pq->size == 0;
}

void offer(PriorityQueue *pq, FileState value, ErrorCatcher *catcher) {
    if (pq->size == pq->capacity) {
        resize(pq, catcher);
    }
    if (catcher->statusCode != SUCCESS_CODE) {
        return;
    }
    pq->data[pq->size] = value;
    int i = pq->size;
    pq->size++;

    // heapify
    while (i != 0 && pq->data[parentIndex(i)].value > pq->data[i].value) {
        SWAP(&pq->data[parentIndex(i)], &pq->data[i], FileState);
        i = parentIndex(i);
    }
}

FileState peek(PriorityQueue *pq, ErrorCatcher *catcher) {
    if (isEmpty(pq)) {
        catcher->statusCode = DEFAULT_ERROR_CODE;
        return (FileState){NULL, 0, true};
    }
    return pq->data[0];
}

FileState poll(PriorityQueue *pq, ErrorCatcher *catcher) {
    if (isEmpty(pq)) {
        catcher->statusCode = DEFAULT_ERROR_CODE;
        return (FileState){NULL, 0, true};
    }
    FileState root = pq->data[0];
    pq->data[0] = pq->data[pq->size - 1];
    pq->size--;

    // heapify
    int i = 0;
    for (;;) {
        const int left = leftIndex(i);
        const int right = rightIndex(i);
        int smallest = i;

        if (left < pq->size && pq->data[left].value < pq->data[smallest].value) {
            smallest = left;
        }
        if (right < pq->size && pq->data[right].value < pq->data[smallest].value) {
            smallest = right;
        }
        if (smallest == i) {
            break;
        }
        SWAP(&pq->data[i], &pq->data[smallest], FileState);
        i = smallest;
    }

    return root;
}
