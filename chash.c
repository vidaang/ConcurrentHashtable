#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include<unistd.h>

void insert(char *name, uint32_t value);
void delete(char *name);
bool deleteSearch(char *name, uint32_t hash_value);
uint32_t search(char *name);
void print();
void *execute_process(void *arg);
void log_timestamp(const char *message);
void log_command(const char *command, uint32_t hash, const char *name, uint32_t salary);

FILE *output_file;
int lock_acquisitions = 0;
int lock_releases = 0;

#define MAX_NAME_LENGTH 50
#define TABLE_SIZE 100  // Adjust size based on expected entries

// Define the structure of each record in the hash table
typedef struct hash_struct {
    uint32_t hash;
    char name[MAX_NAME_LENGTH];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

typedef struct instruction_struct {
    char commandType[10];
    char name[MAX_NAME_LENGTH];
    uint32_t salary;
} instruction;

// Global hash table with a simple array of linked lists for each bucket
hashRecord *hash_table[TABLE_SIZE];

// Reader-writer lock and condition variables
pthread_rwlock_t table_rwlock;

// Jenkins one-at-a-time hash function
uint32_t jenkins_one_at_a_time_hash(const char *name) {
    uint32_t hash = 0;
    while (*name) {
        hash += *name++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

// Insert function
void insert(char *name, uint32_t value) {

    uint32_t hash_value = jenkins_one_at_a_time_hash(name) % TABLE_SIZE;
    
    pthread_rwlock_wrlock(&table_rwlock);
    lock_acquisitions++;
    log_timestamp("WRITE LOCK ACQUIRED");

    // Check if the node exists and update if it does
    hashRecord *node = hash_table[hash_value];
    while (node != NULL) {
        if (strcmp(node->name, name) == 0) {
            node->salary = value;
            
            log_command("INSERT", hash_value, name, value);
            lock_releases++;
            log_timestamp("WRITE LOCK RELEASED");
            pthread_rwlock_unlock(&table_rwlock);
            return;
        }
        node = node->next;
    }

    // Insert new node if it doesn't exist
    hashRecord *new_node = malloc(sizeof(hashRecord));
    new_node->hash = hash_value;
    strncpy(new_node->name, name, MAX_NAME_LENGTH - 1);
    new_node->salary = value;
    new_node->next = hash_table[hash_value];
    hash_table[hash_value] = new_node;

    log_command("INSERT", hash_value, name, value);
    lock_releases++;
    log_timestamp("WRITE LOCK RELEASED");
    pthread_rwlock_unlock(&table_rwlock);
}

// Delete function
void delete(char *name) {

    uint32_t hash_value = jenkins_one_at_a_time_hash(name) % TABLE_SIZE;

    pthread_rwlock_wrlock(&table_rwlock);
    log_timestamp("WRITE LOCK ACQUIRED");
    lock_acquisitions++;

    while(deleteSearch(name, hash_value)){
        pthread_rwlock_unlock(&table_rwlock); // make sure previous unlock happened 
        usleep(10000);  // add wait time of 10 miliseconds to allow for another thread to take over 
    }

    log_timestamp("DELETE AWAKENED");
    log_command("DELETE", hash_value, name, 0);
    log_timestamp("WRITE LOCK RELEASED");
    lock_releases++;
    pthread_rwlock_unlock(&table_rwlock);
}

bool deleteSearch(char *name, uint32_t hash_value){
    pthread_rwlock_wrlock(&table_rwlock);


    hashRecord *node = hash_table[hash_value];
    hashRecord *prev = NULL;
    while (node != NULL) {
        if (strcmp(node->name, name) == 0) {
            if (prev) {
                prev->next = node->next;
            } else {
                hash_table[hash_value] = node->next;
            }
            free(node);
            return true;
        }
        prev = node;
        node = node->next;
    }
    log_timestamp("WRITE LOCK ACQUIRED");
    lock_acquisitions++;
    log_timestamp("WRITE LOCK RELEASED");
    lock_releases++;
    pthread_rwlock_unlock(&table_rwlock);
    return false;
}

// Search function
uint32_t search(char *name) {

    uint32_t hash_value = jenkins_one_at_a_time_hash(name) % TABLE_SIZE;

    pthread_rwlock_rdlock(&table_rwlock);
    log_timestamp("WRITE LOCK ACQUIRED");
    lock_acquisitions++;

    hashRecord *node = hash_table[hash_value];
    while (node != NULL) {
        if (strcmp(node->name, name) == 0) {
            uint32_t value = node->salary;
            
            log_command("SEARCH", hash_value, name, value);
            lock_releases++;
            log_timestamp("READ LOCK RELEASED");
            pthread_rwlock_unlock(&table_rwlock);
            return value;
        }
        node = node->next;
    }

    log_timestamp("SEARCH: NOT FOUND NOT FOUND");
    lock_releases++;
    log_timestamp("READ LOCK RELEASED");
    pthread_rwlock_unlock(&table_rwlock);
    return 0;  // Assume 0 as NULL equivalent for missing data
}

// Function to print the hash table
void print() {
    pthread_rwlock_rdlock(&table_rwlock);
    lock_acquisitions++;
    log_timestamp("READ LOCK ACQUIRED");

    for (int i = 0; i < TABLE_SIZE; i++) {
        hashRecord *node = hash_table[i];
        while (node) {
            fprintf(output_file, "%u,%s,%u\n", node->hash, node->name, node->salary);
            node = node->next;
        }
    }
    lock_releases++;
    log_timestamp("READ LOCK RELEASED");
    pthread_rwlock_unlock(&table_rwlock);
}


// Command execution function
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

// Main program to read commands and start threads
int main() {
    // Initialize reader-writer lock
    pthread_rwlock_init(&table_rwlock, NULL);

    output_file = fopen("output.txt", "w");

    // Process commands from commands.txt and create threads
    FILE *command_file = fopen("commands.txt", "r");
    if (!command_file) {
        fprintf(stderr, "Failed to open commands.txt\n");
        return EXIT_FAILURE;
    }
 
    // Read the first line for the number of threads
    char line[128];
    fgets(line, sizeof(line), command_file);
    int num_threads;
    sscanf(line, "threads,%d,0\n", &num_threads);
    fprintf(output_file, "Running %d threads\n", num_threads);

    // Create threads to process commands concurrently
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

    return 0;
}
