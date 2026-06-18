// stdlib
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
// system
#include <unistd.h>

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
void buf_append(Buffer &buf, const uint8_t *data, size_t len)
{
    buf.insert(buf.end(), data, data + len);
}

/* remove n bytes from front of buf. */
void buf_consume(Buffer &buf, size_t n)
{
    buf.erase(buf.begin(), buf.begin() + n);
}
