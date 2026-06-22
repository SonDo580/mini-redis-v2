#pragma once

// stdlib
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// C++
#include <vector>

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

struct Buffer;
void buf_append(Buffer &buf, const uint8_t *data, size_t len);
void buf_consume(Buffer &buf, size_t n);

typedef struct Buffer
{
  uint8_t *begin = NULL;
  uint8_t *end = NULL; // exclusive
  uint8_t *data_begin = NULL;
  uint8_t *data_end = NULL; // exclusive

  inline size_t size() const
  {
    return data_end - data_begin;
  }

  inline uint8_t *data()
  {
    return data_begin;
  }
  inline const uint8_t *data() const
  {
    return data_begin;
  }

  inline uint8_t &operator[](size_t index)
  {
    return data_begin[index];
  }
  inline const uint8_t &operator[](size_t index) const
  {
    return data_begin[index];
  }

  inline void resize(size_t new_data_size)
  {
    size_t data_size = data_end - data_begin;
    if (new_data_size == data_size) // unchanged
      return;

    if (new_data_size < data_size)
    { // truncate
      data_end = data_begin + new_data_size;
      return;
    }

    // grow by appending zero-initialized bytes
    size_t growth_needed = new_data_size - data_size;
    size_t write_offset = data_end - begin;
    buf_append(*this, NULL, growth_needed); // might reallocate and shift 'begin'
    memset(begin + write_offset, 0, growth_needed);
  }
} Buffer;
