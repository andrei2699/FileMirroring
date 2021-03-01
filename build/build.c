#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 4096

#define MAX_ARRAY_SIZE 10
#define MAX_FILE_NAME_SIZE 100

#define LIBRARY 0
#define SOURCE 1

char libraries[MAX_ARRAY_SIZE][MAX_FILE_NAME_SIZE];
char sources[MAX_ARRAY_SIZE][MAX_FILE_NAME_SIZE];

static inline int check(int errorCode, int exitCode, const char *message);
static inline int isDelimiter(char c);

const char librariesName[] = "libraries";
const char sourcesName[] = "sources";

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s build file\n", argv[0]);
        exit(1);
    }

    struct stat fileStat;
    check(lstat(argv[1], &fileStat), 2, "LSTAT ERROR\n");

    if (!S_ISREG(fileStat.st_mode))
    {
        printf("Argument is not a file \n");
        exit(2);
    }

    char buffer[BUFFER_SIZE];

    int fd = open(argv[1], O_RDONLY);
    check(fd, 3, "Open File Error\n");

    int bytesRead;
    int librariesCount = 0;
    int sourcesCount = 0;

    char element[MAX_FILE_NAME_SIZE];
    int currentElementIndex = 0;

    int fileType = -1;

    while ((bytesRead = check(read(fd, buffer, sizeof(buffer)), 4, "Read From File Error\n")) > 0)
    {
        for (int i = 0; i < bytesRead; i++)
        {
            element[currentElementIndex] = buffer[i];
            if (isDelimiter(buffer[i]))
            {
                element[currentElementIndex] = '\0';

                if (strcmp(element, librariesName) == 0)
                {
                    fileType = LIBRARY;
                }
                else if (strcmp(element, sourcesName) == 0)
                {
                    fileType = SOURCE;
                }
                else if (currentElementIndex > 1)
                {
                    switch (fileType)
                    {
                    case LIBRARY:
                    {
                        strcpy(libraries[librariesCount++], element);
                    }
                    break;

                    case SOURCE:
                    {
                        strcpy(sources[sourcesCount++], element);
                    }
                    break;
                    }
                }

                currentElementIndex = 0;
            }
            else
            {
                currentElementIndex++;
            }
        }
    }

    if (currentElementIndex > 1)
    {
        element[currentElementIndex] = '\0';
        switch (fileType)
        {
        case LIBRARY:
        {
            strcpy(libraries[librariesCount++], element);
        }
        break;

        case SOURCE:
        {
            strcpy(sources[sourcesCount++], element);
        }
        break;
        }
    }

    int status;

    for (int i = 0; i < librariesCount; i++)
    {
        int pid = fork();
        check(pid, i + 5, "Fork Error\n");

        if (pid == 0)
        {

            execlp("gcc", "gcc", "-Wall", "-c", "-g", libraries[i], NULL);
            perror("Execlp Error\n");
            exit(-1);
        }

        pid = wait(&status);
        check(pid, librariesCount + 1, "Wait Error\n");

        if (!WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            printf("Child with PID %d ended with %d\n", pid, WEXITSTATUS(status));
        }
    }

    int n;
    char argumente[BUFFER_SIZE][15];
    int index = 0;
    for (n = 0; n < BUFFER_SIZE; n++)
    {
        strcpy(argumente[n], "");
    }
    strcpy(argumente[index++], "gcc");
    strcpy(argumente[index++], "-Wall");
    strcpy(argumente[index++], "-o");
    int outputFileNameIndex = index;
    strcpy(argumente[index++], "");
    int sourceFileNameIndex = index;
    strcpy(argumente[index++], "");

    for (int i = 0; i < librariesCount; i++)
    {
        strcpy(buffer, libraries[i]);
        buffer[strlen(buffer) - 1] = 'o';

        for (int startIndex = strlen(buffer) - 1; startIndex >= 0; startIndex--)
        {
            if (buffer[startIndex] == '/')
            {
                strcpy(buffer, buffer + startIndex + 1);
                break;
            }
        }
        strcpy(argumente[index++], buffer);
    }

    char *values[BUFFER_SIZE];
    for (n = 0; n < index; n++)
    {
        values[n] = argumente[n];
    }
    values[index] = NULL;

    for (int i = 0; i < sourcesCount; i++)
    {
        int pid = fork();
        check(pid, i + 5 + librariesCount, "Fork Error\n");

        if (pid == 0)
        {
            strcpy(buffer, sources[i]);
            buffer[strlen(buffer) - 2] = '\0';

            strcpy(argumente[outputFileNameIndex], buffer);
            strcpy(argumente[sourceFileNameIndex], sources[i]);

            execvp("gcc", values);
            printf("EXECV Error\n");
            exit(-2);
        }
    }

    for (int i = 0; i < sourcesCount; i++)
    {
        int pid = wait(&status);
        check(pid, librariesCount + 1 + sourcesCount, "Wait Error\n");

        if (!WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            printf("Child with PID %d ended with %d\n", pid, WEXITSTATUS(status));
        }
    }
    check(close(fd), 6, "Close File Error\n");

    return 0;
}

static inline int isDelimiter(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == ',' || c == ';';
}

static inline int check(int errorCode, int exitCode, const char *message)
{
    if (errorCode < 0)
    {
        perror(message);
        exit(exitCode);
    }
    return errorCode;
}