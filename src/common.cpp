// stdlib
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
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
    if (len == 0)
        return;

    size_t free_space_at_back = buf.end - buf.data_end;

    if (len > free_space_at_back)
    {
        size_t data_size = buf.data_end - buf.data_begin;
        size_t total_capacity = buf.end - buf.begin;
        size_t total_free_space = total_capacity - data_size;

        if (len <= total_free_space)
        { // enough space but fragmented -> shift data to front
            memmove(buf.begin, buf.data_begin, data_size);
            buf.data_begin = buf.begin;
            buf.data_end = buf.begin + data_size;
        }
        else
        { // not enough space even when shifting
            size_t new_capacity =
                (total_capacity == 0) ? 64 : total_capacity * 2;
            while (new_capacity < data_size + len)
                new_capacity *= 2;

            size_t free_space_at_front = buf.data_begin - buf.begin;
            buf.begin = (uint8_t *)realloc(buf.begin, new_capacity);
            assert(buf.begin != NULL);

            // re-align pointers
            buf.data_begin = buf.begin + free_space_at_front;
            buf.data_end = buf.begin + free_space_at_front + data_size;
            buf.end = buf.begin + new_capacity;
        }
    }

    // copy data and advance buf.data_end pointer
    if (data != NULL)
        memcpy(buf.data_end, data, len);
    buf.data_end += len;
}

/* remove n bytes from front of buf. */
void buf_consume(Buffer &buf, size_t n)
{
    size_t data_size = buf.data_end - buf.data_begin;
    assert(n <= data_size);

    if (n == data_size)
    { // reset both data pointers to buf.begin (maximize write space)
        buf.data_begin = buf.data_end = buf.begin;
    }
    else
    { // only advance buf.data_begin pointer, don't need to move data
        buf.data_begin += n;
    }
}
