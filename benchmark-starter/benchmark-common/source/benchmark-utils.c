#include "benchmark-utils.h"

#include "cio.h"

#include <ctype.h>
#include <stdlib.h>

size_t readIntFromFile(int fd, int *buf) {
    size_t totalBytesRead = 0;
    char strBuffer[MAX_CHARACTERS_FOR_INT];
    int size = 0;
    while (size < MAX_CHARACTERS_FOR_INT - 1) {
        char symbol;
        size_t bytesRead = cioRead(fd, &symbol, sizeof(char));
        if (bytesRead == -1) {
            return -1;
        }
        if (bytesRead == 0 || symbol == '\0' || (isspace(symbol) && size > 0)) {
            break;
        }
        if (!isspace(symbol)) {
            strBuffer[size] = symbol;
            size++;
            totalBytesRead += bytesRead;
        }
    }
    strBuffer[size] = '\0';
    if (totalBytesRead > 0) {
        *buf = atoi(strBuffer);
    }
    return totalBytesRead;
}