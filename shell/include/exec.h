#ifndef OS_LAB_1_EXEC_H
#define OS_LAB_1_EXEC_H

void setSignalHandling();

typedef struct {
    double execTimeInSeconds;
    int statusCode;
} ExecResult;

ExecResult execCommand(char *args[]);

#endif// OS_LAB_1_EXEC_H
