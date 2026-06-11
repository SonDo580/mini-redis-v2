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
