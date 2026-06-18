// stdlib
#include <math.h>
// system
#include <fcntl.h>

#include "common.hpp"
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
