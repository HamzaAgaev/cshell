#ifndef OS_LAB_2_CACHE_H
#define OS_LAB_2_CACHE_H

#include "utils.h"

#include <sys/stat.h>
#include <sys/types.h>

extern const size_t CACHE_BLOCK_COUNT;
extern const size_t CACHE_BLOCK_SIZE_IN_BYTES;

typedef struct {
    int initialFd;
    ino_t inode;
    dev_t device;
} FileId;

typedef enum {
    NEW,
    CHANGED,
    READ,
    SYNCED
} SyncStatus;

// блок для данных файла, хранящий в себе fileId и offset относительно начала файла
// offset кратен CACHE_BLOCK_SIZE_IN_BYTES
typedef struct {
    FileId fileId;
    off_t offset;
    unsigned char *data;
    size_t size;
    SyncStatus status;
} FileBlock;

typedef struct DataListNode {
    FileBlock *fileBlock;
    struct DataListNode *prev;
    struct DataListNode *next;
} DataListNode;

// узел связанного списка для хэш-таблицы (хранил ссылку на узел DataListNode)
typedef struct HashListNode {
    DataListNode *listNode;
    struct HashListNode *prev;
    struct HashListNode *next;
} HashListNode;

// структура для хранения кэша
typedef struct {
    DataListNode *listHead;
    DataListNode *listTail;
    size_t size;
    HashListNode **hashTable;
} LRUCache;

FileId getFileIdByFd(int fd);
bool isSameFile(FileId fileId1, FileId fileId2);
LRUCache *createLRUCache(ErrorHandler *handler);
void freeLRUCache(LRUCache *lruCache);
off_t getAlignedOffset(off_t offset);
void deleteCacheBlockByDataListNode(LRUCache *lruCache, DataListNode *listNode, ErrorHandler *handler);
size_t writeBlockToFile(FileBlock *fileBlock, ErrorHandler *handler);
size_t readBlockFromFile(FileBlock *fileBlock, ErrorHandler *handler);
FileBlock *getFileBlock(LRUCache *lruCache, FileId fileId, off_t offset, ErrorHandler *handler);

#endif//OS_LAB_2_CACHE_H
