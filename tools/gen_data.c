#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

/* Write count random 32-bit integers (host byte order) to a binary file.
   Usage: gen_data <count> <outfile> [seed] */

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: gen_data <count> <outfile> [seed]\n");
        return 1;
    }

    char *end = NULL;
    errno = 0;
    long count = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || errno != 0 || count <= 0 || count > INT_MAX) {
        fprintf(stderr, "Error: count must be a positive integer.\n");
        return 1;
    }

    unsigned long seed = 1;
    if (argc == 4) {
        errno = 0;
        end = NULL;
        seed = strtoul(argv[3], &end, 10);
        if (end == argv[3] || *end != '\0' || errno != 0) {
            fprintf(stderr, "Error: seed must be an unsigned integer.\n");
            return 1;
        }
    }

    FILE *out = fopen(argv[2], "wb");
    if (out == NULL) {
        perror("Error opening file");
        return 1;
    }

    srand((unsigned)seed);

    for (long i = 0; i < count; i++) {
        unsigned hi = (unsigned)rand();
        unsigned lo = (unsigned)rand();
        int value = (int)((hi << 16) ^ lo);
        if (fwrite(&value, sizeof(value), 1, out) != 1) {
            perror("Error writing file");
            fclose(out);
            return 1;
        }
    }

    if (fclose(out) != 0) {
        perror("Error closing file");
        return 1;
    }

    return 0;
}
