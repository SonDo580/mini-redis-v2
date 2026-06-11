#include "common.hpp"

static int32_t query(int fd, const char *text)
{
    // === send request ===
    uint32_t len = (uint32_t)strlen(text);
    if (len > K_MAX_MSG)
        return -1;

    char wbuf[4 + K_MAX_MSG];
    memcpy(wbuf, &len, 4); // little-endian
    memcpy(&wbuf[4], text, len);

    int32_t err = writen(fd, wbuf, 4 + len);
    if (err)
        return err;

    // === read response ===
    char rbuf[4 + K_MAX_MSG];
    errno = 0;
    err = readn(fd, rbuf, 4);
    if (err)
    {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&len, rbuf, 4); // little-endian
    if (len > K_MAX_MSG)
    {
        msg("too long");
        return -1;
    }

    err = readn(fd, &rbuf[4], len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    printf("server says: %.*s\n", len, &rbuf[4]);
    return 0;
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        die("socket()");

    // connect to server
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1 (localhost)
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
        die("connect()");

    // make multiple requests
    int32_t err = query(fd, "hello1");
    if (err)
        goto L_DONE;
    err = query(fd, "hello2");
    if (err)
        goto L_DONE;
    err = query(fd, "hello3");
    if (err)
        goto L_DONE;

L_DONE:
    close(fd);
    return 0;
}