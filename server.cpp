#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

//defines
const size_t k_max_msg = 4096;

static void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = recv(connfd, rbuf, sizeof(rbuf) - 1, 0);
    if (n < 0) {
        msg("read() error");
        return;
    }
    fprintf(stderr, "client says: %s\n", rbuf);
    char wbuf[] = "word";
    send(connfd, wbuf, strlen(wbuf), 0);
}

static int32_t read_full(int fd, char* buf, size_t len) {
    while (len > 0) {
        ssize_t rv = recv(fd, buf, len, 0);
        if (rv <= 0) return -1; //error or unexpected EOF
        assert((size_t)rv <= len);
        len -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_full(int fd, const char* buf, size_t len) {
    while (len > 0) {
        ssize_t rv = send(fd, buf, len, 0);
        if (rv <= 0) return -1; //error

        assert((size_t)rv <= len);
        len -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

//read 1 request and send 1 response
static int32_t one_request(int connfd) {
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);   //assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }
    //request body

    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    //do something
    printf("client says: %.*s\n",len, &rbuf[4]);

    //reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_full(connfd, wbuf, 4 + len);


}


int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket");

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);
    int rv = bind(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (rv) die("bind");

    rv = listen(fd, SOMAXCONN);
    if (rv) die("listen");

    while (1) {
        //accept
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (connfd < 0) {continue;}

        while (1) {
            int32_t err = one_request(connfd);
            if (err) break;
        }

        close(connfd);
    }
    return 0;
}