#include "cio.h"

#include "cache.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define RW_ALL 0666
#define min(a, b) ((a) < (b) ? (a) : (b))

typedef enum {
    READ_MODE,
    WRITE_MODE
} Mode;

static pthread_mutex_t commonMutex;
static ErrorHandler handler = {SUCCESS_CODE};
static LRUCache *lruCache = NULL;

static size_t ceilDiv(size_t a, size_t b) {
    return a / b + (a % b != 0);
}

void initializeLRUCache() {
    pthread_mutex_init(&commonMutex, NULL);
    assert(lruCache == NULL);
    lruCache = createLRUCache(&handler);
}

void destroyLRUCache() {
    assert(lruCache != NULL);
    freeLRUCache(lruCache);
    pthread_mutex_destroy(&commonMutex);
}

#if defined(__APPLE__)
static int cioOpenApple(const char *path) {
    int fd = open(path, O_SYNC | O_RDWR | O_CREAT, RW_ALL);
    if (fd == -1) {
        return -1;
    }
    int fcntl_ = fcntl(fd, F_NOCACHE, 1);
    if (fcntl_ == -1) {
        close(fd);
        return -1;
    }
    return fd;
}
#endif

#if defined(__linux__) && defined(O_DIRECT)
static int cioOpenLinux(const char *path) {
    return open(path, O_SYNC | O_RDWR | O_CREAT | O_DIRECT, RW_ALL);
}
#endif

#if !defined(__APPLE__) && !(defined(__linux__) && defined(O_DIRECT))
static int cioOpenUnknown(const char *path) {
    int fd = open(path, O_SYNC | O_RDWR | O_CREAT, RW_ALL);
    if (fd == -1) {
        close(fd);
        return -1;
    }
    posix_fadvise(fd, 0, 0, POSIX_FADV_RANDOM);
    return fd;
}
#endif

int cioOpen(const char *path) {
#if defined(__APPLE__)
    return cioOpenApple(path);
#elif defined(__linux__) && defined(O_DIRECT)
    return cioOpenLinux(path);
#else
    return cioOpenUnknown(path);
#endif
}

int cioClose(int fd) {
    pthread_mutex_lock(&commonMutex);
    DataListNode *listNode = lruCache->listHead;
    while (listNode != NULL) {
        int currentFd = listNode->fileBlock->fd;
        DataListNode *listNodeCopy = listNode;
        listNode = listNode->next;
        if (currentFd == fd) {
            deleteCacheBlockByDataListNode(lruCache, listNodeCopy, &handler);
        }
    }
    pthread_mutex_unlock(&commonMutex);
    return close(fd);
}

static void readBlockFromFile(FileBlock *fileBlock) {
    int fd = fileBlock->fd;
    off_t offset = fileBlock->offset;
    if (offset == -1) {
        handler.statusCode = errno;
        return;
    }
    ssize_t bytesRead = pread(fd, fileBlock->data, CACHE_BLOCK_SIZE_IN_BYTES, offset);
    if (bytesRead == -1) {
        handler.statusCode = errno;
        return;
    }
    fileBlock->size = bytesRead;
    fileBlock->status = READ;
}

static ssize_t cioReadWrite(int fd, void *buf, size_t count, Mode mode) {
    off_t offset = lseek(fd, 0, SEEK_CUR);
    if (offset == -1) {
        return -1;
    }
    off_t alignedOffset = getAlignedOffset(offset);
    off_t offsetInBlock = offset - alignedOffset;
    size_t bytesBeUsedInCache = ceilDiv(offsetInBlock + count, CACHE_BLOCK_SIZE_IN_BYTES) * CACHE_BLOCK_SIZE_IN_BYTES;
    if (lseek(fd, alignedOffset, SEEK_SET) == -1) {
        return -1;
    }
    pthread_mutex_lock(&commonMutex);
    ssize_t resultSize = 0;
    for (off_t currentOffset = alignedOffset; currentOffset < alignedOffset + bytesBeUsedInCache; currentOffset += (off_t)CACHE_BLOCK_SIZE_IN_BYTES) {
        FileBlock *fileBlock = getFileBlock(lruCache, fd, currentOffset, &handler);
        if (handler.statusCode != SUCCESS_CODE) {
            pthread_mutex_unlock(&commonMutex);
            return -1;
        }
        if (fileBlock->status == NEW) {
            readBlockFromFile(fileBlock);
        }
        off_t beginOffset;
        off_t endOffset;
        if (offset > currentOffset) {
            beginOffset = offsetInBlock;
        } else {
            beginOffset = 0;
        }
        if (currentOffset + CACHE_BLOCK_SIZE_IN_BYTES <= offset + count) {
            endOffset = (off_t)CACHE_BLOCK_SIZE_IN_BYTES;
        } else {
            endOffset = (off_t)((offset + count) % CACHE_BLOCK_SIZE_IN_BYTES);
        }
        switch (mode) {
            case READ_MODE: {
                size_t bytesBeReadToBuffer = 0;
                if (fileBlock->size >= endOffset) {
                    bytesBeReadToBuffer = min(endOffset - beginOffset, fileBlock->size);
                }
                memcpy((unsigned char *)buf + resultSize, fileBlock->data + beginOffset, bytesBeReadToBuffer);
                resultSize += (ssize_t)bytesBeReadToBuffer;
                break;
            }
            case WRITE_MODE: {
                size_t bytesBeWrittenToBuffer = endOffset - beginOffset;
                memcpy(fileBlock->data + beginOffset, (unsigned char *)buf + resultSize, bytesBeWrittenToBuffer);
                if (fileBlock->size < endOffset) {
                    fileBlock->size = endOffset;
                }
                fileBlock->status = CHANGED;
                resultSize += (ssize_t)bytesBeWrittenToBuffer;
                break;
            }
            default: {
                break;
            }
        }
    }
    pthread_mutex_unlock(&commonMutex);
    if (lseek(fd, (off_t)(offset + resultSize), SEEK_SET) == -1) {
        return -1;
    }
    return resultSize;
}

ssize_t cioRead(int fd, void *buf, size_t count) {
    return cioReadWrite(fd, buf, count, READ_MODE);
}

ssize_t cioWrite(int fd, void *buf, size_t count) {
    return cioReadWrite(fd, buf, count, WRITE_MODE);
}

off_t cioLseek(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}

int cioFsync(int fd) {
    pthread_mutex_lock(&commonMutex);
    size_t cacheSize = lruCache->size;
    size_t totalSyncedBytes = 0;
    for (int i = 0; i < cacheSize; i++) {
        FileBlock *fileBlock = lruCache->listHead[i].fileBlock;
        SyncStatus syncStatus = fileBlock->status;
        int currentFd = fileBlock->fd;
        if (currentFd == fd && syncStatus != SYNCED) {
            size_t bytesSynced = syncFileBlock(fileBlock, &handler);
            if (handler.statusCode != SUCCESS_CODE) {
                pthread_mutex_unlock(&commonMutex);
                return -1;
            }
            totalSyncedBytes += bytesSynced;
        }
    }
    pthread_mutex_unlock(&commonMutex);
    return (int)totalSyncedBytes;
}
