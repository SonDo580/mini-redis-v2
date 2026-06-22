// stdlib
#include <assert.h>

#include "thread_pool.hpp"

static void *consume(void *arg)
{
    ThreadPool *tp = (ThreadPool *)arg;
    while (true)
    {
        pthread_mutex_lock(&tp->mu);

        // wait for condition: a non-empty queue
        while (tp->queue.empty())
        {
            // - release the mutex lock before sleeping.
            // - wait by sleeping.
            // - reclaim the lock when waking up.
            pthread_cond_wait(&tp->not_empty, &tp->mu);
        }
        /*
        1. Why use 'while (tp->queue.empty())' instead of 'if (tp->queue.empty())':
        - if there are multiple consumers, the consumer that is woken up by signal()
          may not get to consume the queue.
        - other consumers may acquire the mutex lock before it and consume all items.

        2. Why pthread_cond_wait() unlock-and-wait atomically:
        - if unlock() and wait() are 2 separate steps,
          the producer may update the queue and call signal() between them.
        - the signal is lost and the consumer will sleep indefinitely
          (until another signal wakes it up).
        */

        // condition met: consume the queue
        Task task = tp->queue.front();
        tp->queue.pop_front();
        pthread_mutex_unlock(&tp->mu);

        // do the work
        task.f(task.arg);
    }
}

void thread_pool_init(ThreadPool *tp, size_t num_threads)
{
    pthread_mutex_init(&tp->mu, NULL);
    pthread_cond_init(&tp->not_empty, NULL);

    tp->threads.resize(num_threads);
    for (size_t i = 0; i < num_threads; i++)
    {
        int rv = pthread_create(&tp->threads[i], NULL, consume, tp);
        assert(rv == 0);
    }
}

/* produce */
void thread_pool_queue(ThreadPool *tp, TaskFunc f, void *arg)
{
    pthread_mutex_lock(&tp->mu);
    tp->queue.push_back(Task{f, arg});

    // signal after unlock
    pthread_mutex_unlock(&tp->mu);
    pthread_cond_signal(&tp->not_empty);

    /* 'signal before unlock' also works
    ...
    pthread_cond_signal(&tp->not_empty);
    pthread_mutex_unlock(&tp->mu);

    - possible performance issue:
      . producer adds a task to queue and asks OS to wake up a consumer.
      . consumer wakes up (moved out of condition's wait queue),
        attempts to acquire the lock, but producer hasn't released it,
        so it is moved backed to mutex's wait queue.
    - however, many OS recognize this pattern and use 'wait morphing':
      . it doesn't wake the consumer right away, but moves the consumer
        from condition's wait queue to mutex's wait queue directly.
    */
}
