#include "common.hpp"

static int32_t client_tx(int fd, const std::vector<std::string> &cmd);

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

    // send request + read response + print response
    client_tx(fd, cmd);

    close(fd);
    return 0;
}

static int32_t send_req(int fd, const std::vector<std::string> &cmd);
static int32_t read_res(int fd, Buffer &rbuf);
static int32_t print_res(const uint8_t *data, size_t size);

/* send request + read response + print response;
   return 0 on success; -1 on failure. */
static int32_t client_tx(int fd, const std::vector<std::string> &cmd)
{
    // send request
    int32_t err = send_req(fd, cmd);
    if (err)
        return err;

    // read response
    Buffer rbuf;
    err = read_res(fd, rbuf);
    if (err)
        return err;

    // print response
    uint32_t payload_size = rbuf.size() - 4;
    int32_t consumed = print_res(&rbuf[4], payload_size);
    if (consumed < 0 || (uint32_t)consumed != payload_size)
    {
        msg("bad response");
        return -1;
    }

    return 0;
}

/* return 0 on success; -1 on failure. */
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

    Buffer wbuf;
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

/* return 0 on success; -1 on failure. */
static int32_t read_res(int fd, Buffer &rbuf)
{
    // payload size
    rbuf.resize(4);
    errno = 0;
    int32_t err = readn(fd, &rbuf[0], 4);
    if (err)
    {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len;
    memcpy(&len, rbuf.data(), 4);
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

    return 0;
}

/* print response; return number of bytes consumed, or -1 on error. */
static int32_t print_res(const uint8_t *data, size_t size)
{
    if (size < 1) // < tag size
        return -1;

    switch (data[0])
    {
    case TAG_NIL:
        printf("(nil)\n");
        return 1;
    case TAG_ERR:
    {
        if (size < 1 + 8) // < (tag + code + msg_size) size
            return -1;
        uint32_t code;
        uint32_t msg_len;
        memcpy(&code, &data[1], 4);
        memcpy(&msg_len, &data[1 + 4], 4);

        if (size < 1 + 8 + msg_len)
            return -1;
        printf("(err) %d %.*s\n", code, msg_len, &data[1 + 8]);
        return 1 + 8 + msg_len;
    }
    case TAG_STR:
    {
        if (size < 1 + 4) // (tag + str_len) size
            return -1;
        uint32_t str_len;
        memcpy(&str_len, &data[1], 4);

        if (size < 1 + 4 + str_len)
            return -1;
        printf("(str) %.*s\n", str_len, &data[1 + 4]);
        return 1 + 4 + str_len;
    }
    case TAG_INT:
    {
        if (size < 1 + 8) // (tag + int64) size
            return -1;
        int64_t val;
        memcpy(&val, &data[1], 8);
        printf("(int) %ld\n", val);
        return 1 + 8;
    }
    case TAG_DBL:
    {
        if (size < 1 + 8) // (tag + double) size
            return -1;
        double val;
        memcpy(&val, &data[1], 8);
        printf("(dbl) %g\n", val);
        return 1 + 8;
    }
    case TAG_ARR:
    {
        if (size < 1 + 4) // (tag + arr_size) size
            return -1;
        uint32_t len;
        memcpy(&len, &data[1], 4);
        printf("(arr) len=%u\n", len);

        // print items
        size_t offset = 1 + 4; // offset of next item in data buffer
        for (uint32_t i = 0; i < len; i++)
        {
            int32_t rv = print_res(&data[offset], size - offset);
            if (rv < 0)
                return rv;
            offset += (size_t)rv;
        }

        printf("(arr) end\n");
        return (int32_t)offset;
    }
    default: // unknown type
        return -1;
    }
}