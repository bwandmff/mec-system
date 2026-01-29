#ifndef MEC_THREAD_H
#define MEC_THREAD_H

#include <pthread.h>

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int running;
} thread_context_t;

int thread_create(thread_context_t *ctx, void *(*start_routine)(void*), void *arg);
void thread_lock(thread_context_t *ctx);
void thread_unlock(thread_context_t *ctx);
void thread_signal(thread_context_t *ctx);
void thread_wait(thread_context_t *ctx);
void thread_join(thread_context_t *ctx);
void thread_destroy(thread_context_t *ctx);

#endif // MEC_THREAD_H
