#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFLEN 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: client <port>\n");
        exit(EXIT_FAILURE);
    }

    const int server_port = atoi(argv[1]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);


    int cn = connect(sock, (const struct sockaddr *) &addr, sizeof(addr));
    if (cn == -1) {
        perror("connect");
    }

    char buf[BUFLEN];
    scanf("%s", buf);

    ssize_t sended = 0;
    ssize_t t = 0;
    if((t = write(sock, &(buf[sended]), BUFLEN - sended)) > 0){
        sended += t;
    }
    if(t == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }

    memset(buf, 0, BUFLEN);

    ssize_t readed = 0;
    if((t = read(sock, &(buf[readed]), BUFLEN - readed)) > 0){
        readed += t;
    }
    if(t == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }

    recv(sock, buf, sizeof(buf), 0);
    printf("%s\n", buf);
    close(sock);
}
