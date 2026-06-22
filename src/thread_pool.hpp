#pragma once

// stdlib
#include <stddef.h>
// system
#include <pthread.h>
// C++
#include <vector>
#include <deque>

// pointer to function that accepts 'void *' argument and returns 'void *'
typedef void (*TaskFunc)(void *);

struct Task
{
    TaskFunc f = NULL;
    void *arg = NULL;
};

struct ThreadPool
{
    std::vector<pthread_t> threads; // worker threads
    std::deque<Task> queue;         // tasks queue
    pthread_mutex_t mu;             // mutex to protect the tasks queue
    pthread_cond_t not_empty;       // condition variable for wait/notify
};

void thread_pool_init(ThreadPool *tp, size_t num_threads);
void thread_pool_queue(ThreadPool *tp, TaskFunc f, void *arg);