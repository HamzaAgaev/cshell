#include "benchmark-utils.h"

#include "cio.h"

#include <ctype.h>
#include <stdlib.h>

size_t readWordFromFile(int fd, char *buf, size_t maxSize) {
    size_t totalBytesRead = 0;
    int size = 0;
    while (size < maxSize - 1) {
        char symbol;
        size_t bytesRead = cioRead(fd, &symbol, sizeof(char));
        if (bytesRead == -1) {
            return -1;
        }
        if (bytesRead == 0 || symbol == '\0' || (isspace(symbol) && size > 0)) {
            break;
        }
        if (!isspace(symbol)) {
            buf[size] = symbol;
            size++;
            totalBytesRead += bytesRead;
        }
    }
    buf[size] = '\0';
    return totalBytesRead;
}

size_t readIntFromFile(int fd, int *buf) {
    char strBuffer[MAX_CHARACTERS_FOR_INT];
    size_t totalBytesRead = readWordFromFile(fd, strBuffer, MAX_CHARACTERS_FOR_INT);
    if (totalBytesRead > 0) {
        *buf = atoi(strBuffer);
    }
    return totalBytesRead;
}