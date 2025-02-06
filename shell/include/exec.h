#ifndef CSHELL_EXEC_H
#define CSHELL_EXEC_H

void setSignalHandling();

typedef struct {
    double execTimeInSeconds;
    int statusCode;
} ExecResult;

ExecResult execCommand(char *args[]);

#endif// CSHELL_EXEC_H
