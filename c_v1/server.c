#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

ssize_t read_line(int fd, char *buffer, size_t buf_size);

int main(int argc, char *argv[])
{
    uint32_t seq_num = 0;
    if (argc > 1) {
        seq_num = strtoul(argv[1], NULL, 10);
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        printf("Failed to ignore sigpipe errors\n\r");
        return -1;
    }
    printf("Server started with sequence number: %d\n\r", seq_num);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    struct addrinfo *result;
    if (getaddrinfo(NULL, "50000", &hints, &result) != 0) {
        printf("Failed to get available address info\n\r");
        return -1;
    }

    int lfd = -1; 
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1) {
            printf("Failed to open listening socket. Trying the next.\n\r");
            continue;
        }

        int optval = 0;
        if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
            printf("Failed to set socket options\n\r");
            return -1;
        }

        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            printf("Bound to socket.\n\r");
            break;
        }

        close(lfd);
    }

    if (lfd == -1) {
        printf("Failed to bind socket to any address.");
        return -1;
    }

    if (listen(lfd, 5) == -1) {
        printf("Failed to listen to socket\n\r");
        return -1;
    }

    freeaddrinfo(result);

    while (1) {
        printf("Waiting for data\n\r");
        int addr_len = sizeof(struct sockaddr_storage);
        struct sockaddr_storage claddr;

        const int cfd = accept(lfd, (struct sockaddr*)&claddr, &addr_len);
        if (cfd == -1) {
            printf("Failed to accept connection\n\r");
            continue;
        }

        char host[NI_MAXHOST];
        char service[NI_MAXSERV];
        char addr_str[NI_MAXHOST + NI_MAXSERV + 10];
        if (getnameinfo((struct sockaddr*)&claddr, addr_len, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
            snprintf(addr_str, sizeof(addr_str), "(%s, %s)", host, service);
        }
        else {
            snprintf(addr_str, sizeof(addr_str), "(?UNKNOWN?)");
        }
        printf("Connection from %s\n\r", addr_str);

        char rd_buffer[256];
        ssize_t rd_length = read_line(cfd, rd_buffer, sizeof(rd_buffer));

        if (rd_length > 0) {
            printf("Received data: %s\n\r", rd_buffer);
            if (write(cfd, "OK", 3) != 3) {
                printf("Reply write failed\n\r");
            }
        }
        else {
            printf("Read failed\n\r");
        }

        if (close(cfd) == -1) {
            printf("Failed to close client connection\n\r");
            return -1;
        }

        printf("Closed connection\n\r");
    }
    return 0;
}

ssize_t read_line(int fd, char *buffer, size_t buf_size)
{
    ssize_t rd_len = 0;

    while (1) {
        char val;
        ssize_t bytes_rd = read(fd, &val, 1);

        if (bytes_rd == -1) {
            return 0;
        }
        else if (bytes_rd == 0) {
            break;
        }
        else {
            buffer[rd_len] = val;
        }

        rd_len++;
        if (val == '\n' || rd_len == buf_size - 1) {
            break;
        }
    }

    buffer[rd_len] = '\0';

    return rd_len;
}