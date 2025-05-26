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

static void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}


static int32_t read_full(int fd, char* buf, size_t len) {
    while (len > 0) {
        ssize_t rv = recv(fd, buf, len, 0);
        if (rv <= 0) return -1;
        assert((size_t)rv <= len);

        len -= (size_t)rv;
        buf += rv;
    }
    return 0;

}

static int32_t write_full(int fd, const char* buf, size_t len) {
    while (len > 0) {
        ssize_t rv = send(fd, buf, len, 0);
        if (rv <= 0) return -1;
        assert((size_t)rv <= len);
        len -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096;

static int32_t query(int fd, const char* text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  //assume little endian
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_full(fd, wbuf, 4 + len)) return err;

    //4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&len, rbuf, 4);  //assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    //reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    //do something
    printf("server says: %.*s\n",len, &rbuf[4]);
    return 0;

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

    //multiple requests
    int32_t err = query(fd, (const char*)"hello");
    if (err) goto L_DONE;

    err = query(fd, (const char*)"hello2");
    if (err) goto L_DONE;

    err = query(fd, (const char*)"hello3");
    if (err) goto L_DONE;

L_DONE:
    close(fd);
    return 0;
}