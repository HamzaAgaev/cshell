#ifndef CSHELL_CIO_H
#define CSHELL_CIO_H

#define CIO_SEEK_SET 0 /* set file offset to offset */
#define CIO_SEEK_CUR 1 /* set file offset to current plus offset */
#define CIO_SEEK_END 2 /* set file offset to EOF plus offset */

#include <sys/types.h>

void initializeLRUCache();
void destroyLRUCache();
int cioOpen(const char *path);
int cioClose(int fd);
ssize_t cioRead(int fd, void *buf, size_t count);
ssize_t cioWrite(int fd, void *buf, size_t count);
off_t cioLseek(int fd, off_t offset, int whence);
int cioFsync(int fd);

#endif//CSHELL_CIO_H
