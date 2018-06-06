#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <string>
#include <iostream>

#define error_msg(msg) do{perror(msg); exit(EXIT_FAILURE);}while(0);
#define MAX_EVENTS 100
#define READ_BUFFER_SIZE 1024

struct epoll_event events[MAX_EVENTS];

struct userT {
    int state;
    std::string sended_message;
    int position;
    std::queue<std::string> messages_to_send;
    std::string name;
    bool available_to_read;

    userT() : state(0), sended_message(""), position(0), name(""), available_to_read(false) {}
};

int sock;

void sig_handler(int signo) {
    close(sock);
    printf("\nexit\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("signal");
    }

    if (argc != 2) {
        printf("Usage: client <port>\n");
        exit(EXIT_FAILURE);
    }

    const int server_port = std::stoi(argv[1]);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        error_msg("bind");
    }

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    if (listen(sock, 1) == -1) {
        error_msg("listen");
    }

    int efd = epoll_create(MAX_EVENTS);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    sockaddr ac_addr;
    socklen_t ac_addrlen = 0;
    memset(&ac_addr, 0, sizeof(ac_addr));

    std::unordered_map<int, userT> users;

    while (true) {
        int nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        std::string new_messages;
        for (int i = 0; i < nfds; ++i) {
            std::string msg;
            int cur_fd = events[i].data.fd;

            if (cur_fd == sock) {
                int conn_sock = accept(sock, &ac_addr, &ac_addrlen);

                if (conn_sock == -1) {
                    error_msg("accept");
                }

                int flags = fcntl(conn_sock, F_GETFL, 0);
                fcntl(conn_sock, F_SETFL, flags | O_NONBLOCK);

                epoll_event ev2;
                ev2.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLERR;
                ev2.data.fd = conn_sock;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, conn_sock, &ev2) == -1) {
                    error_msg("epoll_ctl_add");
                }

                userT u;
                users.insert(std::make_pair(conn_sock, std::move(u)));
            } else {
                userT &user = users[cur_fd];
                if (events[i].events & EPOLLRDHUP) {
                    printf("%s disconnected\n", user.name.data());
                    new_messages.append(user.name + " disconnected");
                    users.erase(cur_fd);
                    epoll_ctl(efd, EPOLL_CTL_DEL, cur_fd, nullptr);
                } else {
                    users[cur_fd].available_to_read =
                            (events[i].events & EPOLLOUT) && !(events[i].events & EPOLLRDHUP);
                    if (events[i].events & EPOLLIN) {
                        char cb[READ_BUFFER_SIZE];
                        memset(cb, 0, READ_BUFFER_SIZE);

                        while (read(cur_fd, cb, sizeof(cb)) != -1) {
                            msg.append(cb);
                            memset(cb, 0, READ_BUFFER_SIZE);
                        }

                        msg.shrink_to_fit();

                        if (user.state == 0) {
                            user.name = msg;
                            user.state++;
                            new_messages.append(user.name + " connected");
                            printf("%s connected\n", user.name.data());
                        } else {
                            new_messages.append(user.name + ": " + msg);
                        }
                    }
                }
            }
        }

        for (auto &s: users) {
            s.second.messages_to_send.push(new_messages);
            if (s.second.available_to_read) {
                userT &user = s.second;

                bool isOk = true;
                while (isOk) {
                    if (user.position == user.sended_message.size()) {
                        if (user.messages_to_send.empty()) {
                            break;
                        } else {
                            user.position = 0;
                            user.sended_message = user.messages_to_send.front();
                            user.messages_to_send.pop();
                            continue;
                        }
                    }

                    ssize_t rez = write(s.first, &(user.sended_message.data()[user.position]),
                                        user.sended_message.size() - user.position);

                    if (rez == -1 || rez == 0) {
                        break;
                    } else {
                        user.position += rez;
                    }
                }
            }
        }
    }
}
