#include "exec.h"

#include <stdio.h>
#include <string.h>

#define DEFAULT_TEXT_SIZE 40
#define MAX_ARGS 100
#define MAX_COMMAND_LENGTH 100

#define GREEN_TEXT(buffer, text) snprintf(buffer, sizeof(buffer), "\033[31m%s\033[0m\n", text)
#define RED_TEXT(buffer, text) snprintf(buffer, sizeof(buffer), "\033[32m%s\033[0m\n", text)

void printNewLine() {
    fprintf(stdout, "\n");
}

int main() {
    setSignalHandling();
    const char inputTextFormat[] = "shell >> ";
    char execTextFormat[DEFAULT_TEXT_SIZE];
    RED_TEXT(execTextFormat, "Exec time: %f");
    char errorTextFormat[DEFAULT_TEXT_SIZE];
    GREEN_TEXT(errorTextFormat, "Error code: %d");
    const char exitText[] = "exit";
    const char delimiter[] = " ";
    char entered_command[MAX_COMMAND_LENGTH];
    do {
        fprintf(stdout, inputTextFormat);
        if (fgets(entered_command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            break;
        }
        const size_t len = strlen(entered_command);
        if (len > 0 && entered_command[len - 1] == '\n') {
            entered_command[len - 1] = '\0';
        }
        char *command_args[MAX_ARGS + 1];
        int args_count = 0;
        char *arg = strtok(entered_command, delimiter);
        while (arg != NULL && args_count < MAX_ARGS) {
            command_args[args_count] = arg;
            args_count++;
            arg = strtok(NULL, delimiter);
        }
        if (strcmp(command_args[0], exitText) == 0) {
            break;
        }
        command_args[args_count + 1] = NULL;
        const ExecResult result = execCommand(command_args);
        fprintf(stdout, execTextFormat, result.execTimeInSeconds);
        if (result.statusCode != 0) {
            fprintf(stdout, errorTextFormat, result.statusCode);
        }
        printNewLine();
    } while (true);
    return 0;
}
