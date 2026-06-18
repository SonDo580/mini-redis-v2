#pragma once

// stdlib
#include <stddef.h>
#include <stdint.h>
// C++
#include <string>

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