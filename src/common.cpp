#include "common.hpp"

/* print error message */
void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

/* print error message and error code */
void msg_errno(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[errno:%d] %s\n", err, msg);
}

/* print system error code and abort */
void die(const char *msg)
{
    msg_errno(msg);
    abort();
}

/* set a file descriptor to non-blocking mode. */
void fd_set_nb(int fd)
{
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno)
    {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    fcntl(fd, F_SETFL, flags);
    if (errno)
        die("fcntl error");
}

/*
read exactly n bytes from fd into buf;
return 0 on success, -1 on error or unexpected EOF.
*/
int32_t readn(int fd, uint8_t *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            if (errno == EINTR) // signal interrupt occurred
                continue;       // safely retry
            else
                return -1; // error or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

/*
write exactly n bytes from buf to fd;
return 0 on success, -1 on error.
*/
int32_t writen(int fd, const uint8_t *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
            return -1; // error
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

/* append len bytes to back of buf. */
void buf_append(
    std::vector<uint8_t> &buf, const uint8_t *data, size_t len)
{
    buf.insert(buf.end(), data, data + len);
}

/* remove n bytes from front of buf. */
void buf_consume(std::vector<uint8_t> &buf, size_t n)
{
    buf.erase(buf.begin(), buf.begin() + n);
}