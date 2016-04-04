
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/errno.h>

#define MAX_LENGTH 100  // expression max length
#define MAX_NUMBER 10   // max operand number

/* is a number? */
int is_number(const char *str) {
    char c = *str++;

    // the first char is allowed to be '-'
    if (isdigit(c) == 0 && c != '-') {
        return 0;
    }

    while (*str) {
        c = *str++;
        if (isdigit(c) == 0) {
            return 0;
        }
    }

    return 1;
}

/* is a valid expression */
int is_valid(const char *str) {
    while (*str) {
        char c = *str++;
        if (!(c == ' ' || c == '+' || c == '-' || c == '*' || c == '/' || (c >= '0' && c <= '9') || c == '(' ||
                                                                                                    c == ')')) {
            return 0;
        }
    }

    return 1;
}

/* resolove an expression */
int *resolve(const char *exp, int *result) {
    result[1] = 0;
    // return if the expression is a single number
    if (is_number(exp)) {
        result[0] = atoi(exp);
        printf("PROCESS %d: Sending '%d' on pipe to parent\n", getpid(), result[0]);
        return result;
    }

    // resolve the expression
    int seg_num     = 0;
    int curPos      = 0;
    int operand_num = 0;
    int bracket_num = 0;
    char operator   = 'x';
    char **operand  = (char **) malloc(sizeof(char *) * MAX_NUMBER);
    char curExp[MAX_LENGTH];

    for (int i = 1, l = strlen(exp); i < l; i++) {
        char c = exp[i];

        switch (c) {
            case ' ':
                if (bracket_num == 0 && curPos != 0) {
                    curExp[curPos] = '\0';
                    if (seg_num == 0) {
                        seg_num++;
                        if (curPos == 1 && (*curExp == '+' || *curExp == '-' || *curExp == '*' || *curExp == '/')) {
                            operator = *curExp;
                            printf("PROCESS %d: Starting '%c' operation\n", getpid(), operator);
                        } else {
                            result[1] = -1;
                            printf("PROCESS %d: ERROR: unknown '%s' operator\n", getpid(), curExp);
                            return result;
                        }
                    } else {
                        operand[operand_num] = (char *) malloc(sizeof(char) * (strlen(curExp) + 1));
                        strcpy(operand[operand_num++], curExp);
                    }

                    curPos = 0;
                } else if (bracket_num != 0) {
                    curExp[curPos++] = c;
                }
                break;

            case '(':
                bracket_num++;
                curExp[curPos++] = c;
                break;

            case ')':
                bracket_num--;
                if (bracket_num != -1) {
                    curExp[curPos++] = c;
                    if (bracket_num == 0) {
                        curExp[curPos] = '\0';
                        operand[operand_num] = (char *) malloc(sizeof(char) * (strlen(curExp) + 1));
                        strcpy(operand[operand_num++], curExp);
                        curPos = 0;
                    }
                } else if (curPos != 0) {
                    curExp[curPos] = '\0';
                    operand[operand_num] = (char *) malloc(sizeof(char) * (strlen(curExp) + 1));
                    strcpy(operand[operand_num++], curExp);
                    curPos = 0;
                }
                break;
            default:
                curExp[curPos++] = c;
                break;
        }
    }

    // return error if operands are not enough
    if (operand_num < 2) {
        result[1] = -1;
        printf("PROCESS %d: ERROR: not enough operands\n", getpid());
        return result;
    }

    // create pipes
    int **pipes = (int **) malloc(sizeof(int *) * operand_num);
    for (int i = 0; i < operand_num; i++) {
        pipes[i] = (int *) malloc(sizeof(int) * 2);
        pipe(pipes[i]);
    }

    pid_t pid;
    int *results = (int *) malloc(sizeof(int) * operand_num);
    int operand_index = 0;
    for (int i = 0; i < operand_num; i++) {
        pid = fork();

        if (pid == -1) {
            perror("fork() failed");
            result[1] = -1;
            return result;
        } else if (pid == 0) {
            char res[10];
            char buffer[MAX_LENGTH];
            int expResult[2];
            int bytes_read = (int) read(pipes[operand_index][0], buffer, MAX_LENGTH);
            buffer[bytes_read] = '\0';
            close(pipes[operand_index][0]);

            resolve(buffer, expResult);
            results[operand_index] = expResult[0];
            sprintf(res, "%d", expResult[0]);

            if (expResult[1] == -1) {
                result[1] = -1;
                write(pipes[operand_index][1], "ERROR", 5);
            } else {
                int bytes_written = (int) write(pipes[operand_index][1], res, strlen(res));
                if (bytes_written == -1) {
                    printf("PROCESS %d: write() failed\n", getpid());
                }
            }
            close(pipes[operand_index][1]);
            exit(0);
        } else {
            int bytes_written = (int) write(pipes[operand_index][1], operand[operand_index], strlen(operand[operand_index]));
            if (bytes_written == -1) {
                printf("PROCESS %d: write() failed\n", getpid());
            }
            close(pipes[operand_index][1]);
            operand_index++;
        }
    }

    if (pid > 0) {
        int status;
        wait(&status);

        if (WIFSIGNALED(status)) {
            printf("PARENT: child %d terminated abnormally\n", pid);
            result[1] = -1;
        }
        else if (WIFEXITED(status)) {
            int rc = WEXITSTATUS(status);
            if (rc != 0) {
                printf("PARENT: child %d terminated with nonzero exit status %d\n", pid, rc);
            }
        }
    }

    while (waitpid(-1, NULL, 0)) {
        if (errno == ECHILD) {
            break;
        }
    }

    for (int i = 0; i < operand_num; i++) {
        char res[10];
        int res_len = read(pipes[i][0], res, 10);
        res[res_len] = '\0';
        if (strcmp(res, "ERROR") == 0) {
            result[1] = -1;
            return result;
        }
        results[i] = atoi(res);
    }

    switch (operator) {
        case '+':
            result[0] = 0;
            for (int i = 0; i < operand_num; i++) {
                int o = results[i];
                result[0] += o;
            }
            break;

        case '-':
            result[0] = results[0];
            for (int i = 1; i < operand_num; i++) {
                int o = results[i];
                result[0] -= o;
            }
            break;

        case '*':
            result[0] = 1;
            for (int i = 0; i < operand_num; i++) {
                int o = results[i];
                result[0] *= o;
            }
            break;

        case '/':
            result[0] = results[0];
            for (int i = 1; i < operand_num; i++) {
                int o = results[i];
                result[0] /= o;
            }
            break;

        default:
            result[1] = -1;
            return result;
    }

    free(pipes);
    free(results);
    free(operand);

    printf("PROCESS %d: Sending '%d' on pipe to parent\n", getpid(), result[0]);
    return result;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        char *input_file = argv[1];

        FILE *fp = fopen(input_file, "r");

        if (fp == NULL) {
            printf("ERROR: File not found\n");
            return EXIT_FAILURE;
        } else {
            char *exp = (char *) malloc(sizeof(char) * MAX_LENGTH);

            while (fgets(exp, MAX_LENGTH, fp) != NULL) {
                if (exp[strlen(exp) - 1] == '\n') {
                    exp[strlen(exp) - 1] = '\0';
                }
                int result[2];

                if (is_valid(exp)) {
                    resolve(exp, result);
                } else {
                    printf("Error: Invalid expression\n");
                    return EXIT_FAILURE;
                }

                if (result[1] != -1) {
                    printf("PROCESS %d: Final answer is '%d'\n", getpid(), result[0]);
                }
                //printf("- - - - - - - - - - \n");
                free(exp);
                exp = (char *) malloc(sizeof(char) * MAX_LENGTH);

                sleep(0);
            }

            return EXIT_SUCCESS;
        }
    } else {
        printf("ERROR: Invalid arguments\nUSAGE: ./a.out <input-file>\n");
        return EXIT_FAILURE;
    }
}


