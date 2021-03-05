#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>

#include "utils.h"
#include "FileMirroringOperations.h"
#include "Stack.h"

#define MAX_CLIENTS 10
#define IP_ADDR_BUFF_SIZE 20
#define READ_BLOCK_SIZE 4096
#define MAX_PATH_SIZE 256

char currentPath[MAX_PATH_SIZE * 2 + 1] = "";
char lastPath[MAX_PATH_SIZE] = "";

Stack_t stack;

int ValidateNumber(char str[]);
int ValidateIpAddress(char *ipAddress);
void ResponseToChildEnd(int sig);
void NewConnection(int serverSocket, int clientSocket, char path[]);

void InitRootDir(char *path);
void GetNextFilePath(PathData_t *pathData);
int TryToSendLastFilePathContent(int clientFd);
void ClearAndCloseDirStack();

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage:%s <ip_address> <port_number> <folder_path>\n", argv[0]);
        exit(1);
    }

    if (!ValidateIpAddress(argv[1]))
    {
        printf("The given IP address is NOT valid\n");
        exit(2);
    }

    if (!ValidateNumber(argv[2]))
    {
        printf("The given server port is not a valid number\n");
        exit(3);
    }

    struct stat folderStatBuffer;
    error_check(lstat(argv[3], &folderStatBuffer), 4, "Lstat error for given path\n");

    if (!S_ISDIR(folderStatBuffer.st_mode))
    {
        printf("The given path should correspond to a folder\n");
        exit(5);
    }

    int serverSocket, clientSocket;
    socklen_t remoteLen;

    short serverPort = (short)atoi(argv[2]);

    struct sockaddr_in localAddress, remoteAddress;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    error_check(serverSocket, 6, "Create server socket error\n");

    set_addr(&localAddress, argv[1], INADDR_ANY, serverPort);

    error_check(bind(serverSocket, (struct sockaddr *)&localAddress, sizeof(struct sockaddr)), 7, "Bind server socket error\n");

    error_check(listen(serverSocket, MAX_CLIENTS), 8, "Listen error\n");

    remoteLen = sizeof(remoteAddress);

    struct sigaction childEnd;
    memset(&childEnd, 0x00, sizeof(struct sigaction));
    childEnd.sa_handler = ResponseToChildEnd;

    error_check(sigaction(SIGCHLD, &childEnd, NULL), 9, "Error for SIGCHLD\n");

    int pid;

    while (1)
    {
        clientSocket = accept(serverSocket, (struct sockaddr *)&remoteAddress, &remoteLen);

        if (clientSocket < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            printf("Accept error with code %d\n", clientSocket);
            exit(10);
        }

        pid = fork();
        error_check(pid, 11, "Fork error\n");

        if (pid == 0)
        {
            NewConnection(serverSocket, clientSocket, argv[3]);
        }
        close(clientSocket);
    }

    return 0;
}

int ValidateNumber(char str[])
{

    int i;
    int len = strlen(str);

    for (i = 0; i < len; i++)
    {
        if (!isdigit(str[i]))
        {
            return 0;
        }
    }

    return 1;
}

int ValidateIpAddress(char *ipAddress)
{
    int number, dots = 0;
    char *p;

    if (ipAddress == NULL)
    {
        return 0;
    }

    if (strlen(ipAddress) > 17)
    {
        return 0;
    }

    char buffer[IP_ADDR_BUFF_SIZE];

    strcpy(buffer, ipAddress);

    p = strtok(buffer, ".");
    if (p == NULL)
    {
        return 0;
    }

    while (p)
    {
        if (!ValidateNumber(p))
        {
            return 0;
        }
        number = atoi(p);
        if (number >= 0 && number <= 255)
        {
            p = strtok(NULL, ".");
            if (p != NULL)
            {
                dots++;
            }
        }
        else
        {
            return 0;
        }
    }

    if (dots != 3)
    {
        return 0;
    }

    return 1;
}

void ResponseToChildEnd(int sig)
{
    int status, waitPid;
    waitPid = wait(&status);
    error_check(waitPid, 6, "Error at wait\n");

    if (!WIFEXITED(status))
    {
        printf("Child proccess with PID equal to %d ended abnormal\n", waitPid);
    }
}

