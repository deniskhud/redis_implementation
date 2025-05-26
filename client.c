#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

static void msg(const char* msg) {
    fprintf
}

static void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

int main() {
    int fd, rv;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die("socket");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  //127.0.0.1
    if ((rv = connect(fd, (const struct sockaddr*)&addr, sizeof addr)) < 0) {
        die("connect");
    }

    //send message
    char msg[] = "hello";
    send(fd, msg, strlen(msg), 0);


    //requvied message
    char rbuf[64] = {};

    ssize_t n = recv(fd, rbuf, sizeof(rbuf), 0);
    if (n < 0) {
        die("read");
    }

    printf("server says: %s\n", rbuf);
    close(fd);

    return 0;
}