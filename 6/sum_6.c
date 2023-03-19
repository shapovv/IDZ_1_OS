#include <_ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }

    int fd[2]; // Массив для хранения файловых дескрипторов канала.
    if (pipe(fd) == -1) { // Создаем неименованный канал.
        perror("pipe");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        return 1;
    } else if (pid1 == 0) {
        // Дочерний процесс 1 записывает данные из файла в канал.
        close(fd[0]); // Закрываем чтение.
        int input_fd = open(argv[1], O_RDONLY);
        char buffer[BUFFER_SIZE];
        ssize_t n;
        while ((n = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
            write(fd[1], buffer, n);
        }
        close(input_fd);
        close(fd[1]); // Закрываем запись.
        return 0;
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        return 1;
    } else if (pid2 == 0) {
        // Дочерний процесс 2 считывает данные из канала и подсчитывает сумму цифр
        close(fd[1]); // Закрываем запись
        char buffer[BUFFER_SIZE];
        ssize_t n;
        int sum = 0;
        while ((n = read(fd[0], buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < n; i++) {
                if (buffer[i] >= '0' && buffer[i] <= '9') {
                    sum += buffer[i] - '0';
                }
            }
        }

        close(fd[0]); // Закрываем чтение и записываем результат в выходной файл
        int output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        char result_buffer[BUFFER_SIZE];
        snprintf(result_buffer, BUFFER_SIZE, "%d\n", sum);
        write(output_fd, result_buffer, strlen(result_buffer));
        close(output_fd);
        return 0;
    }

    // Родительский процесс ждет завершения всех дочерних процессов
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}