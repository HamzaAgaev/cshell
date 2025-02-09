#ifndef OS_LAB_2_CACHE_H
#define OS_LAB_2_CACHE_H

#include "utils.h"

#include <sys/types.h>

#define CACHE_BLOCK_COUNT 64
#define CACHE_BLOCK_SIZE (1024 * 1024)

typedef enum {
    NEW,
    CHANGED,
    READ,
    SYNCED
} SyncStatus;

// блок для данных файла, хранящий в себе fd и offset относительно начала файла
// offset кратен CACHE_BLOCK_SIZE
typedef struct {
    int fd;
    off_t offset;
    unsigned char *data;
    size_t size;
    SyncStatus syncStatus;
} FileBlock;

// узел связанного списка для хранения блоков кеша
struct DataListNode {
    FileBlock *fileBlock;
    struct DataListNode *prev;
    struct DataListNode *next;
};

// узел связанного списка для хэш-таблицы (хранил ссылку на узел DataListNode)
struct HashListNode {
    struct DataListNode *listNode;
    struct HashListNode *prev;
    struct HashListNode *next;
};

// структура для хранения кэша
typedef struct {
    struct DataListNode *listHead;
    struct DataListNode *listTail;
    size_t size;
    struct HashListNode **hashTable;
} LRUCache;

LRUCache *createLRUCache(ErrorCatcher *catcher);
void freeLRUCache(LRUCache *lruCache);
off_t getAlignedOffset(off_t offset);
void deleteCacheBlockByDataListNode(LRUCache *lruCache, struct DataListNode *listNode, ErrorCatcher *catcher);
size_t syncFileBlock(FileBlock *fileBlock, ErrorCatcher *catcher);
FileBlock *getFileBlock(LRUCache *lruCache, int fd, off_t offset, ErrorCatcher *catcher);

#endif//OS_LAB_2_CACHE_H
