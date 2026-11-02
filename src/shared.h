#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>

#define BUFFER_SIZE 8192

typedef struct {
    sem_t mutex;
    sem_t parent_sem;
    sem_t child_sem;
    
    int eof;
    char parent_msg[BUFFER_SIZE];
    char child_msg[BUFFER_SIZE];
    int is_error;
    char filename[256];
} SharedData;

#endif