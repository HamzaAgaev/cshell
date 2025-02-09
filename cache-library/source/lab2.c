#include "lab2.h"

#include "cache.h"

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define RW_ALL 0666

static pthread_mutex_t commonMutex;
static ErrorCatcher catcher = {SUCCESS_CODE};
static LRUCache *lruCache = NULL;

static size_t ceilDiv(size_t a, size_t b) {
    return a / b + (a % b != 0);
}

static size_t min(size_t a, size_t b) {
    if (a < b) {
        return a;
    }
    return b;
}

void initializeLRUCache() {
    pthread_mutex_init(&commonMutex, NULL);
    if (lruCache == NULL) {
        lruCache = createLRUCache(&catcher);
    }
}

void destroyLRUCache() {
    if (lruCache != NULL) {
        freeLRUCache(lruCache);
    }
    pthread_mutex_destroy(&commonMutex);
}

int lab2Open(const char *path) {
#ifdef __APPLE__
    int fd = open(path, O_SYNC | O_RDWR | O_CREAT, RW_ALL);
    if (fd == -1) {
        return -1;
    }
    int fcntl_ = fcntl(fd, F_NOCACHE, 1);
    if (fcntl_ == -1) {
        return -1;
    }
    return fd;
#elif defined(__linux__)
    return open(path, O_SYNC | O_RDWR | O_CREAT | O_DIRECT, RW_ALL);
#else
    return open(path, O_SYNC | O_RDWR | O_CREAT, RW_ALL);
#endif
}

int lab2Close(int fd) {
    pthread_mutex_lock(&commonMutex);
    struct DataListNode *listNode = lruCache->listHead;
    while (listNode != NULL) {
        int currentFd = listNode->fileBlock->fd;
        struct DataListNode *listNodeCopy = listNode;
        listNode = listNode->next;
        if (currentFd == fd) {
            deleteCacheBlockByDataListNode(lruCache, listNodeCopy, &catcher);
        }
    }
    pthread_mutex_unlock(&commonMutex);
    return close(fd);
}

static void readBlockFromFile(FileBlock *fileBlock, int fd) {
    ssize_t bytesRead = read(fd, fileBlock->data, CACHE_BLOCK_SIZE);
    if (bytesRead == -1) {
        catcher.statusCode = errno;
        return;
    }
    fileBlock->size = bytesRead;
    fileBlock->syncStatus = READ;
}

ssize_t lab2Read(int fd, void *buf, size_t count) {
    off_t offset = lseek(fd, 0, SEEK_CUR);
    if (offset == -1) {
        return -1;
    }
    off_t alignedOffset = getAlignedOffset(offset);
    off_t offsetInBlock = offset - alignedOffset;
    size_t bytesToBeReadInCache = ceilDiv(offsetInBlock + count, CACHE_BLOCK_SIZE) * CACHE_BLOCK_SIZE;
    if (lseek(fd, alignedOffset, SEEK_SET) == -1) {
        return -1;
    }
    pthread_mutex_lock(&commonMutex);
    ssize_t resultReadSize = 0;
    for (off_t currentOffset = alignedOffset; currentOffset < alignedOffset + bytesToBeReadInCache; currentOffset += CACHE_BLOCK_SIZE) {
        FileBlock *fileBlock = getFileBlock(lruCache, fd, currentOffset, &catcher);
        if (catcher.statusCode != SUCCESS_CODE) {
            return -1;
        }
        if (fileBlock->syncStatus == NEW) {
            readBlockFromFile(fileBlock, fd);
        }
        if (catcher.statusCode != SUCCESS_CODE) {
            return -1;
        }
        off_t beginOffset;
        off_t endOffset;
        if (offset > currentOffset) {
            beginOffset = offsetInBlock;
        } else {
            beginOffset = 0;
        }
        if (currentOffset + CACHE_BLOCK_SIZE <= offset + count) {
            endOffset = CACHE_BLOCK_SIZE;
        } else {
            endOffset = (off_t)((offset + count) % CACHE_BLOCK_SIZE);
        }
        size_t bytesToBeReadToBuffer = 0;
        if (fileBlock->size >= endOffset) {
            bytesToBeReadToBuffer = min(endOffset - beginOffset, fileBlock->size);
        }
        memcpy((unsigned char *)buf + currentOffset - alignedOffset, fileBlock->data + beginOffset, bytesToBeReadToBuffer);
        resultReadSize += (ssize_t)bytesToBeReadToBuffer;
    }
    pthread_mutex_unlock(&commonMutex);
    if (lseek(fd, (off_t)(offset + resultReadSize), SEEK_SET) == -1) {
        return -1;
    }
    return resultReadSize;
}

