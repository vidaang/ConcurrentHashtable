#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "logger.h"
#include <stdint.h>

extern FILE *output_file;

void log_timestamp(const char *message) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;  // Convert to microseconds
    fprintf(output_file, "%lu: %s\n", timestamp, message);           // Print timestamp with message
}

void log_command(const char *command, uint32_t hash, const char *name, uint32_t salary) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;  // Convert to microseconds
    
    if (strcmp(command, "DELETE") == 0) {
        fprintf(output_file, "%lu: %s,%s\n", timestamp, command, name);
    } else {
        fprintf(output_file, "%lu: %s,%u,%s,%u\n", timestamp, command, hash, name, salary);
    }
}
