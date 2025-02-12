#ifndef OS_LAB_2_LAB2_H
#define OS_LAB_2_LAB2_H

#include <sys/types.h>

void initializeLRUCache();
void destroyLRUCache();
int lab2Open(const char *path);
int lab2Close(int fd);
ssize_t lab2Read(int fd, void *buf, size_t count);
ssize_t lab2Write(int fd, void *buf, size_t count);
off_t lab2Lseek(int fd, off_t offset, int whence);
int lab2Fsync(int fd);

#endif//OS_LAB_2_LAB2_H
