#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUF_SIZE 5000

int main(int argc, char *argv[]) {
    // Проверка наличия входных и выходных файлов
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Открытие входных и выходных файлов
    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    FILE *output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Создание неименованных каналов
    int channel_1[2], channel_2[2];
    if (pipe(channel_1) == -1 || pipe(channel_2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Создание первого дочернего процесса
    pid_t pid_1 = fork();
    if (pid_1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid_1 == 0) {
        close(channel_1[0]);
        close(channel_2[0]);
        close(channel_2[1]);

        // Считывание входных данных из файла
        char buf[BUF_SIZE];
        int num_bytes = fread(buf, sizeof(char), BUF_SIZE, input_file);

        // Запись входных данных в канал 1
        if (write(channel_1[1], buf, num_bytes) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(channel_1[1]);

        // Выход из первого дочернего процесса
        exit(EXIT_SUCCESS);
    }

    // Создание второго дочернего процесса
    pid_t pid_2 = fork();
    if (pid_2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid_2 == 0) {
        close(channel_1[1]);
        close(channel_2[0]);

        // Чтение входных данных из канала 1
        char buf[BUF_SIZE];
        int num_bytes = read(channel_1[0], buf, BUF_SIZE);

        // Вычисление суммы цифр в строке
        int sum = 0;
        for (int i = 0; i < num_bytes; i++) {
            if (buf[i] >= '0' && buf[i] <= '9') {
                sum += buf[i] - '0';
            }
        }

        // Запись суммы в канал 2
        if (write(channel_2[1], &sum, sizeof(int)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(channel_1[0]);
        close(channel_2[1]);

        // Выход из второго дочернего процесса
        exit(EXIT_SUCCESS);
    }

    // Создание третьего дочернего процесса
    pid_t pid_3 = fork();
    if (pid_3 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid_3 == 0) {
        close(channel_1[0]);
        close(channel_1[1]);
        close(channel_2[1]);

        // Считывание суммы из канала 2
        int sum;
        if (read(channel_2[0], &sum, sizeof(int)) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Запись суммы в выходной файл
        fprintf(output_file, "%d\n", sum);

        close(channel_2[0]);

        // Выход из третьего дочернего процесса
        exit(EXIT_SUCCESS);
    }

// Дожидаемся завершения всех дочерних процессов
    int status;
    waitpid(pid_1, &status, 0);
    waitpid(pid_2, &status, 0);
    waitpid(pid_3, &status, 0);

// Закрытие входных и выходных файлов
    fclose(input_file);
    fclose(output_file);

    return 0;
}