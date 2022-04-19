#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>

#include "preprocessor_html.h"

#define ROOT_FILE "crust.html"
#define BACKLOG_SIZE 5

// TODO - specify content types properly (currently omitting so browser assumes)
// char header[] = "HTTP/1.1 200 OK\r\n"
//                 "Content-Type: text/html; charset=UTF-8\r\n";
char header[] = "HTTP/1.1 200 OK\r\n"
                "\r\n";
char end[] = "\r\n";

int main(int argc, char *argv[]) {
    int port;
    if (argc >= 2) port = atoi(argv[1]);
    else port = 8080;

    int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock_fd < 0) err(1, "Failed to open socket");

    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void *)1, sizeof(int));

    struct sockaddr_in6 svr_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = IN6ADDR_ANY_INIT,
        .sin6_port = htons(port)
    };
    socklen_t addr_size = sizeof(svr_addr);
    
    if (bind(sock_fd, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) < 0) {
        close(sock_fd);
        err(1, "Failed to bind socket to server address");
    }

    listen(sock_fd, BACKLOG_SIZE);

    struct sockaddr_in6 cli_addr;
    printf("Listening on port %d ...\n", port);
    while(1) {
        int cli_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &addr_size);

        char ip[30];
        inet_ntop(AF_INET6, &cli_addr.sin6_addr, ip, addr_size);
        printf("> Connection from %s : %d \n", ip, cli_addr.sin6_port);
        
        if (cli_fd == -1) {
            printf("> Failed to accept\n");
            continue;
        }

        int buf_size = 4096;
        char buf[buf_size];
        if (recv(cli_fd, buf, buf_size, 0) < 1) {
            printf("> Error receiving request from connection\n");
            continue;
        }

        strtok(buf, " ");
        char *url = strtok(NULL, " ");
        printf("=> %s\n", url);

        char *filepath;
        if (strcmp(url, "/") == 0) {
            // Requested root
            filepath = alloca(strlen(ROOT_FILE)-1);
            filepath = ROOT_FILE;
        } else {
            url++;
            filepath = url;
        }

        int fd = open(filepath, O_RDONLY);
        if (fd < 0) {
            printf("=> Failed to open requested file\n");
            continue;
        }
        int len = lseek(fd, 0, SEEK_END);
        if (len < 0) {
            printf("=> Failed lseek in requested file\n");
            continue;
        }
        char *data = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
        if ((void *) data == (void *) -1) {
            printf("=> Failed mmap of requested file\n");
            continue;
        }
        
        char *response = alloca(strlen(header) + len + strlen(end));
        memcpy(response, header, strlen(header));
        memcpy(&response[strlen(header)], data, len);
        memcpy(&response[strlen(header) + len], end, strlen(end));
        printf("==> %d\n", len);
        printf("--> %d %s \n", strlen(header) + len + strlen(end), response);
        printf("wrote %d bytes\n", send(cli_fd, response, strlen(header) + len + strlen(end), 0));
        // printf("%s\n\n", strerror(errno));
        close(cli_fd);
        if (munmap(data, len) < 0) {
            printf("=> Failed munmap of requested file\n");
            continue;
        }
    }

    close(sock_fd);
    return 0;
}