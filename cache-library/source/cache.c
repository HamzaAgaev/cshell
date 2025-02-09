#include "cache.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

static size_t hashFunction(ssize_t value) {
    const size_t K = 2654435769;
    const size_t L = 51;
    return (size_t)((value + L) * K) & (CACHE_BLOCK_COUNT - 1);
}

static size_t hashFunctionByFdAndOffset(int fd, off_t offset) {
    const ssize_t A = 37;
    const ssize_t B = 43;
    ssize_t value = (fd * A) * (offset + B);
    return hashFunction(value);
}

LRUCache *createLRUCache(ErrorCatcher *catcher) {
    LRUCache *lruCache = (LRUCache *)malloc(sizeof(LRUCache));
    if (lruCache == NULL) {
        catcher->statusCode = errno;
        return lruCache;
    }
    lruCache->listHead = NULL;
    lruCache->listTail = NULL;
    lruCache->size = 0;
    lruCache->hashTable = (struct HashListNode **)malloc(sizeof(struct HashListNode *) * CACHE_BLOCK_COUNT);
    if (lruCache->hashTable == NULL) {
        catcher->statusCode = errno;
        return lruCache;
    }
    for (int i = 0; i < CACHE_BLOCK_COUNT; i++) {
        lruCache->hashTable[i] = NULL;
    }
    return lruCache;
}

static FileBlock *createFileBlock(int fd, off_t offset, ErrorCatcher *catcher) {
    FileBlock *fileBlock = (FileBlock *)malloc(sizeof(FileBlock));
    if (fileBlock == NULL) {
        catcher->statusCode = errno;
        return fileBlock;
    }
    fileBlock->fd = fd;
    fileBlock->offset = offset;
    fileBlock->data = (unsigned char *)calloc(CACHE_BLOCK_SIZE, sizeof(unsigned char));
    if (fileBlock->data == NULL) {
        catcher->statusCode = errno;
        return fileBlock;
    }
    fileBlock->size = 0;
    fileBlock->syncStatus = NEW;
    return fileBlock;
}

static void freeFileBlock(FileBlock *fileBlock) {
    free(fileBlock->data);
    free(fileBlock);
}

static struct DataListNode *createDataListNode(FileBlock *fileBlock, struct DataListNode *prev,
                                               struct DataListNode *next, ErrorCatcher *catcher) {
    struct DataListNode *listNode = (struct DataListNode *)malloc(sizeof(struct DataListNode));
    if (listNode == NULL) {
        catcher->statusCode = errno;
        return listNode;
    }
    listNode->fileBlock = fileBlock;
    listNode->prev = prev;
    listNode->next = next;
    return listNode;
}

static void freeDataListNode(struct DataListNode *listNode) {
    free(listNode);
}

static struct HashListNode *createHashListNode(struct DataListNode *listNode, struct HashListNode *prev,
                                               struct HashListNode *next, ErrorCatcher *catcher) {
    struct HashListNode *hashListNode = (struct HashListNode *)malloc(sizeof(struct HashListNode));
    if (hashListNode == NULL) {
        catcher->statusCode = errno;
        return hashListNode;
    }
    hashListNode->listNode = listNode;
    hashListNode->prev = prev;
    hashListNode->next = next;
    return hashListNode;
}

static void freeHashListNode(struct HashListNode *hashListNode) {
    free(hashListNode);
}

static void freeCacheBlockByHashListNode(struct HashListNode *hashListNode) {
    freeFileBlock(hashListNode->listNode->fileBlock);
    freeDataListNode(hashListNode->listNode);
    freeHashListNode(hashListNode);
}

void freeLRUCache(LRUCache *lruCache) {
    for (int i = 0; i < CACHE_BLOCK_COUNT; i++) {
        struct HashListNode *hashListNode = lruCache->hashTable[i];
        if (hashListNode == NULL) {
            continue;
        }
        while (hashListNode != NULL) {
            struct HashListNode *next = hashListNode->next;
            freeCacheBlockByHashListNode(hashListNode);
            hashListNode = next;
        }
    }
    free(lruCache->hashTable);
    free(lruCache);
}

off_t getAlignedOffset(off_t offset) {
    return (offset / CACHE_BLOCK_SIZE) * CACHE_BLOCK_SIZE;
}

static struct HashListNode *getHashListNode(LRUCache *lruCache, int fd, off_t offset) {
    size_t hashIndex = hashFunctionByFdAndOffset(fd, offset);
    struct HashListNode *hashListNode = lruCache->hashTable[hashIndex];
    while (hashListNode != NULL) {
        if (hashListNode->listNode->fileBlock->fd == fd &&
            hashListNode->listNode->fileBlock->offset == offset) {
            break;
        }
        hashListNode = hashListNode->next;
    }
    return hashListNode;
}

static void moveDataListNodeToHead(LRUCache *lruCache, struct DataListNode *listNode) {
    struct DataListNode *prev = listNode->prev;
    struct DataListNode *next = listNode->next;
    struct DataListNode *head = lruCache->listHead;
    if (listNode != lruCache->listHead) {
        if (prev != NULL) {
            prev->next = next;
        }
        if (next != NULL) {
            next->prev = prev;
        }
        if (head != NULL) {
            head->prev = listNode;
        }
        lruCache->listHead = listNode;
        listNode->prev = NULL;
        listNode->next = head;
    }
}

