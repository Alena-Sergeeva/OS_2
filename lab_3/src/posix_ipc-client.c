#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg), "usage: %s filename, wrong cnt_arg\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];

    // Открываем семафор
    sem_t *semaphore = sem_open(SEM_NAME, 0);
    if (semaphore == SEM_FAILED)
    {
        perror("error: failed to open semaphore");
        exit(EXIT_FAILURE);
    }

    // Открываем совместно используемую память
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0);
    if (shm_fd < 0)
    {
        perror("error: failed to open shared memory");
        sem_close(semaphore);
        exit(EXIT_FAILURE);
    }

    // Отображаем совместно используемую память в адресное пространство процесса
    char *shm_ptr = mmap(0, 1024, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("error: failed to map shared memory");
        shm_unlink(SHM_NAME);
        sem_close(semaphore);
        exit(EXIT_FAILURE);
    }

    // Ожидаем семафор
    if (sem_wait(semaphore) == -1)
    {
        perror("error: failed sem_wait");
    }

    // Считываем числа из общей памяти и считаем сумму
    char *input = shm_ptr;

    int j = 0, length = 0;
    long int res = 0, sum = 0;
    char msg[33];
    char num[20];
    for (int i = 0; i < strlen(input); ++i)
    {
        if (!isspace(input[i]))
        {
            num[j++] = input[i];
        }
        else
        {
            if (j != 0)
            {
                num[j] = '\0';
                sum += strtol(num, NULL, 10);
                j = 0;
            }
        }
    }

    // Записываем сумму в указанный файл
    int file = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file == -1)
    {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        munmap(shm_ptr, 1024);
        sem_close(semaphore);
        exit(EXIT_FAILURE);
    }
    char sum_str[60];
    snprintf(sum_str, sizeof(sum_str), "%ld -- sum\n", sum);
    int32_t written = write(file, sum_str, strlen(sum_str));
    if (written != strlen(sum_str))
    {
        snprintf(sum_str, sizeof(sum_str) - 1, "error: failed to write to file\n");
        write(STDERR_FILENO, sum_str, sizeof(sum_str));
        exit(EXIT_FAILURE);
    }
    close(file);
    munmap(shm_ptr, 1024);
    sem_close(semaphore);
    return 0;
}