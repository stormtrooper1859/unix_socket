#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: client <port>\n");
        exit(EXIT_FAILURE);
    }

    const int server_port = atoi(argv[1]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bn = bind(sock, (const struct sockaddr *) &addr, sizeof(addr));
    if (bn == -1) {
        perror("bind");
    }
    if (listen(sock, 1) == -1) {
        perror("listen");
    }

    struct sockaddr ac_addr;
    socklen_t ac_addrlen;
    while (1) {
        int ac_sock = accept(sock, &ac_addr, &ac_addrlen);
        char cb[100];
        ssize_t r = recv(ac_sock, cb, 100, 0);
        ssize_t s = send(ac_sock, cb, 100, 0);
        shutdown(ac_sock, SHUT_RDWR);
        close(ac_sock);
    }
}
