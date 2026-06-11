#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <assert.h>

const size_t K_MAX_MSG = 4096; // max message size in bytes

void msg(const char *msg);
void die(const char *msg);

int32_t readn(int fd, char *buf, size_t n);
int32_t writen(int fd, const char *buf, size_t n);