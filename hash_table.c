#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hash_table.h"
#include "logger.h"

// extern FILE *output_file;
// pthread_rwlock_t table_rwlock = PTHREAD_RWLOCK_INITIALIZER;
// extern int lock_acquisitions;
// extern int lock_releases;
// extern hashRecord *hash_table[TABLE_SIZE];

FILE *output_file = NULL;
int lock_acquisitions = 0;
int lock_releases = 0;
hashRecord *hash_table[TABLE_SIZE] = {NULL};

pthread_rwlock_t table_rwlock = PTHREAD_RWLOCK_INITIALIZER;

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

void insert(char *name, uint32_t value) {
    uint32_t hash_value = jenkins_one_at_a_time_hash(name) % TABLE_SIZE;
    
    pthread_rwlock_wrlock(&table_rwlock);
    lock_acquisitions++;
    log_timestamp("WRITE LOCK ACQUIRED");

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

void delete(char *name) {
    uint32_t hash_value = jenkins_one_at_a_time_hash(name) % TABLE_SIZE;
    pthread_rwlock_wrlock(&table_rwlock);
    log_timestamp("WRITE LOCK ACQUIRED");
    lock_acquisitions++;

    while(deleteSearch(name, hash_value)){
        pthread_rwlock_unlock(&table_rwlock); 
        usleep(10000);
    }

    log_timestamp("DELETE AWAKENED");
    log_command("DELETE", hash_value, name, 0);
    log_timestamp("WRITE LOCK RELEASED");
    lock_releases++;
    pthread_rwlock_unlock(&table_rwlock);
}

bool deleteSearch(char *name, uint32_t hash_value) {
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

    log_timestamp("SEARCH: NOT FOUND");
    lock_releases++;
    log_timestamp("READ LOCK RELEASED");
    pthread_rwlock_unlock(&table_rwlock);
    return 0;
}

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