ssize_t lab2Write(int fd, const void *buf, size_t count) {
    off_t offset = lseek(fd, 0, SEEK_CUR);
    if (offset == -1) {
        return -1;
    }
    off_t alignedOffset = getAlignedOffset(offset);
    off_t offsetInBlock = offset - alignedOffset;
    size_t bytesToBeWrittenToCache = ceilDiv(count, CACHE_BLOCK_SIZE) * CACHE_BLOCK_SIZE;
    if (lseek(fd, alignedOffset, SEEK_SET) == -1) {
        return -1;
    }
    pthread_mutex_lock(&commonMutex);
    ssize_t resultWrittenSize = 0;
    for (off_t currentOffset = alignedOffset; currentOffset < alignedOffset + bytesToBeWrittenToCache; currentOffset += CACHE_BLOCK_SIZE) {
        FileBlock *fileBlock = getFileBlock(lruCache, fd, currentOffset, &catcher);
        if (catcher.statusCode != SUCCESS_CODE) {
            return -1;
        }
        if (fileBlock->syncStatus == NEW) {
            readBlockFromFile(fileBlock, fd);
        }
        off_t beginOffset;
        off_t endOffset;
        if (offset > currentOffset) {
            beginOffset = offsetInBlock;
        } else {
            beginOffset = 0;
        }
        if (currentOffset + CACHE_BLOCK_SIZE <= offset + count) {
            endOffset = CACHE_BLOCK_SIZE;
        } else {
            endOffset = (off_t)((offset + count) % CACHE_BLOCK_SIZE);
        }
        size_t bytesToBeWrittenToBuffer = endOffset - beginOffset;
        memcpy(fileBlock->data + beginOffset, (unsigned char *)buf + currentOffset - alignedOffset, bytesToBeWrittenToBuffer);
        if (fileBlock->size < endOffset) {
            fileBlock->size = endOffset;
        }
        fileBlock->syncStatus = CHANGED;
        resultWrittenSize += (ssize_t)bytesToBeWrittenToBuffer;
    }
    pthread_mutex_unlock(&commonMutex);
    if (lseek(fd, (off_t)(offset + resultWrittenSize), SEEK_SET) == -1) {
        return -1;
    }
    return resultWrittenSize;
}

off_t lab2Lseek(int fd, off_t offset, int whence) {
    int fSync = lab2Fsync(fd);
    if (fSync == -1) {
        return -1;
    }
    return lseek(fd, offset, whence);
}

int lab2Fsync(int fd) {
    pthread_mutex_lock(&commonMutex);
    size_t cacheSize = lruCache->size;
    size_t totalSyncedBytes = 0;
    for (int i = 0; i < cacheSize; i++) {
        FileBlock *fileBlock = lruCache->listHead[i].fileBlock;
        SyncStatus syncStatus = fileBlock->syncStatus;
        int currentFd = fileBlock->fd;
        if (currentFd == fd && syncStatus != SYNCED) {
            size_t bytesSynced = syncFileBlock(fileBlock, &catcher);
            if (catcher.statusCode != SUCCESS_CODE) {
                return -1;
            }
            totalSyncedBytes += bytesSynced;
        }
    }
    pthread_mutex_unlock(&commonMutex);
    return (int)totalSyncedBytes;
}
