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
#include <libgen.h>

#include "utils.h"
#include "FileMirroringOperations.h"
#include "List.h"

#define IP_ADDR_BUFF_SIZE 20
#define READ_BLOCK_SIZE 4096
#define MAX_PATH_SIZE 256

PathData_t data;

int ValidateNumber(char str[]);
int ValidateIpAddress(char *ipAddress);
void ListFilesRecursively(ListNode_t *head, char name[100]);
void CreateFile(char *dataPath);
void CreateFolder(char *dataPath);
void WriteToFile(int fd, int sockfd);
int LastSlashIndex(char *str);

int main(int argc, char *argv[])
{
    if(argc != 4)
	{
		printf("Usage:%s <ip_address> <port_number> <folder_path>\n", argv[0]);
		exit(1);
	}

    if(!ValidateIpAddress(argv[1]))
	{
		printf("The given IP address is NOT valid\n");
		exit(2);
	}
	
	if(!ValidateNumber(argv[2]))
	{
		printf("The given server port is not a valid number\n");
		exit(3);
	}

    struct stat folderStatBuffer;
	error_check(lstat(argv[3], &folderStatBuffer), 4, "Lstat error for given path\n");

    if(!S_ISDIR(folderStatBuffer.st_mode))
	{
		printf("The given path should correspond to a folder\n");
		exit(5);
	}

    int sockfd;
	short serverPort = (short) atoi(argv[2]);
    struct sockaddr_in localAddress, remoteAddress;

    sockfd = socket (PF_INET, SOCK_STREAM, 0); 
	error_check(sockfd, 6, "Create client socket error\n");

    set_addr(&localAddress, NULL, INADDR_ANY, 0);
    error_check(bind(sockfd, (struct sockaddr *)&localAddress, sizeof(localAddress)), 7, "Bind client socket error\n");

    set_addr(&remoteAddress, argv[1], 0, serverPort);
    error_check(connect(sockfd, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress)), 8, "Connect client to server socket error\n");

    // TODO : create path list

	ListNode_t *headServer = (ListNode_t *)malloc(sizeof(ListNode_t));
	InitList(headServer);

	ListNode_t *head = (ListNode_t *)malloc(sizeof(ListNode_t));
	InitList(head);
	ListFilesRecursively(head, argv[3]);
	// ListPrint(head);

	char cmd;
	char rootDir[MAX_PATH_SIZE];

    do
    {
        cmd = FMO_SEND_NEXT_PATH;
        write(sockfd, &cmd, sizeof(char));
        recv(sockfd, &data, sizeof(PathData_t), 0);

        if (strcmp(data.path, "") != 0) {
			sleep(5);
        	strcpy(rootDir, argv[3]);
			strcat(rootDir, data.path);
			strcpy(data.path, rootDir);
			ListAdd(headServer, data.path);
			// printf("%s\n", data.path);

        	if (ListSearch(head, data.path))
			{
				struct stat fileStat;
				error_check(lstat(data.path, &fileStat), 9, "Lstat error for given path\n");

				if (!S_ISDIR(fileStat.st_mode))
				{
					if (fileStat.st_mtime != data.lastModifiedTime) 
					{
						int fd = open(data.path, O_WRONLY);
						error_check(fd, 4, "Open File Error\n");
					
						WriteToFile(fd, sockfd);
					}
				}

				printf("%s\n", data.path);

				ListRemove(head, data.path);
			} 
			else 
			{
			
				if(data.fileType == FT_FOLDER)
				{
					CreateFolder(data.path);
				}
				else 
				{
					CreateFile(data.path);
				}
				
				struct stat fileStat;
				error_check(lstat(data.path, &fileStat), 11, "Lstat in traversal Error\n");

				if (!S_ISDIR(fileStat.st_mode))
				{
					int fd = open(data.path, O_WRONLY);
					error_check(fd, 4, "Open File Error\n");				

					WriteToFile(fd, sockfd);
				}
			}

		} else break;

	} while(1);

	while(!ListIsEmpty(head))
	{
		// Start deleting
		ListNode_t *nodeToDelete = GetItem(head);
		// printf("%s\n", nodeToDelete->value);
		char pathToDelete[MAX_PATH_SIZE];
		strcpy(pathToDelete, nodeToDelete->value);
		ListRemove(head, nodeToDelete->value);

		do 
		{
			int lastIndex = LastSlashIndex(pathToDelete);
			if (lastIndex == -1) {
				break;
			}

			struct stat fileStat;
			error_check(lstat(pathToDelete, &fileStat), 9, "Lstat error for given path\n");
			if (!S_ISDIR(fileStat.st_mode))
			{
				remove(pathToDelete);
			}
			else
			{
				printf("DIR: %s\n", pathToDelete);
				rmdir(pathToDelete);
			}

			pathToDelete[lastIndex] = '\0';

		} while(!ListSearch(headServer, pathToDelete));
	}


	FreeList(headServer);
	FreeList(head);
	printf("Folder is up to date\n");
}

