#include "cache.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

const size_t CACHE_BLOCK_COUNT = 64;
const size_t CACHE_BLOCK_SIZE_IN_BYTES = 1024 * 1024;

static size_t sizeHash(ssize_t value) {
    const size_t K = 2654435769;
    return (size_t)((value * K) & (CACHE_BLOCK_COUNT - 1));
}

static size_t sizeHashByFdAndOffset(int fd, off_t offset) {
    ssize_t value = fd ^ offset;
    return sizeHash(value);
}

LRUCache *createLRUCache(ErrorHandler *handler) {
    LRUCache *lruCache = (LRUCache *)malloc(sizeof(LRUCache));
    if (lruCache == NULL) {
        handler->statusCode = errno;
        return lruCache;
    }
    lruCache->listHead = NULL;
    lruCache->listTail = NULL;
    lruCache->size = 0;
    lruCache->hashTable = (HashListNode **)malloc(sizeof(HashListNode *) * CACHE_BLOCK_COUNT);
    if (lruCache->hashTable == NULL) {
        handler->statusCode = errno;
        return lruCache;
    }
    for (int i = 0; i < CACHE_BLOCK_COUNT; i++) {
        lruCache->hashTable[i] = NULL;
    }
    return lruCache;
}

static FileBlock *createFileBlock(int fd, off_t offset, ErrorHandler *handler) {
    FileBlock *fileBlock = (FileBlock *)malloc(sizeof(FileBlock));
    if (fileBlock == NULL) {
        handler->statusCode = errno;
        return fileBlock;
    }
    fileBlock->fd = fd;
    fileBlock->offset = offset;
    fileBlock->data = (unsigned char *)calloc(CACHE_BLOCK_SIZE_IN_BYTES, sizeof(unsigned char));
    if (fileBlock->data == NULL) {
        handler->statusCode = errno;
        return fileBlock;
    }
    fileBlock->size = 0;
    fileBlock->status = NEW;
    return fileBlock;
}

static void freeFileBlock(FileBlock *fileBlock) {
    free(fileBlock->data);
    free(fileBlock);
}

static DataListNode *createDataListNode(FileBlock *fileBlock, DataListNode *prev,
                                        DataListNode *next, ErrorHandler *handler) {
    DataListNode *listNode = (DataListNode *)malloc(sizeof(DataListNode));
    if (listNode == NULL) {
        handler->statusCode = errno;
        return listNode;
    }
    listNode->fileBlock = fileBlock;
    listNode->prev = prev;
    listNode->next = next;
    return listNode;
}

static void freeDataListNode(DataListNode *listNode) {
    free(listNode);
}

static HashListNode *createHashListNode(DataListNode *listNode, HashListNode *prev,
                                        HashListNode *next, ErrorHandler *handler) {
    HashListNode *hashListNode = (HashListNode *)malloc(sizeof(HashListNode));
    if (hashListNode == NULL) {
        handler->statusCode = errno;
        return hashListNode;
    }
    hashListNode->listNode = listNode;
    hashListNode->prev = prev;
    hashListNode->next = next;
    return hashListNode;
}

static void freeHashListNode(HashListNode *hashListNode) {
    free(hashListNode);
}

static void freeCacheBlockByHashListNode(HashListNode *hashListNode) {
    freeFileBlock(hashListNode->listNode->fileBlock);
    freeDataListNode(hashListNode->listNode);
    freeHashListNode(hashListNode);
}

void freeLRUCache(LRUCache *lruCache) {
    for (int i = 0; i < CACHE_BLOCK_COUNT; i++) {
        HashListNode *hashListNode = lruCache->hashTable[i];
        if (hashListNode == NULL) {
            continue;
        }
        while (hashListNode != NULL) {
            HashListNode *next = hashListNode->next;
            freeCacheBlockByHashListNode(hashListNode);
            hashListNode = next;
        }
    }
    free(lruCache->hashTable);
    free(lruCache);
}

off_t getAlignedOffset(off_t offset) {
    return (off_t)((offset / CACHE_BLOCK_SIZE_IN_BYTES) * CACHE_BLOCK_SIZE_IN_BYTES);
}

static HashListNode *getHashListNode(LRUCache *lruCache, int fd, off_t offset) {
    size_t hashIndex = sizeHashByFdAndOffset(fd, offset);
    HashListNode *hashListNode = lruCache->hashTable[hashIndex];
    while (hashListNode != NULL) {
        if (hashListNode->listNode->fileBlock->fd == fd &&
            hashListNode->listNode->fileBlock->offset == offset) {
            break;
        }
        hashListNode = hashListNode->next;
    }
    return hashListNode;
}

