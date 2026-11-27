#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

void pipecall(int numForks, const char *filename);
static void getMinmaxPath(char *out, size_t outSize);

int main(int argc, char *argv[]) {
    if (argc < 3) {  // Check if filename and numForks are provided
        printf("Usage: prog02 <numOfProcesses> <filename>\n");
        return 1;
    }

    const char *filename = argv[2];  // Get filename
    int numForks = atoi(argv[1]);    // Convert numForks to integer

    if (numForks <= 0) {
        printf("Error: numOfProcesses must be a positive integer.\n");
        return 1;
    }

    pipecall(numForks, filename);
    return 0;
}

/* minmax is installed next to this binary (see /proc/self/exe). */
static void getMinmaxPath(char *out, size_t outSize) {
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);

    if (len > 0) {
        exePath[len] = '\0';
        char *slash = strrchr(exePath, '/');
        if (slash != NULL) {
            *slash = '\0';
            snprintf(out, outSize, "%s/minmax", exePath);
            return;
        }
    }

    /* Fall back to a minmax binary in the current directory. */
    snprintf(out, outSize, "./minmax");
}

void pipecall(int numForks, const char *filename) {
    int cp[numForks][2];   // Pipes for communication
    pid_t pids[numForks];  // Child pids, so the parent can wait
    int min = INT_MAX;     // Global min
    int max = INT_MIN;     // Global max
    char minmaxPath[PATH_MAX];

    getMinmaxPath(minmaxPath, sizeof(minmaxPath));

    for (int i = 0; i < numForks; i++) {
        if (pipe(cp[i]) < 0) {
            perror("Pipe creation failed");
            exit(1);
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (pid == 0) {  // Child Process
            /* Close read ends inherited from earlier pipes, then our own.
               The parent has already closed the earlier write ends. */
            for (int j = 0; j < i; j++) {
                close(cp[j][0]);
            }
            close(cp[i][0]);  // Close read end of the pipe

            if (dup2(cp[i][1], STDOUT_FILENO) < 0) { // Redirect stdout to pipe
                perror("dup2 failed");
                exit(1);
            }
            close(cp[i][1]);  // Close write end (not needed after dup2)

            // Convert process number to string
            char processNumStr[16], numForksStr[16];
            snprintf(processNumStr, sizeof(processNumStr), "%d", i);
            snprintf(numForksStr, sizeof(numForksStr), "%d", numForks);

            execl(minmaxPath, "minmax", filename, numForksStr, processNumStr, (char *)NULL);

            perror("execl failed");
            exit(1);
        } else {  // Parent Process
            pids[i] = pid;
            close(cp[i][1]);  // Close write end of the pipe
        }
    }

    // Parent Process: Collect results from child processes
    int readFailed = 0;
    for (int i = 0; i < numForks; i++) {
        int childMin, childMax;
        FILE *pipeStream = fdopen(cp[i][0], "r");
        if (pipeStream == NULL) {
            perror("fdopen failed");
            exit(1);
        }

        if (fscanf(pipeStream, "%d %d", &childMin, &childMax) == 2) {
            if (childMin < min) min = childMin;
            if (childMax > max) max = childMax;
        } else {
            fprintf(stderr, "Failed to read min/max from child %d\n", i);
            readFailed = 1;
        }

        fclose(pipeStream);
    }

    int childFailed = 0;
    for (int i = 0; i < numForks; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid failed");
            exit(1);
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child %d failed\n", i);
            childFailed = 1;
        }
    }

    if (readFailed || childFailed) {
        exit(1);
    }

    printf("Global Min: %d\n", min);
    printf("Global Max: %d\n", max);
}