void WriteToFile(int fd, int sockfd)
{
	char cmd = FMO_SEND_LAST_PATH_FILE_CONTENT;
	write(sockfd, &cmd, sizeof(char));
	int nread = -1;
	char buffer[READ_BLOCK_SIZE];
	do
	{
		int size = 0;
		nread = stream_read(sockfd, &size, sizeof(size));
		error_check(nread, 10, "Steram Read Error\n");
		if (size == 0)
		{
			break;
		}

		nread = stream_read(sockfd, buffer, size);
		error_check(nread, 10, "Steram Read Error\n");

		write(fd, buffer, nread);
	} while (nread > 0);

}

void CreateFolder(char *dataPath)
{
	char mkdirCmd[MAX_PATH_SIZE] = {0};
    strcat(mkdirCmd, "mkdir -p ");
    strcat(mkdirCmd, dataPath);

	system(mkdirCmd);
}

void CreateFile(char *dataPath)
{
	char * file = basename(dataPath);
	printf("%s\n", file);
    char dir[MAX_PATH_SIZE];
    strcpy(dir, dataPath);
    dir[strlen(dir)-strlen(file)-1] = '\0';

    printf("%s - %s - %s\n", dataPath, dir, file);

    char mkdirCmd[MAX_PATH_SIZE] = {0};
    strcat(mkdirCmd, "mkdir -p ");
    strcat(mkdirCmd, dir);

    char touchCmd[MAX_PATH_SIZE] = {0};
    strcat(touchCmd, "touch ");
    strcat(touchCmd, dir);
    strcat(touchCmd, "/");
    strcat(touchCmd, file);

    system(mkdirCmd);
	system(touchCmd);
}

void ListFilesRecursively(ListNode_t *head, char *basePath)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    int empty = 1;

    // Unable to open directory stream
    if (!dir) return;

	while ((dp = readdir(dir)) != NULL) {
		
        if (dp->d_type == DT_DIR) {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
                continue;
			empty = 0;
			strcpy(path, basePath);
			strcat(path, "/");
			strcat(path, dp->d_name);
            ListFilesRecursively(head, path);
        } 
		else 
		{
			empty = 0;
            strcpy(path, basePath);
			strcat(path, "/");
			strcat(path, dp->d_name);
			ListAdd(head, path);
		}
    }

	if(empty)
	{
		ListAdd(head, basePath);
	}

    closedir(dir);
}
int ValidateNumber(char str[])
{
	
	int i;
	int len=strlen(str);
	
	for(i=0;i<len;i++)
	{
		if(!isdigit(str[i]))
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
   
   if(strlen(ipAddress) > 17)
   {
	   return 0;
   }
   
   char buffer[IP_ADDR_BUFF_SIZE];
   
   strcpy(buffer,ipAddress);
   
   p=strtok(buffer, ".");
   if(p == NULL)
   {
	   return 0;
   }
   
   while(p)
   {
		if(!ValidateNumber(p))
		{
			return 0;
		}
		number = atoi(p);
		if(number >= 0 && number <= 255)
		{
			p=strtok(NULL, ".");
			if(p != NULL)
			{
				dots++;
			}
		}
		else 
		{
			return 0;
		}
   }
   
   if(dots != 3)
   {
	   return 0;
   }
   
   return 1;
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