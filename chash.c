#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "logger.h"
#include "hash_table.h"

extern FILE *output_file;
extern int lock_acquisitions;
extern int lock_releases;

typedef struct instruction_struct {
    char commandType[10];
    char name[MAX_NAME_LENGTH];
    uint32_t salary;
} instruction;

void *execute_process(void *arg) {
    FILE *command_file = arg;
    char line[128];
    instruction instruct;
    fgets(line, sizeof(line), command_file);
    sscanf(line, "%9[^,],%49[^,],%u\n", instruct.commandType, instruct.name, &instruct.salary);

    if(instruct.commandType != NULL){
        if (strcmp(instruct.commandType, "insert") == 0) {
            insert(instruct.name, instruct.salary);
        } 
        else if (strcmp(instruct.commandType, "delete") == 0) {
            delete(instruct.name);
        } 
        else if (strcmp(instruct.commandType, "search") == 0) {
            uint32_t found_salary = search(instruct.name);
        } 
    }

    return NULL;
}

int main() {
    pthread_rwlock_init(&table_rwlock, NULL);
    output_file = fopen("output.txt", "w");
    if (output_file == NULL) {
        fprintf(stderr, "Failed to open commands.txt\n");
        return EXIT_FAILURE;
    }

    FILE *command_file = fopen("commands.txt", "r");
    if (!command_file) {
        fprintf(stderr, "Failed to open commands.txt\n");
        return EXIT_FAILURE;
    }

    char line[128];
    fgets(line, sizeof(line), command_file);
    int num_threads;
    sscanf(line, "threads,%d,0\n", &num_threads);
    fprintf(output_file, "Running %d threads\n", num_threads);

    pthread_t threads[num_threads];

    log_timestamp("WAITING ON INSERTS");
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, execute_process, command_file);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    fprintf(output_file, "Finished all threads.\n");
    fprintf(output_file, "Number of lock acquisitions: %d\n", lock_acquisitions);
    fprintf(output_file, "Number of lock releases: %d\n", lock_releases);
    print();

    fclose(command_file);
    fclose(output_file);
    pthread_rwlock_destroy(&table_rwlock);

    return ;
}