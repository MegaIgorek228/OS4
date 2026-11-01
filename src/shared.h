#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>

#define BUFFER_SIZE 8192

typedef struct {
    sem_t mutex;            // Замок для защиты данных
    sem_t parent_sem;       // Сигнал от родителя (данные готовы)
    sem_t child_sem;        // Сигнал от ребенка (ответ готов)
    
    int eof;                // Флаг окончания ввода
    char parent_msg[BUFFER_SIZE]; // Данные от родителя
    char child_msg[BUFFER_SIZE];  // Ошибка от ребенка
    int is_error;           // Флаг, что ребенок нашел ошибку
    char filename[256];     // Имя файла для записи
} SharedData;

#endif