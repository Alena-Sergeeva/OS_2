#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdint.h>

#define SHM_SIZE 1024
#define SEM_NAME "/my_semaphore"
#define SHM_NAME "/my_shared_memory"

static char CLIENT_PROGRAM_NAME[] = "posix_ipc-client";

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        char msg[1024];
        int len = snprintf(msg, sizeof(msg), "usage: %s filename\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_FAILURE);
    }

    // Создаем или открываем объект совместно используемой памяти
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        perror("error: failed to open shared memory");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем размер совместно используемой памяти
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {
        perror("error: failed to ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Отображаем совместно используемую память в адресное пространство процесса
    char *shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("error: failed to map shared memory");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Создаем или открываем семафор
    sem_t *semaphore = sem_open(SEM_NAME, O_CREAT, 0644, 0);
    if (semaphore == SEM_FAILED)
    {
        perror("error: failed to sem_open");
        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("error: failed to fork");
        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        // Дочерний процесс
        char shm_fd_str[10];
        sprintf(shm_fd_str, "%d", shm_fd);

        char path[1050];
        snprintf(path, sizeof(path), "%s/%s", ".", CLIENT_PROGRAM_NAME);

        char *const args[] = {CLIENT_PROGRAM_NAME, argv[1], NULL};
        execv(path, args);

        perror("error: failed to exec into new executable image");

        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }
    else
    {
        // Родительский процесс
        printf("Введите числа через пробел: ");
        fgets(shm_ptr, SHM_SIZE, stdin);

        // Сигнализируем дочернему процессу
        if (sem_post(semaphore) == -1)
        {
            perror("error: failed to sem_post");
        }

        wait(NULL);

        // Освобождаем ресурсы
        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
    }

    return 0;
}