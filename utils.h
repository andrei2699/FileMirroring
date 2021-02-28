#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
#include <netdb.h>

int error_check(int errorCode, int exitCode, const char *message);
int set_addr(struct sockaddr_in *addr, char *name, uint32_t inaddr, short sin_port);
int stream_read(int socketfd, char *buffer, int len);
int stream_write(int socketfd, char *buffer, int len);

#endif // UTILS_H