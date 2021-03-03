#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>

#include "utils.h"

#define MAX_PATH_SIZE 256
#define SERVER_PORT 5678

typedef enum
{
	FT_NONE = 0,
	FT_FILE,
	FT_FOLDER,
} FileType_t;

typedef struct
{
	char path[MAX_PATH_SIZE];
	time_t lastModifiedTime;
	FileType_t fileType;
} PathData_t;

typedef enum
{
	FMO_SEND_NEXT_PATH = 1,
	FMO_SEND_LAST_PATH_FILE_CONTENT,
} FileMirroringOperations_t;

PathData_t data;

int main()
{
	int client_socket;
	int connection;

	client_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	connection = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
	if (connection < 0)
	{
		printf("There was an error making the connection to the server!\n");
		exit(1);
	}

	/*
	char server_response[256];
	recv(client_socket,&server_response, sizeof(server_response), 0);
	
	printf("The server sent the following message: %s\n", server_response);
	*/
	char c = FMO_SEND_NEXT_PATH;

	do
	{
		scanf("%c", &c);

		switch (c)
		{
		case 'a':
		{
			c = FMO_SEND_NEXT_PATH;
			write(client_socket, &c, sizeof(char));
			recv(client_socket, &data, sizeof(PathData_t), 0);

			printf("%s %d\n", data.path, data.fileType);
		}
		break;

		case 'b':
		{
			c = FMO_SEND_LAST_PATH_FILE_CONTENT;
			write(client_socket, &c, sizeof(char));
			printf("Reading content...\n");
			int nread = -1;
			char buffer[4096];

			do
			{
				int size = 0;
				nread = stream_read(client_socket, &size, sizeof(size));
				if (nread < 0)
				{
					perror("Read Error\n");
				}
				if (size == 0)
				{
					break;
				}
				nread = stream_read(client_socket, buffer, size);
				for (int i = 0; i < nread; i++)
				{
					printf("%c", buffer[i]);
				}
				if (nread < 0)
				{
					perror("Read Error\n");
				}
			} while (nread > 0);
			printf("\n");

			printf("Content Read\n");
		}
		break;

		default:
			continue;
			break;
		}

	} while (strcmp(data.path, "") != 0);

	close(connection);
	close(client_socket);
	return 0;
}