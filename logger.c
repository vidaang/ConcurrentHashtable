#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "logger.h"
#include <stdint.h>

extern FILE *output_file;

void log_timestamp(const char *message) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(output_file, "%ld: %s\n", tv.tv_sec, message);
}

void log_command(const char *command, uint32_t hash, const char *name, uint32_t salary) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if((strcmp(command, "DELETE")) == 0){
        fprintf(output_file, "%ld: %s,%s\n", tv.tv_sec, command, name);
    } else {
        fprintf(output_file, "%ld: %s,%u,%s,%u\n", tv.tv_sec, command, hash, name, salary);
    }
}
