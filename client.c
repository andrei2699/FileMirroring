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
void CreateFile(char *path);

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
	ListNode_t *head = (ListNode_t *)malloc(sizeof(ListNode_t));
	InitList(head);
	ListFilesRecursively(head, argv[3]);
	ListPrint(head);

    do
    {
        char cmd = FMO_SEND_NEXT_PATH;
        write(sockfd, &cmd, sizeof(char));
        recv(sockfd, &data, sizeof(PathData_t), 0);

		//printf("%s %d\n", data.path, i);
        if (strcmp(data.path, "") != 0) {
        	if (ListSearch(head, data.path))
			{
				struct stat fileStat;
				error_check(lstat(data.path, &fileStat), 9, "Lstat error for given path\n");
				if (fileStat.st_mtime != data.lastModifiedTime) 
				{
					// cerem continut
				}
			} else {
				CreateFile(data.path);
		    
				FILE *fp;
				fp  = fopen (data.path, "w");
				if(!fp) {
					printf("File open error\n");
					exit(10);
				}
				
			}

	        // TODO 1 : check if path exists
	            // if it does: go to TODO 2
	            // else: create file nad ask for file content

	        // TODO 2 : check the date of last modification on the file and compare it to the copy on the client computer
	            // if == move on
	            // else: ask for file content and overwrite the content of the file
		} else break;
		

	} while(1);

}

void CreateFile(char *dataPath)
{
	char * file = basename(data.path);
    char * dir = (char *) malloc(sizeof(char) * MAX_PATH_SIZE);//dirname(data.path);
    strcpy(dir, data.path);
    dir[strlen(dir)-strlen(file)-1] = '\0';

    printf("%s - %s - %s\n", data.path, dir, file);

    char mkdirCmd[ 80 ] = { 0 };
    strcat( mkdirCmd, "mkdir -p " );
    strcat( mkdirCmd, dir );

    char touchCmd[ 80 ] = { 0 };
    strcat( touchCmd, "touch " );
    strcat( touchCmd, dir );
    strcat( touchCmd, "/" );
    strcat( touchCmd, file );

    system( mkdirCmd );
	system( touchCmd );
}

void ListFilesRecursively(ListNode_t *head, char *basePath)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    // Unable to open directory stream
    if (!dir) return;

    while ((dp = readdir(dir)) != NULL)
    {
		if (dp->d_type != DT_DIR) {
			strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
			ListAdd(head, path);
		}
	
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            // printf("%s/%s\n", basePath, dp->d_name);
            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);

			ListFilesRecursively(head, path);
        }
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