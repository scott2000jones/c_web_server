#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <threads.h>
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
//                 "Content-Type: text/html; charset=UTF-8\r\n";
char header[] = "HTTP/1.1 200 OK\r\n"
                "\r\n";
char end[] = "\r\n";

int serve(void *cli_fd_p) {
    int cli_fd = (int) cli_fd_p;
    int buf_size = 4096;
    char buf[buf_size];
    if (recv(cli_fd, buf, buf_size, 0) < 1) {
        printf("> Error receiving request from connection\n");
        return -1;
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
        return -1;
    }
    int len = lseek(fd, 0, SEEK_END);
    if (len < 0) {
        printf("=> Failed lseek in requested file\n");
        return -1;
    }
    char *data = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if ((void *) data == (void *) -1) {
        printf("=> Failed mmap of requested file\n");
        return -1;
    }
    
    char *response = alloca(strlen(header) + len + strlen(end));
    memcpy(response, header, strlen(header));
    memcpy(&response[strlen(header)], data, len);
    memcpy(&response[strlen(header) + len], end, strlen(end));
    printf("Wrote %d bytes\n", send(cli_fd, response, strlen(header) + len + strlen(end), 0));
    close(cli_fd);
    if (munmap(data, len) < 0) printf("=> Failed munmap of requested file\n");
    
    return 0;
}

struct server {
    int port;
    int sock_fd;
    struct sockaddr_in6 svr_addr;
    struct sockaddr_in6 cli_addr;
    socklen_t addr_size;
    void (*listen)(struct server *);
    void (*destroy)(struct server *);
};

void server_listen(struct server *this) {
    printf("Listening on port %d ...\n", this->port);
    while(1) {
        int cli_fd = accept(this->sock_fd, (struct sockaddr *) &(this->cli_addr), &(this->addr_size));

        char ip[30];
        inet_ntop(AF_INET6, &(this->cli_addr.sin6_addr), ip, this->addr_size);
        printf("> Connection from %s : %d \n", ip, this->cli_addr.sin6_port);
        
        if (cli_fd == -1) {
            printf("> Failed to accept\n");
            continue;
        }

        thrd_t thread;
        if (thrd_create(&thread, &serve, (void *) cli_fd) != thrd_success) {
            printf("> Failed to create C11 thread\n");
            continue;
        }
        if (thrd_detach(thread) != thrd_success) printf("> Failed to detach C11 thread\n");
    }
}

void server_destroy(struct server *this) {
    close(this->sock_fd);
    free(this);
}

struct server *create_server(int port) {
    struct server *new_server = malloc(sizeof(struct server));
    new_server->port = port;
    new_server->sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (new_server->sock_fd < 0) err(1, "Failed to open socket");
    setsockopt(new_server->sock_fd, SOL_SOCKET, SO_REUSEADDR, (void *)1, sizeof(int));
    new_server->svr_addr = (struct sockaddr_in6) {
        .sin6_family = AF_INET6,
        .sin6_addr = IN6ADDR_ANY_INIT,
        .sin6_port = htons(port)
    };
    new_server->addr_size = sizeof(new_server->svr_addr);
    if (bind(new_server->sock_fd, (struct sockaddr *) &(new_server->svr_addr), sizeof(new_server->svr_addr)) < 0) {
        close(new_server->sock_fd);
        err(1, "Failed to bind socket to server address");
    }
    if (listen(new_server->sock_fd, BACKLOG_SIZE) < 0) err(1, "Failed to set socket to passive with listen()");
    new_server->listen = &server_listen;
    new_server->destroy = &server_destroy;
    return new_server;
}


int main(int argc, char *argv[]) {
    int port;
    if (argc >= 2) port = atoi(argv[1]);
    else port = 8080;

    struct server *s = create_server(port);
    s->listen(s);
    s->destroy(s);
    return 0;
}