void NewConnection(int serverSocket, int clientSocket, char path[])
{
    int rootFolderNameIndexOffset = strlen(path);
    char charBuffer;
    int nread;

    close(serverSocket);

    InitRootDir(path);

    printf("Connection started...\n");

    while (1)
    {
        while ((nread = read(clientSocket, &charBuffer, sizeof(char))) > 0)
        {
            switch (charBuffer)
            {
            case FMO_SEND_NEXT_PATH:
            {
                PathData_t data;
                GetNextFilePath(&data);
                strcpy(data.path, data.path + rootFolderNameIndexOffset);
                stream_write(clientSocket, &data, sizeof(PathData_t));
            }
            break;

            case FMO_SEND_LAST_PATH_FILE_CONTENT:
                TryToSendLastFilePathContent(clientSocket);
                break;

            default:
                printf("Invalid Character Received!\n");
                break;
            }
        }
        if (nread == 0)
        {
            break;
        }

        error_check(nread, 8, "Reading char from client error");
    }

    printf("Connection closed\n");

    close(clientSocket);

    ClearAndCloseDirStack();
    FreeStack(&stack);

    exit(0);
}

void InitRootDir(char *path)
{
    DIR *rootDir = opendir(path);
    if (rootDir == NULL)
    {
        perror("Open Root Dir Error\n");
        exit(1);
    }
    strcpy(currentPath, path);

    InitStack(&stack);
    StackPush(&stack, (void *)rootDir);
}

int LastSlashIndex(char *str)
{
    for (int i = strlen(str) - 1; i >= 0; i--)
    {
        if (str[i] == '/')
        {
            return i;
        }
    }

    return -1;
}

void GetNextFilePath(PathData_t *pathData)
{
    struct dirent *entry;
    struct stat statBuffer;

    int currentDirHasEntries = 0;
    int currentDirDefaultEntriesCount = 0;

    pathData->fileType = FT_NONE;

    do
    {
        DIR *currentDir = (DIR *)StackPeek(&stack);

        if (currentDir == NULL)
        {
            strcpy(pathData->path, "");
            strcpy(lastPath, "");
            return;
        }
        currentDirHasEntries = 0;
        currentDirDefaultEntriesCount = 0;

        while ((entry = readdir(currentDir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                currentDirDefaultEntriesCount++;
                continue;
            }

            strcat(currentPath, "/");
            strcat(currentPath, entry->d_name);

            error_check(lstat(currentPath, &statBuffer), 2, "Lstat in traversal Error\n");

            pathData->lastModifiedTime = statBuffer.st_mtime;
            strcpy(pathData->path, currentPath);
            strcpy(lastPath, currentPath);

            currentDirHasEntries = 1;

            if (S_ISDIR(statBuffer.st_mode))
            {
                DIR *newDir = opendir(currentPath);
                if (newDir == NULL)
                {
                    perror("Open Dir Error\n");
                    exit(3);
                }

                StackPush(&stack, (void *)newDir);

                break;
            }
            else
            {
                int lastIndex = LastSlashIndex(currentPath);

                if (lastIndex >= 0)
                {
                    currentPath[lastIndex] = '\0';
                }
                pathData->fileType = FT_FILE;
                return;
            }
        }

        if (!currentDirHasEntries)
        {
            error_check(closedir(currentDir), 4, "Close Dir Error\n");

            if (StackPop(&stack) == NULL)
            {
                strcpy(pathData->path, "");
                strcpy(lastPath, "");
                return;
            }

            int lastIndex = LastSlashIndex(currentPath);

            if (lastIndex >= 0)
            {
                currentPath[lastIndex] = '\0';
            }

            if (currentDirDefaultEntriesCount == 2)
            {
                pathData->fileType = FT_FOLDER;
                return;
            }
        }
    } while (1);
}

int TryToSendLastFilePathContent(int clientFd)
{
    int bytesRead = 0;

    if (strlen(lastPath) == 0)
    {
        stream_write(clientFd, &bytesRead, sizeof(bytesRead));
        return 0;
    }

    struct stat statBuffer;

    error_check(lstat(lastPath, &statBuffer), 3, "Lstat in file content Error\n");

    if (S_ISDIR(statBuffer.st_mode))
    {
        stream_write(clientFd, &bytesRead, sizeof(bytesRead));
        return 0;
    }

    int fd = open(lastPath, O_RDONLY);
    error_check(fd, 4, "Open File Error\n");

    char readBuffer[READ_BLOCK_SIZE];

    while ((bytesRead = read(fd, readBuffer, sizeof(readBuffer))) > 0)
    {
        stream_write(clientFd, &bytesRead, sizeof(bytesRead));
        stream_write(clientFd, readBuffer, bytesRead);
    }

    error_check(bytesRead, 5, "Read Error \n");
    error_check(close(fd), 6, "Close File Error\n");

    stream_write(clientFd, &bytesRead, sizeof(bytesRead));

    return 1;
}

void ClearAndCloseDirStack()
{
    DIR *entry = (DIR *)StackPop(&stack);

    while (entry != NULL)
    {
        closedir(entry);
        entry = (DIR *)StackPop(&stack);
    }
}