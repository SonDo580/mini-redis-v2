// stdlib
#include <assert.h>
#include <string.h>
#include <math.h>
// system
#include <fcntl.h>

#include "server_utils.hpp"

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

/* FNV hash */
uint64_t str_hash(const uint8_t *data, size_t len)
{
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++)
        h = (h + data[i]) * 0x01000193;
    return h;
}

/* convert string to double;
   return true if fully consumed and result is not NaN. */
bool str2dbl(const std::string &s, double &out)
{
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return (endp == s.c_str() + s.size()) && !isnan(out);
}

/* convert string to int64; return true if fully consumed. */
bool str2int(const std::string &s, int64_t &out)
{
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

// === helpers: serialization ===

void buf_append_u8(Buffer &buf, uint8_t data)
{
    buf_append(buf, (const uint8_t *)&data, 1);
}

void buf_append_u32(Buffer &buf, uint32_t data)
{
    buf_append(buf, (const uint8_t *)&data, 4);
}

void buf_append_i64(Buffer &buf, int64_t data)
{
    buf_append(buf, (const uint8_t *)&data, 8);
}

void buf_append_dbl(Buffer &buf, double data)
{
    buf_append(buf, (const uint8_t *)&data, 8);
}

// === helpers: output response to buffer ===

void out_nil(Buffer &out)
{
    buf_append_u8(out, TAG_NIL);
}

void out_str(Buffer &out, const char *s, size_t size)
{
    buf_append_u8(out, TAG_STR);
    buf_append_u32(out, (uint32_t)size);
    buf_append(out, (const uint8_t *)s, size);
}

void out_int(Buffer &out, int64_t val)
{
    buf_append_u8(out, TAG_INT);
    buf_append_i64(out, val);
}

void out_dbl(Buffer &out, double val)
{
    buf_append_u8(out, TAG_DBL);
    buf_append_dbl(out, val);
}

void out_err(
    Buffer &out, uint32_t code, const std::string &msg)
{
    buf_append_u8(out, TAG_ERR);
    buf_append_u32(out, code);
    buf_append_u32(out, (uint32_t)msg.size());
    buf_append(out, (const uint8_t *)msg.data(), msg.size());
}

/* output tag and size; items are added separately. */
void out_arr(Buffer &out, uint32_t n)
{
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, n);
}

/* output tag and reserve space for size (filled later by out_end_arr());
   return size position in out buffer. */
size_t out_begin_arr(Buffer &out)
{
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, 0);
    return out.size() - 4;
}

/* fill final array size. */
void out_end_arr(Buffer &out, size_t size_pos, uint32_t n)
{
    assert(out[size_pos - 1] == TAG_ARR);
    memcpy(&out[size_pos], &n, 4);
}
