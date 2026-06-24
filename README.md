# Mini Redis

An in-memory key-value server

## Guide

https://build-your-own.org/redis/

## Features

- Event loop, Thread pool, Timer.
- Data structures:
  - Hash map: for top-level store.
  - Sorted set: combine hashmap & AVL tree.
  - Doubly linked list: for idle connection timeout.
  - Min heap: for entries' TTLs.
- CLI client.

## Usage

```bash
# build server & client
cd src
make

# send command(s)
./client.exe <command> <args...> # one-shot
./client.exe # REPL
```

## Supported commands

```bash
set key val
get key
del key
keys

pexpire key ttl_ms
pttl key
persist key

zadd zset score name
zrem zset name
zscore zset name
zrank zset name
zquery zset score name offset limit
```

## My extensions

- efficient `Buffer` _(original implementation use `std::vector`, which takes O(n) when removing from the front)_.
- REPL client.
- `zrank` command.
- command table.
