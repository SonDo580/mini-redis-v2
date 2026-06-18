#pragma once

// stdlib
#include <stddef.h>
#include <stdint.h>
// C++
#include <string>

#include "common.hpp"

void fd_set_nb(int fd);

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

bool str2dbl(const std::string &s, double &out);
bool str2int(const std::string &s, int64_t &out);

// === helpers: serialization ===
void buf_append_u8(Buffer &buf, uint8_t data);
void buf_append_u32(Buffer &buf, uint32_t data);
void buf_append_i64(Buffer &buf, int64_t data);
void buf_append_dbl(Buffer &buf, double data);

// === helpers: output response to buffer ===
void out_nil(Buffer &out);
void out_str(Buffer &out, const char *s, size_t size);
void out_int(Buffer &out, int64_t val);
void out_dbl(Buffer &out, double val);
void out_err(Buffer &out, uint32_t code, const std::string &msg);
void out_arr(Buffer &out, uint32_t n);
size_t out_begin_arr(Buffer &out);
void out_end_arr(Buffer &out, size_t size_pos, uint32_t n);