#include "common.hpp"

/* handle 1 request */
static int32_t one_request(int connfd)
{
    char rbuf[4 + K_MAX_MSG]; // 4-byte header + payload
    errno = 0;
    int32_t err = readn(connfd, rbuf, 4);
    if (err)
    {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    // payload size
    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // little-endian
    if (len > K_MAX_MSG)
    {
        msg("too long");
        return -1;
    }

    // payload
    err = readn(connfd, &rbuf[4], len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    // === dummy work ===

    printf("client says: %.*s\n", len, &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return writen(connfd, wbuf, 4 + len);
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0); // IPv4 + TCP
    if (fd < 0)
        die("socket()");

    // allow binding to the same `IP:port` after restart
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {}; // holds IPv4:port as big-endian numbers
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);     // port
    addr.sin_addr.s_addr = htonl(0); // 0.0.0.0 (to listen on all network interfaces)
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
        die("bind()");

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv)
        die("listen()");

    while (true)
    {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0)
            continue; // error -> wait for next client

        // only serve 1 client connection at once
        while (true)
        {
            int32_t err = one_request(connfd);
            if (err)
                break;
        }

        close(connfd);
    }

    return 0;
}
