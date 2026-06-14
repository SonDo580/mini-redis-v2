#include "common.hpp"

static int32_t send_req(int fd, const std::vector<std::string> &cmd);
static int32_t read_res(int fd);

int main(int argc, char **argv)
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

    // cmd + args
    std::vector<std::string> cmd;
    for (int i = 1; i < argc; i++)
        cmd.push_back(argv[i]);

    int32_t err = send_req(fd, cmd);
    if (err)
        goto L_DONE;

    err = read_res(fd);
    if (err)
        goto L_DONE;

L_DONE:
    close(fd);
    return 0;
}

static int32_t send_req(int fd, const std::vector<std::string> &cmd)
{
    // payload size
    uint32_t len = 4; // for nstr
    for (const std::string &s : cmd)
    {
        len += 4 + s.size(); // for (str_len + str)
        if (len > K_MAX_MSG)
            return -1;
    }

    std::vector<uint8_t> wbuf;
    buf_append(wbuf, (const uint8_t *)&len, 4);

    uint32_t n = (uint32_t)cmd.size(); // nstr
    buf_append(wbuf, (const uint8_t *)&n, 4);

    for (const std::string &s : cmd)
    {
        uint32_t str_len = (uint32_t)s.size();
        buf_append(wbuf, (const uint8_t *)&str_len, 4);
        buf_append(wbuf, (const uint8_t *)s.data(), s.size());
    }

    return writen(fd, wbuf.data(), wbuf.size());
}

static int32_t read_res(int fd)
{
    // payload size
    std::vector<uint8_t> rbuf;
    rbuf.resize(4);
    errno = 0;
    int32_t err = readn(fd, &rbuf[0], 4);
    if (err)
    {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4);
    if (len > K_MAX_MSG)
    {
        msg("too long");
        return -1;
    }
    if (len < 4)
    { // < res_code size
        msg("bad response");
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

    // print result
    uint32_t res_code = 0;
    memcpy(&res_code, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n",
           res_code, len - 4, &rbuf[8]);

    return 0;
}