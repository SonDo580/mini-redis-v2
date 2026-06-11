#include "common.hpp"

/* print error message */
void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

/* print system error code and abort */
void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

/*
read exactly n bytes from fd into buf;
return 0 on success, -1 on error or unexpected EOF.
*/
int32_t readn(int fd, char *buf, size_t n)
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
int32_t writen(int fd, const char *buf, size_t n)
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
