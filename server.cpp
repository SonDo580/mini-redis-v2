#include "common.hpp"

// dummy processing
static void do_something(int connfd)
{
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0)
    {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
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

        do_something(connfd);
        close(connfd);
    }

    return 0;
}
