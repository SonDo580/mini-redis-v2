#pragma once

// stdlib
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
// system
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
// C++
#include <vector>
#include <string>

// max message size: 32 MB (likely larger than kernel's TCP receive buffer)
const size_t K_MAX_MSG = 32 << 20;

// max number of strings in request command
const size_t K_MAX_ARGS = 200 * 1000;

// response data types
enum Tag : uint8_t
{
  TAG_NIL = 0, // nil
  TAG_ERR = 1, // error
  TAG_STR = 2, // string
  TAG_INT = 3, // int64
  TAG_DBL = 4, // double
  TAG_ARR = 5, // array
};

// error code for TAG_ERR
enum ErrCode : uint32_t
{
  ERR_UNKNOWN = 1, // unknown command
  ERR_TOO_BIG = 2, // response too big
};

void msg(const char *msg);
void msg_errno(const char *msg);
void die(const char *msg);

void fd_set_nb(int fd);

int32_t readn(int fd, uint8_t *buf, size_t n);
int32_t writen(int fd, const uint8_t *buf, size_t n);

typedef std::vector<uint8_t> Buffer;
void buf_append(Buffer &buf, const uint8_t *data, size_t len);
void buf_consume(Buffer &buf, size_t n);

/* return pointer to T given pointer to 1 member */
#define container_of(ptr, T, member) ({                  \
    const typeof( ((T *)0)->member ) *__mptr = (ptr);    \
    (T *)( (char *)__mptr - offsetof(T, member) ); })
/*
- `({...})`: statement expression
  . final value is value of the last expression
- `typeof( ((T *)0)->member )`: type of T->member
  . (T *)0: pretend there is an instance of T sitting at address 0
- `const ... *__mptr = (ptr)`:
  . ensure ptr is of correct type (at compile time).
- `(char *)__mptr`: convert to raw byte pointer
- `offsetof(T, member):` number of bytes between start of struct T and where member lives
   . account for alignment padding of any compiler.
   . predictable for each target platform.
*/

uint64_t str_hash(const uint8_t *data, size_t len);