#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <_ctype.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }

    char *pipe_name = "/tmp/pipe";
    mkfifo(pipe_name, 0666); // Создаем именованный канал.

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        return 1;
    } else if (pid1 == 0) {
        // Дочерний процесс 1 записывает данные из файла в канал.
        int input_fd = open(argv[1], O_RDONLY);
        int pipe_fd = open(pipe_name, O_WRONLY); // Открываем именованный канал на запись.
        char buffer[BUFFER_SIZE];
        ssize_t n;
        while ((n = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
            write(pipe_fd, buffer, n);
        }
        close(input_fd);
        close(pipe_fd);
        return 0;
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        return 1;
    } else if (pid2 == 0) {
        // Дочерний процесс считывает данные из канала и подсчитывает количество цифр и букв.
        int pipe_fd = open(pipe_name, O_RDONLY); // Открываем именованный канал на чтение.
        int output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        char buffer[BUFFER_SIZE];
        ssize_t n;
        int sum = 0;
        while ((n = read(pipe_fd, buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < n; i++) {
                if (buffer[i] >= '0' && buffer[i] <= '9') {
                    sum += buffer[i] - '0';
                }
            }
        }
        close(pipe_fd);
        // Записываем результат в выходной файл.
        char result_buffer[BUFFER_SIZE];
        snprintf(result_buffer, BUFFER_SIZE, "%d\n", sum);
        write(output_fd, result_buffer, strlen(result_buffer));
        close(output_fd);
        return 0;
    }

    // Родительский процесс ждет завершения всех дочерних процессов.
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // Удаляем именованный канал.
    unlink(pipe_name);

    return 0;
}