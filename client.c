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

#define IP_ADDR_BUFF_SIZE 20
#define READ_BLOCK_SIZE 4096
#define MAX_PATH_SIZE 256

PathData_t data;

int ValidateNumber(char str[]);
int ValidateIpAddress(char *ipAddress);

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

    do
    {
        char cmd = FMO_SEND_NEXT_PATH;
        write(sockfd, &cmd, sizeof(char));

        recv(sockfd, &data, sizeof(PathData_t), 0);
        printf("%s %d\n", data.path, data.fileType);

        // TODO 1 : check if path exists
            // if it does: go to TODO 2
            // else: create file nad ask for file content

        // TODO 2 : check the date of last modification on the file and compare it to the copy on the client computer
            // if == move on
            // else: ask for file content and overwrite the content of the file

    } while(strcmp(data.path, "") != 0);

}

int ValidateNumber(char str[]) {
	
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