static void moveDataListNodeToHead(LRUCache *lruCache, DataListNode *listNode) {
    DataListNode *prev = listNode->prev;
    DataListNode *next = listNode->next;
    DataListNode *head = lruCache->listHead;
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

static DataListNode *findDataListNode(LRUCache *lruCache, int fd, off_t offset) {
    HashListNode *hashListNode = getHashListNode(lruCache, fd, offset);
    if (hashListNode == NULL) {
        return NULL;
    }
    moveDataListNodeToHead(lruCache, hashListNode->listNode);
    return hashListNode->listNode;
}

static void removeDataListNode(LRUCache *lruCache, DataListNode *listNode) {
    DataListNode *prev = listNode->prev;
    DataListNode *next = listNode->next;
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

static void removeHashListNode(LRUCache *lruCache, HashListNode *hashListNode) {
    HashListNode *prev = hashListNode->prev;
    HashListNode *next = hashListNode->next;
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    int fd = hashListNode->listNode->fileBlock->fd;
    off_t offset = hashListNode->listNode->fileBlock->offset;
    size_t hashIndex = sizeHashByFdAndOffset(fd, offset);
    if (lruCache->hashTable[hashIndex] == hashListNode) {
        lruCache->hashTable[hashIndex] = hashListNode->next;
    }
    hashListNode->prev = NULL;
    hashListNode->next = NULL;
}

void deleteCacheBlockByDataListNode(LRUCache *lruCache, DataListNode *listNode, ErrorHandler *handler) {
    FileBlock *fileBlock = listNode->fileBlock;
    if (fileBlock->status != SYNCED) {
        syncFileBlock(fileBlock, handler);
    }
    if (handler->statusCode != SUCCESS_CODE) {
        return;
    }
    HashListNode *hashListNodeToFree = getHashListNode(
            lruCache, listNode->fileBlock->fd, listNode->fileBlock->offset);
    removeDataListNode(lruCache, listNode);
    removeHashListNode(lruCache, hashListNodeToFree);
    freeCacheBlockByHashListNode(hashListNodeToFree);
    lruCache->size--;
}

size_t syncFileBlock(FileBlock *fileBlock, ErrorHandler *handler) {
    size_t bytesSynced = 0;
    off_t offset = fileBlock->offset;
    size_t size = fileBlock->size;
    unsigned char *data = fileBlock->data;
    if (lseek(fileBlock->fd, offset, SEEK_SET) == -1) {
        handler->statusCode = errno;
        return bytesSynced;
    }
    bytesSynced = pwrite(fileBlock->fd, data, size, offset);
    if (bytesSynced == -1) {
        handler->statusCode = errno;
        return bytesSynced;
    }
    fileBlock->status = SYNCED;
    return bytesSynced;
}

static DataListNode *addDataListNode(LRUCache *lruCache, int fd, off_t offset, ErrorHandler *handler) {
    size_t hashIndex = sizeHashByFdAndOffset(fd, offset);
    HashListNode *hashListNode = lruCache->hashTable[hashIndex];
    bool isEmptyNode = false;
    if (hashListNode == NULL) {
        isEmptyNode = true;
    }
    while (!isEmptyNode && hashListNode->next != NULL) {
        hashListNode = hashListNode->next;
    }
    DataListNode *listNode = NULL;
    FileBlock *fileBlock = createFileBlock(fd, offset, handler);
    if (handler->statusCode != SUCCESS_CODE) {
        return listNode;
    }
    listNode = createDataListNode(fileBlock, NULL, NULL, handler);
    if (handler->statusCode != SUCCESS_CODE) {
        return listNode;
    }
    HashListNode *newHashListNode = createHashListNode(listNode, hashListNode, NULL, handler);
    if (handler->statusCode != SUCCESS_CODE) {
        return listNode;
    }
    if (!isEmptyNode) {
        hashListNode->next = newHashListNode;
    } else {
        lruCache->hashTable[hashIndex] = newHashListNode;
    }
    // вытеснение
    if (lruCache->size == CACHE_BLOCK_COUNT) {
        DataListNode *listTail = lruCache->listTail;
        deleteCacheBlockByDataListNode(lruCache, listTail, handler);
    }
    moveDataListNodeToHead(lruCache, listNode);
    lruCache->size++;
    if (lruCache->listTail == NULL) {
        lruCache->listTail = listNode;
    }
    return listNode;
}

FileBlock *getFileBlock(LRUCache *lruCache, int fd, off_t offset, ErrorHandler *handler) {
    FileBlock *fileBlock = NULL;
    DataListNode *listNode = findDataListNode(lruCache, fd, offset);
    if (listNode == NULL) {
        listNode = addDataListNode(lruCache, fd, offset, handler);
    }
    if (handler->statusCode != SUCCESS_CODE) {
        return fileBlock;
    }
    fileBlock = listNode->fileBlock;
    return fileBlock;
}