static struct DataListNode *findDataListNode(LRUCache *lruCache, int fd, off_t offset) {
    struct HashListNode *hashListNode = getHashListNode(lruCache, fd, offset);
    if (hashListNode == NULL) {
        return NULL;
    }
    moveDataListNodeToHead(lruCache, hashListNode->listNode);
    return hashListNode->listNode;
}

static void removeDataListNode(LRUCache *lruCache, struct DataListNode *listNode) {
    struct DataListNode *prev = listNode->prev;
    struct DataListNode *next = listNode->next;
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    if (listNode == lruCache->listHead) {
        lruCache->listHead = next;
    }
    if (listNode == lruCache->listTail) {
        lruCache->listTail = prev;
    }
    listNode->prev = NULL;
    listNode->next = NULL;
}

static void removeHashListNode(LRUCache *lruCache, struct HashListNode *hashListNode) {
    struct HashListNode *prev = hashListNode->prev;
    struct HashListNode *next = hashListNode->next;
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    int fd = hashListNode->listNode->fileBlock->fd;
    off_t offset = hashListNode->listNode->fileBlock->offset;
    size_t hashIndex = hashFunctionByFdAndOffset(fd, offset);
    if (lruCache->hashTable[hashIndex] == hashListNode) {
        lruCache->hashTable[hashIndex] = hashListNode->next;
    }
    hashListNode->prev = NULL;
    hashListNode->next = NULL;
}

void deleteCacheBlockByDataListNode(LRUCache *lruCache, struct DataListNode *listNode, ErrorCatcher *catcher) {
    FileBlock *fileBlock = listNode->fileBlock;
    if (fileBlock->syncStatus != SYNCED) {
        syncFileBlock(fileBlock, catcher);
    }
    if (catcher->statusCode != SUCCESS_CODE) {
        return;
    }
    struct HashListNode *hashListNodeToFree = getHashListNode(
            lruCache, listNode->fileBlock->fd, listNode->fileBlock->offset);
    removeDataListNode(lruCache, listNode);
    removeHashListNode(lruCache, hashListNodeToFree);
    freeCacheBlockByHashListNode(hashListNodeToFree);
    lruCache->size--;
}

size_t syncFileBlock(FileBlock *fileBlock, ErrorCatcher *catcher) {
    size_t bytesSynced = 0;
    off_t initialOffset = lseek(fileBlock->fd, 0, SEEK_CUR);
    if (initialOffset == -1) {
        catcher->statusCode = errno;
        return bytesSynced;
    }
    off_t offset = fileBlock->offset;
    size_t size = fileBlock->size;
    unsigned char *data = fileBlock->data;
    if (lseek(fileBlock->fd, offset, SEEK_SET) == -1) {
        catcher->statusCode = errno;
        return bytesSynced;
    }
    bytesSynced = write(fileBlock->fd, data, size);
    if (bytesSynced == -1) {
        catcher->statusCode = errno;
        return bytesSynced;
    }
    fileBlock->syncStatus = SYNCED;
    if (lseek(fileBlock->fd, initialOffset, SEEK_SET) == -1) {
        catcher->statusCode = errno;
        return bytesSynced;
    }
    return bytesSynced;
}

static struct DataListNode *addDataListNode(LRUCache *lruCache, int fd, off_t offset, ErrorCatcher *catcher) {
    size_t hashIndex = hashFunctionByFdAndOffset(fd, offset);
    struct HashListNode *hashListNode = lruCache->hashTable[hashIndex];
    bool isEmptyNode = false;
    if (hashListNode == NULL) {
        isEmptyNode = true;
    }
    while (!isEmptyNode && hashListNode->next != NULL) {
        hashListNode = hashListNode->next;
    }
    struct DataListNode *listNode = NULL;
    FileBlock *fileBlock = createFileBlock(fd, offset, catcher);
    if (catcher->statusCode != SUCCESS_CODE) {
        return listNode;
    }
    listNode = createDataListNode(fileBlock, NULL, NULL, catcher);
    if (catcher->statusCode != SUCCESS_CODE) {
        return listNode;
    }
    struct HashListNode *newHashListNode = createHashListNode(listNode, hashListNode, NULL, catcher);
    if (catcher->statusCode != SUCCESS_CODE) {
        return listNode;
    }
    if (!isEmptyNode) {
        hashListNode->next = newHashListNode;
    } else {
        lruCache->hashTable[hashIndex] = newHashListNode;
    }
    // вытеснение
    if (lruCache->size == CACHE_BLOCK_COUNT) {
        struct DataListNode *listTail = lruCache->listTail;
        deleteCacheBlockByDataListNode(lruCache, listTail, catcher);
    }
    moveDataListNodeToHead(lruCache, listNode);
    lruCache->size++;
    if (lruCache->listTail == NULL) {
        lruCache->listTail = listNode;
    }
    return listNode;
}

FileBlock *getFileBlock(LRUCache *lruCache, int fd, off_t offset, ErrorCatcher *catcher) {
    FileBlock *fileBlock = NULL;
    struct DataListNode *listNode = findDataListNode(lruCache, fd, offset);
    if (listNode == NULL) {
        listNode = addDataListNode(lruCache, fd, offset, catcher);
    }
    if (catcher->statusCode != SUCCESS_CODE) {
        return fileBlock;
    }
    fileBlock = listNode->fileBlock;
    return fileBlock;
}
