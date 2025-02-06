#include "exec.h"

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define CHILD_PID_DEFAULT (-1)

#define NANOSEC_IN_SEC 1e9

volatile pid_t childPid = CHILD_PID_DEFAULT;

static void signalHandler(int signal) {
    if (childPid > 0) {
        kill(-childPid, signal);
    } else {
        _exit(0);
    }
}

static void resetChildPid() {
    childPid = CHILD_PID_DEFAULT;
}

void setSignalHandling() {
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGSTOP, &sa, NULL);
}

ExecResult execCommand(char *args[]) {
    childPid = vfork();
    ExecResult result = {.execTimeInSeconds = 0, .statusCode = 0};
    if (childPid <= -1) {
        result.statusCode = errno;
        result.execTimeInSeconds = 0;
    } else if (childPid == 0) {
        setpgid(childPid, childPid);
        const int exec_code = execvp(args[0], args);
        const int error_code = exec_code == 0 ? 0 : errno;
        _exit(error_code);
    } else {
        setpgid(childPid, childPid);
        struct timespec start;
        struct timespec end;
        int stat_loc;
        clock_gettime(CLOCK_MONOTONIC, &start);
        waitpid(childPid, &stat_loc, WUNTRACED | WCONTINUED);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (WIFEXITED(stat_loc)) {
            result.statusCode = WEXITSTATUS(stat_loc);
        } else if (WIFSIGNALED(stat_loc)) {
            result.statusCode = WTERMSIG(stat_loc);
        } else if (WIFSTOPPED(stat_loc)) {
            kill(childPid, SIGKILL);
        }
        const double exec_time_in_sec =
                (double)(end.tv_sec - start.tv_sec) + ((double)(end.tv_nsec - start.tv_nsec) / NANOSEC_IN_SEC);
        result.execTimeInSeconds = exec_time_in_sec;
    }
    resetChildPid();
    return result;
}
