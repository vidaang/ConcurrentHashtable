#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include "hash_table.h"
#include "logger.h"

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
    uint32_t hash_value = jenkins_one_at_a_time_hash(name);
    uint32_t index = hash_value % TABLE_SIZE;    

    pthread_rwlock_wrlock(&table_rwlock);
    lock_acquisitions++;
    log_timestamp("WRITE LOCK ACQUIRED");

    hashRecord *node = hash_table[index];
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
    new_node->next = hash_table[index];
    hash_table[index] = new_node;

    log_command("INSERT", hash_value, name, value);
    lock_releases++;
    log_timestamp("WRITE LOCK RELEASED");
    pthread_rwlock_unlock(&table_rwlock);
}

void delete(char *name) {
    uint32_t hash_value = jenkins_one_at_a_time_hash(name);
    uint32_t index = hash_value % TABLE_SIZE;

    pthread_rwlock_wrlock(&table_rwlock);
    log_timestamp("WRITE LOCK ACQUIRED");
    lock_acquisitions++;

    while(deleteSearch(name, index)){
        pthread_rwlock_unlock(&table_rwlock); 
        usleep(10000);
    }

    log_timestamp("DELETE AWAKENED");
    log_command("DELETE", hash_value, name, 0);
    log_timestamp("WRITE LOCK RELEASED");
    lock_releases++;
    pthread_rwlock_unlock(&table_rwlock);
}

bool deleteSearch(char *name, uint32_t index) {
    pthread_rwlock_wrlock(&table_rwlock);

    hashRecord *node = hash_table[index];
    hashRecord *prev = NULL;
    while (node != NULL) {
        if (strcmp(node->name, name) == 0) {
            if (prev) {
                prev->next = node->next;
            } else {
                hash_table[index] = node->next;
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
    uint32_t hash_value = jenkins_one_at_a_time_hash(name);
    uint32_t index = hash_value % TABLE_SIZE;

    pthread_rwlock_rdlock(&table_rwlock);
    log_timestamp("WRITE LOCK ACQUIRED");
    lock_acquisitions++;

    hashRecord *node = hash_table[index];
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

int compare_hash(const void *a, const void *b) {
    hashRecord *record_a = *(hashRecord **)a;
    hashRecord *record_b = *(hashRecord **)b;

    if (record_a->hash < record_b->hash) return -1;
    if (record_a->hash > record_b->hash) return 1;
    return 0;
}

void print() {
    pthread_rwlock_rdlock(&table_rwlock);
    lock_acquisitions++;
    log_timestamp("READ LOCK ACQUIRED");

    hashRecord *all_records[TABLE_SIZE * 100]; // Assuming MAX_BUCKET_SIZE is the max records in a bucket
    int record_count = 0;

    // Collect records from all hash table buckets
    for (int i = 0; i < TABLE_SIZE; i++) {
        hashRecord *node = hash_table[i];
        while (node) {
            all_records[record_count++] = node;
            node = node->next;
        }
    }

    qsort(all_records, record_count, sizeof(hashRecord *), compare_hash);

    // Print the sorted records
    for (int i = 0; i < record_count; i++) {
        fprintf(output_file, "%u,%s,%u\n", all_records[i]->hash, all_records[i]->name, all_records[i]->salary);
    }

    lock_releases++;
    log_timestamp("READ LOCK RELEASED");
    pthread_rwlock_unlock(&table_rwlock);
}