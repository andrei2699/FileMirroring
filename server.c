#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

#include <time.h>

#include "utils.h"
#include "FileMirroringOperations.h"

#include "stack.h"

#define MAX_PATH_SIZE 256
#define READ_BLOCK_SIZE 4096

char directoryPath[] = "../../dir";
char currentPath[MAX_PATH_SIZE * 2 + 1] = "";
char lastPath[MAX_PATH_SIZE] = "";

Stack_t stack;

typedef struct
{
    char path[MAX_PATH_SIZE];
    time_t lastModifiedTime;
    FileType_t fileType;
} PathData_t;

void InitRootDir(char *path);
void GetNextFilePath(PathData_t *pathData);
int TryToSendLastFilePathContent(int clientFd);

int main()
{
    PathData_t data;

    int fileCount = 0;

    InitRootDir(directoryPath);
    do
    {
        GetNextFilePath(&data);

        fileCount++;
        if (fileCount == 1)
        {
            TryToSendLastFilePathContent(1);
        }

        printf("%s %d\n", data.path, data.fileType);
        // printf("%s %d %s %ld\n", data.path, data.fileType, ctime(&data.lastModifiedTime), data.lastModifiedTime);
    } while (strlen(data.path) > 0);

    FreeStack(&stack);

    return 0;
}

void InitRootDir(char *path)
{
    DIR *rootDir = opendir(directoryPath);
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
        DIR *currentDir = StackPeek(&stack);

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
    if (strlen(lastPath) == 0)
    {
        return 0;
    }

    struct stat statBuffer;

    error_check(lstat(lastPath, &statBuffer), 3, "Lstat in file content Error\n");

    if (S_ISDIR(statBuffer.st_mode))
    {
        return 0;
    }

    int fd = open(lastPath, O_RDONLY);
    error_check(fd, 4, "Open File Error\n");

    char readBuffer[READ_BLOCK_SIZE];
    int bytesRead;

    while ((bytesRead = read(fd, readBuffer, sizeof(readBuffer))) > 0)
    {
        stream_write(clientFd, readBuffer, bytesRead);
    }

    error_check(bytesRead, 5, "Read Error \n");

    error_check(close(fd), 6, "Close File Error\n");

    return 1;
}