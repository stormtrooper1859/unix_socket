#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cerrno>
#include <queue>
#include <string>


#define MSG_SIZE 2048
#define error_msg(msg) do{perror(msg); exit(EXIT_FAILURE);}while(0);


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: client <port>\n");
        exit(EXIT_FAILURE);
    }

    const int server_port = atoi(argv[1]);

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
        error_msg("socket");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int ep = epoll_create(2);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
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

    if (connect(sock, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        if (errno != EINPROGRESS) {
            error_msg("connect");
        }
    }

    char welcome[18] = "Enter your name: ";
    write(STDERR_FILENO, welcome, 18);

    std::queue<std::string> msg_queue;

    struct epoll_event events[2];
    char text[MSG_SIZE];

    while (1) {
        int nfds = epoll_wait(ep, events, 2, -1);
        if (nfds == -1) {
            error_msg("epoll_wait");
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == STDIN_FILENO) {
                memset(text, 0, MSG_SIZE);

                ssize_t readed = read(STDIN_FILENO, text, MSG_SIZE);
                if (readed == -1) {
                    perror("read");
                }

                text[strlen(text) - 1] = 0;

                if (msg_queue.empty()) {
                    ev.events |= EPOLLOUT;
                    ev.data.fd = sock;
                    if (epoll_ctl(ep, EPOLL_CTL_MOD, sock, &ev) == -1) {
                        error_msg("epoll_ctl_mod sock");
                    }
                }
                msg_queue.push(std::string(text));

            } else if (events[i].data.fd == sock) {

                if (events[i].events & EPOLLRDHUP) {
                    printf("Server has been shut down\n");
                    exit(EXIT_SUCCESS);
                } else if (events[i].events & EPOLLIN) {

                    char cb[MSG_SIZE];
                    memset(cb, 0, MSG_SIZE);

                    ssize_t readed = 0;
                    ssize_t t = 0;

                    if ((t = read(sock, &(cb[readed]), MSG_SIZE - 1 - readed)) > 0) {
                        readed += t;
                    }

                    printf("%s\n", cb);

                } else if (events[i].events & EPOLLOUT) {
                    const char *str = msg_queue.front().data();
                    ssize_t len = msg_queue.front().size();

                    ssize_t sended = 0;
                    ssize_t t = 0;
                    if ((t = write(sock, &(str[sended]), len - sended)) > 0) {
                        sended += t;
                    }

                    if (len != sended) {
                        msg_queue.front() = msg_queue.front().substr(sended);
                    } else {
                        msg_queue.pop();
                    }

                    if (msg_queue.empty()) {
                        ev.events &= ~EPOLLOUT;
                        ev.data.fd = sock;
                        if (epoll_ctl(ep, EPOLL_CTL_MOD, sock, &ev) == -1) {
                            error_msg("epoll_ctl_mod sock");
                        }
                    }
                }
            }
        }
    }
}

