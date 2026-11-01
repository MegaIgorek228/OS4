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
    // 1. Создаем и открываем файл для отображения в память
    int fd = open("shared_mem.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) { perror("open shared_mem.bin"); return 1; }

    // 2. Устанавливаем размер файла равным размеру нашей структуры
    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("ftruncate"); return 1;
    }

    // 3. Отображаем файл в память (MAP_SHARED делает его общим для процессов)
    SharedData *shm = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); return 1; }

    // 4. Инициализируем семафоры (второй аргумент 1 означает, что они общие для процессов)
    sem_init(&shm->mutex, 1, 1);
    sem_init(&shm->parent_sem, 1, 0);
    sem_init(&shm->child_sem, 1, 0);

    // 5. Запрашиваем имя файла и сохраняем в общую память
    printf("Имя файла для записи: ");
    fgets(shm->filename, sizeof(shm->filename), stdin);
    shm->filename[strcspn(shm->filename, "\r\n")] = '\0';

    // 6. Создаем дочерний процесс
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0) {
        // РЕБЕНОК: запускаем программу ./child
        execl("/project/OS4/cmake-build-debug/child", "child", NULL);
        perror("execl");
        _exit(1);
    }

    // РОДИТЕЛЬ: читаем строки и передаем ребенку
    char buf[BUFFER_SIZE];
    while (1) {
        printf("Строка: ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            // Конец ввода (Ctrl+D)
            sem_wait(&shm->mutex);
            shm->eof = 1;
            sem_post(&shm->mutex);
            sem_post(&shm->parent_sem); // Будим ребенка
            break;
        }
        buf[strcspn(buf, "\n")] = '\0';

        // Записываем строку в общую память
        sem_wait(&shm->mutex);
        strcpy(shm->parent_msg, buf);
        shm->is_error = 0;
        sem_post(&shm->mutex);

        sem_post(&shm->parent_sem); // Сигнал ребенку: "данные готовы!"
        sem_wait(&shm->child_sem);  // Ждем, пока ребенок обработает

        // Читаем ответ от ребенка
        sem_wait(&shm->mutex);
        if (shm->is_error) {
            printf("[Ошибка от ребенка]: %s\n", shm->child_msg);
        }
        sem_post(&shm->mutex);
    }

    // Ожидаем завершения ребенка
    waitpid(pid, NULL, 0);

    // Очистка ресурсов
    sem_destroy(&shm->mutex);
    sem_destroy(&shm->parent_sem);
    sem_destroy(&shm->child_sem);
    munmap(shm, sizeof(SharedData));
    close(fd);
    unlink("shared_mem.bin"); // Удаляем файл

    return 0;
}