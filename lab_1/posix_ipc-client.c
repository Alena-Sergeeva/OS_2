#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>

#include <string.h>
#include <ctype.h>

enum err
{
	OK,
	LONG_INT_OVERFLOW,
	NOT_NUM
};

int check_long_int(char *num, int size, long int *res)
{
	char *end_num = NULL;
	*res = strtol(num, &end_num, 10);
	if ((*res == LONG_MAX) || (*res == LONG_MIN))
	{
		return LONG_INT_OVERFLOW;
	}

	if ((*end_num != '\0') || (*res == 0 && strcmp(num, "0") != 0))
	{
		return NOT_NUM;
	}
	return 0;
}

int main(int argc, char **argv)
{
	char buf[4096];
	ssize_t bytes;

	pid_t pid = getpid();

	// `O_WRONLY` файл для записи
	// `O_CREAT` Если файл не существует, он будет создан.
	// `O_TRUNC`Если файл существует и успешно открывается на запись либо
	// на чтение и запись, то его размер усекается до нуля.
	// `O_APPEND` Запись производится в конец файла.
	int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
	if (file == -1)
	{
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	{
		char msg[128];
		int32_t len = snprintf(msg, sizeof(msg) - 1,
							   "%d: Start typing lines of text. Press 'Ctrl-D' or 'Enter' with no input to exit\n", pid);
		write(STDOUT_FILENO, msg, len);
	}

	while (bytes = read(STDIN_FILENO, buf, sizeof(buf)))
	{
		if (bytes < 0)
		{
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		else if (buf[0] == '\n')
		{
			// завершение ввода
			break;
		}

		{
			int j = 0, length = 0;
			long int res = 0, sum = 0;
			char msg[33];
			char num[20];
			for (int i = 0; i < bytes / sizeof(char); ++i)
			{
				if (!isspace(buf[i]))
				{
					num[j++] = buf[i];
				}
				else
				{
					if (j != 0)
					{
						num[j] = '\0';
						switch (check_long_int(num, j, &res))
						{
						case LONG_INT_OVERFLOW:
							length = snprintf(msg, sizeof(msg) - 1, "error: overflow long int type\n");
							write(STDERR_FILENO, msg, length);
							exit(EXIT_FAILURE);
							break;
						case NOT_NUM:
							length = snprintf(msg, sizeof(msg) - 1, "error: lecsema not number\n");
							write(STDERR_FILENO, msg, length);
							exit(EXIT_FAILURE);
							break;
						case OK:
							if (((res > 0) && (sum > LONG_MAX - res)) || ((res < 0) && (sum < LONG_MIN - res)))
							{
								length = snprintf(msg, sizeof(msg) - 1, "error: overflow long int type\n");
								write(STDERR_FILENO, msg, sizeof(msg));
								exit(EXIT_FAILURE);
							}
							else
							{
								sum += res;
							}
							break;
						}
						j = 0;
					}
				}
			}
			int32_t len = snprintf(msg, sizeof(msg) - 1,
								   "%ld -- sum\n", sum);
			int32_t written = write(file, msg, len);
			if (written != len)
			{
				length = snprintf(msg, sizeof(msg) - 1, "error: failed to write to file\n");
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		}
	}

	// записываем последний символ в файл
	const char term = '\0';
	write(file, &term, sizeof(term));

	close(file);
}