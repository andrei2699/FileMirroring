#include "utils.h"

int error_check(int errorCode, int exitCode, const char *message)
{
    if (errorCode < 0)
    {
        perror(message);
        exit(exitCode);
    }
    return errorCode;
}

int set_addr(struct sockaddr_in *addr, char *name, uint32_t inaddr, short sin_port)
{
    struct hostent *h;
    memset((void *)addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    if (name != NULL)
    {
        h = gethostbyname(name);
        if (h == NULL)
            return -1;
        addr->sin_addr.s_addr = *(uint32_t *)h->h_addr_list[0];
    }
    else
        addr->sin_addr.s_addr = htonl(inaddr);
    addr->sin_port = htons(sin_port);
    return 0;
}

int stream_read(int socketfd, char *buffer, int len)
{
    int nread;
    int remaining = len;
    while (remaining > 0)
    {
        nread = read(socketfd, buffer, remaining);
        if (nread == -1)
            return -1;
        if (nread == 0)
            break;
        remaining -= nread;
        buffer += nread;
    }

    return len - remaining;
}

int stream_write(int socketfd, char *buffer, int len)
{
    int nwrite;
    int remaining = len;
    while (remaining > 0)
    {
        nwrite = write(socketfd, buffer, remaining);
        if (nwrite == -1)
            return -1;
        remaining -= nwrite;
        buffer += nwrite;
    }

    return len - remaining;
}