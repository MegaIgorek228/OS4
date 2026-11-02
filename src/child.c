#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include "shared.h"

int main(void) {
    printf("[Child] PID: %d\n", getpid());
    int fd = open("shared_mem.bin", O_RDWR);
    if (fd == -1) { perror("open shared_mem.bin"); return 1; }
    SharedData *shm = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); return 1; }
    FILE *out = fopen(shm->filename, "w");
    if (!out) { perror("fopen"); return 1; }

    while (1) {
        sem_wait(&shm->parent_sem);
        sem_wait(&shm->mutex);
        if (shm->eof) {
            sem_post(&shm->mutex);
            break;
        }

        // Заглавна буква
        if (isupper((unsigned char)shm->parent_msg[0])) {
            fprintf(out, "%s\n", shm->parent_msg);
            fflush(out);
            shm->is_error = 0;
        } else {
            snprintf(shm->child_msg, sizeof(shm->child_msg), "Строка должна начинаться с заглавной буквы: \"%s\"", shm->parent_msg);
            shm->is_error = 1;
        }
        sem_post(&shm->mutex);
        sem_post(&shm->child_sem);
    }

    fclose(out);
    munmap(shm, sizeof(SharedData));
    close(fd);
    return 0;
}