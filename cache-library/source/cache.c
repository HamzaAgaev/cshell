#include "cache.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

const size_t CACHE_BLOCK_COUNT = 64;
const size_t CACHE_BLOCK_SIZE_IN_BYTES = 1024 * 1024;

static size_t sizeHash(size_t value) {
    const size_t K = 2654435769;
    return (value * K) & (CACHE_BLOCK_COUNT - 1);
}

static size_t sizeHashByFileIdAndOffset(FileId fileId, off_t offset) {
    size_t value = fileId.inode ^ fileId.device ^ offset;
    return sizeHash(value);
}

FileId getFileIdByFd(int fd) {
    struct stat fileStat;
    fstat(fd, &fileStat);
    return (FileId){fd, fileStat.st_ino, fileStat.st_dev};
}

bool isSameFile(FileId fileId1, FileId fileId2) {
    return fileId1.inode == fileId2.inode && fileId1.device == fileId2.device;
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

static FileBlock *createFileBlock(FileId fileId, off_t offset, ErrorHandler *handler) {
    FileBlock *fileBlock = (FileBlock *)malloc(sizeof(FileBlock));
    if (fileBlock == NULL) {
        handler->statusCode = errno;
        return fileBlock;
    }
    fileBlock->fileId = fileId;
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

static HashListNode *getHashListNode(LRUCache *lruCache, FileId fileId, off_t offset) {
    size_t hashIndex = sizeHashByFileIdAndOffset(fileId, offset);
    HashListNode *hashListNode = lruCache->hashTable[hashIndex];
    while (hashListNode != NULL) {
        if (isSameFile(hashListNode->listNode->fileBlock->fileId, fileId) &&
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

static DataListNode *findDataListNode(LRUCache *lruCache, FileId fileId, off_t offset) {
    HashListNode *hashListNode = getHashListNode(lruCache, fileId, offset);
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
    FileId fileId = hashListNode->listNode->fileBlock->fileId;
    off_t offset = hashListNode->listNode->fileBlock->offset;
    size_t hashIndex = sizeHashByFileIdAndOffset(fileId, offset);
    if (lruCache->hashTable[hashIndex] == hashListNode) {
        lruCache->hashTable[hashIndex] = hashListNode->next;
    }
    hashListNode->prev = NULL;
    hashListNode->next = NULL;
}

void deleteCacheBlockByDataListNode(LRUCache *lruCache, DataListNode *listNode, ErrorHandler *handler) {
    FileBlock *fileBlock = listNode->fileBlock;
    if (fileBlock->status != SYNCED) {
        writeBlockToFile(fileBlock, handler);
    }
    if (handler->statusCode != SUCCESS_CODE) {
        return;
    }
    HashListNode *hashListNodeToFree = getHashListNode(
            lruCache, listNode->fileBlock->fileId, listNode->fileBlock->offset);
    removeDataListNode(lruCache, listNode);
    removeHashListNode(lruCache, hashListNodeToFree);
    freeCacheBlockByHashListNode(hashListNodeToFree);
    lruCache->size--;
}

size_t readBlockFromFile(FileBlock *fileBlock, ErrorHandler *handler) {
    ssize_t bytesRead = 0;
    int fd = fileBlock->fileId.initialFd;
    off_t offset = fileBlock->offset;
    bytesRead = pread(fd, fileBlock->data, CACHE_BLOCK_SIZE_IN_BYTES, offset);
    if (bytesRead == -1) {
        handler->statusCode = errno;
        return bytesRead;
    }
    fileBlock->size = bytesRead;
    fileBlock->status = READ;
    return bytesRead;
}

size_t writeBlockToFile(FileBlock *fileBlock, ErrorHandler *handler) {
    size_t bytesSynced = 0;
    int fd = fileBlock->fileId.initialFd;
    off_t offset = fileBlock->offset;
    size_t size = fileBlock->size;
    bytesSynced = pwrite(fd, fileBlock->data, size, offset);
    if (bytesSynced == -1) {
        handler->statusCode = errno;
        return bytesSynced;
    }
    fileBlock->status = SYNCED;
    return bytesSynced;
}

static void removeLeastRecentlyUsed(LRUCache *lruCache, ErrorHandler *handler) {
    DataListNode *listTail = lruCache->listTail;
    deleteCacheBlockByDataListNode(lruCache, listTail, handler);
}

static DataListNode *addDataListNode(LRUCache *lruCache, FileId fileId, off_t offset, ErrorHandler *handler) {
    size_t hashIndex = sizeHashByFileIdAndOffset(fileId, offset);
    HashListNode *hashListNode = lruCache->hashTable[hashIndex];
    bool isEmptyNode = false;
    if (hashListNode == NULL) {
        isEmptyNode = true;
    }
    while (!isEmptyNode && hashListNode->next != NULL) {
        hashListNode = hashListNode->next;
    }
    DataListNode *listNode = NULL;
    FileBlock *fileBlock = createFileBlock(fileId, offset, handler);
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
    if (lruCache->size == CACHE_BLOCK_COUNT) {
        removeLeastRecentlyUsed(lruCache, handler);
    }
    if (handler->statusCode != SUCCESS_CODE) {
        return listNode;
    }
    moveDataListNodeToHead(lruCache, listNode);
    lruCache->size++;
    if (lruCache->listTail == NULL) {
        lruCache->listTail = listNode;
    }
    return listNode;
}

FileBlock *getFileBlock(LRUCache *lruCache, FileId fileId, off_t offset, ErrorHandler *handler) {
    FileBlock *fileBlock = NULL;
    DataListNode *listNode = findDataListNode(lruCache, fileId, offset);
    if (listNode == NULL) {
        listNode = addDataListNode(lruCache, fileId, offset, handler);
    }
    if (handler->statusCode != SUCCESS_CODE) {
        return fileBlock;
    }
    fileBlock = listNode->fileBlock;
    return fileBlock;
}
