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
#include <map>

// max message size: 32 MB (likely larger than kernel's TCP receive buffer)
const size_t K_MAX_MSG = 32 << 20;

// max number of strings in request command
const size_t K_MAX_ARGS = 200 * 1000;

void msg(const char *msg);
void msg_errno(const char *msg);
void die(const char *msg);

void fd_set_nb(int fd);

int32_t readn(int fd, uint8_t *buf, size_t n);
int32_t writen(int fd, const uint8_t *buf, size_t n);

void buf_append(
    std::vector<uint8_t> &buf, const uint8_t *data, size_t len);
void buf_consume(std::vector<uint8_t> &buf, size_t n);
