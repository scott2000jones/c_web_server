#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

#include "preprocessor_html.h"

#define BACKLOG_SIZE 5

char response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
DOCTYPE("html")
TAG("html",
    TAG("head",
        TAG("title", "C server")
        TAG("style", "body { font-family: sans-serif; color: fuchsia; }\n")
    )
    TAG("body",
        TAG("h1", "Server says hello!\n")
    )
)"\r\n";

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
    while(1) {
        printf("Listening on port %d...\n", port);
        int cli_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &addr_size);

        char ip[30];
        inet_ntop(AF_INET6, &cli_addr.sin6_addr, ip, addr_size);
        printf("> Connection from %s, %d\n", ip, cli_addr.sin6_port);
        
        if (cli_fd == -1) {
            perror("> Failed to accept");
            continue;
        }

        write(cli_fd, response, sizeof(response)-1); /*-1:'\0'*/
        close(cli_fd);
    }

    close(sock_fd);
    return 0;
}