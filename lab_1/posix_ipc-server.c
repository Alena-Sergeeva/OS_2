#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

static char CLIENT_PROGRAM_NAME[] = "posix_ipc-client";

int main(int argc, char **argv)
{
	if (argc == 1)
	{
		char msg[1024];
		// Вывод имени файла в который будет писаться результат
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

	// NOTE: Get full path to the directory, where program resides
	char progpath[1024];
	{
		// Пишем путь к исполняемому файлу, readlink копирует ссылку не копируя терминирующий элемент
		ssize_t len = readlink("/proc/self/exe", progpath,
							   sizeof(progpath) - 1);
		if (len == -1)
		{
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

		// идем к концу пути и добавляем 0
		while (progpath[len] != '/')
			--len;

		progpath[len] = '\0';
	}

	// Открываем канал
	int channel[2]; // массив для файловых дескрипторов канала
	/* Create a one-way communication channel (pipe).
   If successful, two file descriptors are stored in PIPEDES;
   bytes written on PIPEDES[1] can be read from PIPEDES[0].
   Returns 0 if successful, -1 if not.  */
	/*Неуспешное завершение pipe скорее всего означает,
	что переполнилась таблица открытых файлов у процесса либо у всей системы.*/
	if (pipe(channel) == -1)
	{
		const char msg[] = "error: failed to create pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	// Порождаем от родителя нового ребенка
	// В случае успеха в ребенка возвращается 0, в родителя iD ребенка, иначе -1
	const pid_t child = fork();
	// дальше код выполняется в ребенке
	switch (child)
	{
	case -1:
	{ // Ядро не смогло создать новый процесс
		const char msg[] = "error: failed to spawn new process\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}
	break;

	case 0:
	{
		// Код выполняется в сыновьем процессе
		//????????? получаем pid ребенка	  // NOTE: We're a child, child doesn't know its pid after fork
		pid_t pid = getpid(); // NOTE: Get child PID

		// NOTE: Соединяем stdin родителя с stdin ребенка STDIN_FILENO = 0
		dup2(STDIN_FILENO, channel[STDIN_FILENO]);
		close(channel[STDOUT_FILENO]); // закрываем выходной поток ребенка

		{
			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
											"%d: I'm a child\n", pid);
			write(STDOUT_FILENO, msg, length);
		}

		{
			char path[1050];
			// Вывод пути к исполняемому клиентскому файлу
			snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME);

			// Первый аргумент в командной строке имя исполняемой программы, второй имя файла для записи
			// выходных данных
			//
			//  NOTE: `NULL` at the end is mandatory, because `exec*`
			//        expects a NULL-terminated list of C-strings
			char *const args[] = {CLIENT_PROGRAM_NAME, argv[1], NULL}; // массив контантных указателей на строки

			// замена тела процесса, виртуальное адресное пространство процесса полностью заменяется
			int32_t status = execv(path, args);
			// загрузка в виртуальное адресное пространство новой программы и
			// необходимых библиотек и передача управления на точку входа новой программы.

			if (status == -1)
			{
				const char msg[] = "error: failed to exec into new exectuable image\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		}
	}
	break;

	default:
	{						  // находимся либо в ребенке, либо в родителе		  // NOTE: We're a parent, parent knows PID of child after fork
		pid_t pid = getpid(); // NOTE: Get parent PID

		{
			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
											"%d: I'm a parent, my child has PID %d\n", pid, child);
			write(STDOUT_FILENO, msg, length);
		}

		// ждем пока ребенок существует -- блокируем родителя
		int child_status;
		wait(&child_status);

		if (child_status != EXIT_SUCCESS)
		{
			const char msg[] = "error: child exited with error\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(child_status);
		}
	}
	break;
	}
}