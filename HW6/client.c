#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

#define MSG_SIZE 2048
#define error_msg(msg) do{perror(msg); exit(EXIT_FAILURE);}while(0);


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: client <port>\n");
        exit(EXIT_FAILURE);
    }

    const int server_port = atoi(argv[1]);

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error_msg("socket");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int ep = epoll_create(2);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR | EPOLLET;
    ev.data.fd = sock;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, sock, &ev) == -1) {
        error_msg("epoll_ctl sock");
    }

    struct epoll_event ev2;
    ev2.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    ev2.data.fd = STDIN_FILENO;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, STDIN_FILENO, &ev2) == -1) {
        error_msg("epoll_ctl stdin");
    }

    char name[100];
    printf("Enter your name: ");
    scanf("%s", name);

    if (connect(sock, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        error_msg("connect");
    }

    struct epoll_event events[2];
    char text[MSG_SIZE];

    write(sock, name, sizeof(name));

    while (1) {
        int nfds = epoll_wait(ep, events, 2, -1);
        if (nfds == -1) {
            error_msg("epoll_wait");
        }
        for (int i = 0; i < nfds; ++i) {
//            printf("connection %d event %d\n", events[i].data.fd, events[i].events);
            if (events[i].data.fd == STDIN_FILENO) {
                memset(text, 0, MSG_SIZE);
                ssize_t readed = read(STDIN_FILENO, text, MSG_SIZE);
                if (readed == -1) {
                    perror("read");
                }
                if (write(sock, text, readed) == -1) {
                    perror("send");
                }
            } else if (events[i].data.fd == sock) {

                if (events[i].events & EPOLLRDHUP) {
                    printf("Server has been shut down\n");
                    exit(EXIT_SUCCESS);
                }

                char cb[64];
                memset(cb, 0, 64);

                ssize_t r = read(sock, cb, 64);
                if (r == -1) {
                    perror("read");
                }

//                putchar(cb[0]);


//                printf("%d\n", cb[0]);
//
//                printf("got %ld %s\n", r, cb);
                printf("%s", cb);
            }
        }
    }
}

