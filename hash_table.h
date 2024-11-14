#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_NAME_LENGTH 50
#define TABLE_SIZE 100  

typedef struct hash_struct {
    uint64_t hash;
    char name[MAX_NAME_LENGTH];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

// Declare global variables
extern FILE *output_file;
extern pthread_rwlock_t table_rwlock;
extern int lock_releases;
extern hashRecord *hash_table[TABLE_SIZE];

// Declare the functions for hash table operations
void insert(char *name, uint32_t value);
void delete(char *name);
bool deleteSearch(char *name, uint32_t hash_value);
uint32_t search(char *name);
void print();

#endif // HASH_TABLE_H
