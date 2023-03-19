#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }
    // Создаем именованные каналы
    const char *p1_pipe = "/tmp/p1_pipe";
    const char *p2_pipe = "/tmp/p2_pipe";
    mkfifo(p1_pipe, 0666);
    mkfifo(p2_pipe, 0666);

    pid_t pid1 = fork();
    if (pid1 == -1) {
        printf("Error creating first child process.\n");
        return 1;
    } else if (pid1 == 0) {
        // Дочерний процесс 1 записывает данные из файла в именованный канал p1_pipe
        close(STDIN_FILENO);
        int input_fd = open(argv[1], O_RDONLY);
        close(STDOUT_FILENO);
        int p1_fd = open(p1_pipe, O_WRONLY);
        char buffer[BUFFER_SIZE];
        ssize_t n;
        while ((n = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
            write(p1_fd, buffer, n);
        }
        close(input_fd);
        close(p1_fd);
        return 0;
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        printf("Error creating second child process.\n");
        return 1;
    } else if (pid2 == 0) {
        // Дочерний процесс 2 считывает данные из именованного канала p1_pipe подсчитывает сумму цифр в строке
        close(STDIN_FILENO);
        int p1_fd = open(p1_pipe, O_RDONLY);
        close(STDOUT_FILENO);
        int p2_fd = open(p2_pipe, O_WRONLY);
        char buffer[BUFFER_SIZE];
        ssize_t n;
        int digit_sum = 0;
        while ((n = read(p1_fd, buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < n; i++) {
                if (isdigit(buffer[i])) {
                    digit_sum += buffer[i] - '0';
                }
            }
        }
        close(p1_fd);
        // Записываем результат в именованный канал p2_pipe
        char result_buffer[BUFFER_SIZE];
        snprintf(result_buffer, BUFFER_SIZE, "%d\n", digit_sum);
        write(p2_fd, result_buffer, strlen(result_buffer));
        close(p2_fd);
        return 0;
    }
    pid_t pid3 = fork();
    if (pid3 == -1) {
        printf("Error creating third child process.\n");
        return 1;
    } else if (pid3 == 0) {
        // Дочерний процесс 3: читает данные из именованного канала p2 и записывает их в выходной файл.
        int output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        int p2_fd = open(p2_pipe, O_RDONLY);
        char buffer[BUFFER_SIZE];
        ssize_t n;
        while ((n = read(p2_fd, buffer, BUFFER_SIZE)) > 0) {
            write(output_fd, buffer, n);
        }
        close(output_fd);
        close(p2_fd);
        return 0;
    }
// Родительский процесс ждет завершения всех дочерних процессов.
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

// Удаляем именованные каналы.
    unlink(p1_pipe);
    unlink(p2_pipe);

    return 0;
}