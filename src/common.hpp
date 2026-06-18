#pragma once

// stdlib
// #include <stdlib.h>
// #include <string.h>
// #include <stdio.h>
// #include <errno.h>
// #include <math.h>
#include <stddef.h>
#include <stdint.h>
// system
// #include <fcntl.h>
// C++
#include <vector>
// #include <string>

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
  ERR_UNKNOWN = 1,  // unknown command
  ERR_TOO_BIG = 2,  // response too big
  ERR_BAD_TYPE = 3, // unexpected value type
  ERR_BAD_ARG = 4,  // bad arguments
};

void msg(const char *msg);
void msg_errno(const char *msg);
void die(const char *msg);

int32_t readn(int fd, uint8_t *buf, size_t n);
int32_t writen(int fd, const uint8_t *buf, size_t n);

typedef std::vector<uint8_t> Buffer;
void buf_append(Buffer &buf, const uint8_t *data, size_t len);
void buf_consume(Buffer &buf, size_t n);
