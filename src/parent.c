#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "shared.h"

int main(void) {
    printf("[Parent] PID: %d\n", getpid());
    // Файл
    int fd = open("shared_mem.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) { perror("open shared_mem.bin"); return 1; }
    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("ftruncate"); return 1;
    }
    SharedData *shm = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); return 1; }

    // семафоры
    sem_init(&shm->mutex, 1, 1);
    sem_init(&shm->parent_sem, 1, 0);
    sem_init(&shm->child_sem, 1, 0);

    // Имя файла
    printf("Имя файла для записи: ");
    fgets(shm->filename, sizeof(shm->filename), stdin);
    if ((unsigned char)shm->filename[0] == '\n') {
        printf("неверное имя файла");
        // fprintf(stderr, "Ошибка: имя файла не может быть пустым!\n");
        return 1;
    }
    shm->filename[strcspn(shm->filename, "\r\n")] = '\0';

    // Дочерний процесс
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0) {
        // ./child
        execl("/project/OS4/cmake-build-debug/child", "child", NULL);
        perror("execl");
        _exit(1);
    }

    // родитель
    char buf[BUFFER_SIZE];
    while (1) {
        printf("Строка: ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            // Конец ввода
            sem_wait(&shm->mutex);
            shm->eof = 1;
            sem_post(&shm->mutex);
            sem_post(&shm->parent_sem);
            break;
        }
        buf[strcspn(buf, "\n")] = '\0';
        sem_wait(&shm->mutex);
        strcpy(shm->parent_msg, buf);
        shm->is_error = 0;
        sem_post(&shm->mutex);

        sem_post(&shm->parent_sem);
        sem_wait(&shm->child_sem);

        // Ответ от ребенка
        sem_wait(&shm->mutex);
        if (shm->is_error) {
            printf("[Ошибка от ребенка]: %s\n", shm->child_msg);
        }
        sem_post(&shm->mutex);
    }
    waitpid(pid, NULL, 0);
    sem_destroy(&shm->mutex);
    sem_destroy(&shm->parent_sem);
    sem_destroy(&shm->child_sem);
    munmap(shm, sizeof(SharedData));
    close(fd);
    unlink("shared_mem.bin");
    return 0;
}
