#include "common.hpp"

static int32_t send_req(int fd, const uint8_t *text, size_t len);
static int32_t read_res(int fd);

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

    // multiple pipedline requests
    std::vector<std::string> query_list = {
        "hello1", "hello2", "hello3",
        std::string(K_MAX_MSG, 'z'), // large message (requires multiple event loop iterations)
        "hello5"};

    // send requests
    for (const std::string &s : query_list)
    {
        int32_t err = send_req(fd, (const uint8_t *)s.data(), s.size());
        if (err)
            goto L_DONE;
    }

    // read responses
    for (size_t i = 0; i < query_list.size(); i++)
    {
        int32_t err = read_res(fd);
        if (err)
            goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
}

static int32_t send_req(int fd, const uint8_t *text, size_t len)
{
    if (len > K_MAX_MSG)
        return -1;

    std::vector<uint8_t> wbuf;
    buf_append(wbuf, (const uint8_t *)&len, 4); // little-endian
    buf_append(wbuf, text, len);
    return writen(fd, wbuf.data(), wbuf.size());
}

static int32_t read_res(int fd)
{
    // 4-byte header
    std::vector<uint8_t> rbuf;
    rbuf.resize(4);
    errno = 0;
    int32_t err = readn(fd, &rbuf[0], 4);
    if (err)
    {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    // payload size
    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4); // little-endian
    if (len > K_MAX_MSG)
    {
        msg("too long");
        return -1;
    }

    // payload
    rbuf.resize(4 + len);
    err = readn(fd, &rbuf[4], len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    // logging
    printf("server says: len:%u data:%.*s%s\n",
           len, (len < 100 ? len : 100), &rbuf[4], (len > 100 ? "..." : ""));

    return 0;
}