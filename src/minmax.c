#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int readFileSize(FILE *file);
void readMinMaxIntRange(FILE *file, int startIndex, int endIndex, int *min, int *max);

int main(int argc, char *argv[]) {
    if (argc < 4) {  // Check for correct number of arguments
        printf("Usage: minmax <filename> <numOfProcesses> <processNum>\n");
        return 1;
    }

    const char *filename = argv[1];  // Get filename from command line
    int numOfProcesses = atoi(argv[2]);
    int processNum = atoi(argv[3]);

    if (numOfProcesses <= 0 || processNum < 0 || processNum >= numOfProcesses) {
        printf("Error: processNum must be in range, and numOfProcesses must be positive.\n");
        return 1;
    }

    FILE *ft = fopen(filename, "rb");
    if (ft == NULL) {
        perror("Error opening file");
        return 1;
    }

    int numInts = readFileSize(ft);
    int sectionSize = numInts / numOfProcesses;

    int startIndex = sectionSize * processNum;
    int endIndex = startIndex + sectionSize - 1;

    // Ensure the last process gets any remaining values
    if (processNum == numOfProcesses - 1) {
        endIndex = numInts - 1;
    }

    int min = INT_MAX;
    int max = INT_MIN;

    readMinMaxIntRange(ft, startIndex, endIndex, &min, &max);

    fclose(ft);

    // Send min and max through stdout (to be captured by pipe in parent)
    printf("%d %d\n", min, max);
    return 0;
}

int readFileSize(FILE *file) {
    fseek(file, 0, SEEK_END);  // Move to end of file
    long size = ftell(file);   // Get current position (file size in bytes)
    if (size < 0) {
        return 0;
    }
    fseek(file, 0, SEEK_SET);  // Reset to beginning of file
    return (int)(size / (long)sizeof(int)); // Number of integers in the file
}

void readMinMaxIntRange(FILE *file, int startIndex, int endIndex, int *min, int *max) {
    if (startIndex > endIndex) {
        return;
    }

    fseek(file, (long)startIndex * (long)sizeof(int), SEEK_SET);

    int value;
    for (int i = startIndex; i <= endIndex; i++) {
        if (fread(&value, sizeof(int), 1, file) == 1) {
            if (value > *max) {
                *max = value;
            }
            if (value < *min) {
                *min = value;
            }
        } else {
            /* stderr: stdout is the pipe back to prog02. */
            fprintf(stderr, "Error reading at index %d\n", i);
            break;
        }
    }
}
