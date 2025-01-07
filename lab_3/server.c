#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char CLIENT_PROGRAM_NAME[] = "posix_ipc-client";

// Структура для хранения общих данных
typedef struct
{
    char message[1024];
    sem_t sem_read;
    sem_t sem_write;
} shared_data_t;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg), "usage: %s filename\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_SUCCESS);
    }

    // Открываем совместно используемую память
    int shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        perror("error: failed to open shared memory");
        exit(EXIT_FAILURE);
    }

    // Увеличиваем размер совместно используемой памяти
    ftruncate(shm_fd, sizeof(shared_data_t));

    // Отображаем совместно используемую память в адресное пространство процесса
    shared_data_t *shared_data = mmap(0, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED)
    {
        perror("error: failed to map shared memory");
        exit(EXIT_FAILURE);
    }

    // Инициализируем семафоры
    sem_init(&shared_data->sem_read, 1, 0);  // начинаем с 0, чтобы ребенок ждал
    sem_init(&shared_data->sem_write, 1, 1); // разрешаем запись

    const pid_t child = fork();
    switch (child)
    {
    case -1:
    {
        perror("error: failed to spawn new process");
        exit(EXIT_FAILURE);
    }
    case 0:
    {
        // Код выполняется в дочернем процессе
        pid_t pid = getpid();
        snprintf(shared_data->message, sizeof(shared_data->message), "%d: I'm a child\n", pid);

        // Синхронизация: сигналим родителю, что есть сообщение
        sem_post(&shared_data->sem_read);

        // Ждем, пока родитель прочитает сообщение
        sem_wait(&shared_data->sem_write);

        // Запускаем клиентский процесс
        char path[1050];
        snprintf(path, sizeof(path), "%s/%s", ".", CLIENT_PROGRAM_NAME);

        char *const args[] = {CLIENT_PROGRAM_NAME, argv[1], NULL};
        execv(path, args);

        perror("error: failed to exec into new executable image");
        exit(EXIT_FAILURE);
    }
    default:
    {
        // Код выполняется в родительском процессе
        pid_t pid = getpid();
        char msg[64];
        snprintf(msg, sizeof(msg), "%d: I'm a parent, my child has PID %d\n", pid, child);
        write(STDOUT_FILENO, msg, strlen(msg));

        // Ждем, пока ребенок напишет сообщение
        sem_wait(&shared_data->sem_read);

        // Считываем сообщение из общей памяти
        write(STDOUT_FILENO, shared_data->message, strlen(shared_data->message));

        // Сигналим дочернему процессу, что мы прочитали сообщение
        sem_post(&shared_data->sem_write);

        // Ждем завершения дочернего процесса
        int child_status;
        wait(&child_status);
        if (child_status != EXIT_SUCCESS)
        {
            perror("error: child exited with error");
            exit(child_status);
        }
    }
    }

    // Освобождаем ресурсы
    sem_destroy(&shared_data->sem_read);
    sem_destroy(&shared_data->sem_write);
    munmap(shared_data, sizeof(shared_data_t));
    shm_unlink("/my_shared_memory");

    return 0;
}