#include "mec_common.h"

int thread_create(thread_context_t *ctx, void *(*start_routine)(void*), void *arg) {
    if (!ctx) return -1;
    
    ctx->data = arg;
    ctx->running = 1;
    
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize mutex");
        return -1;
    }
    
    if (pthread_cond_init(&ctx->cond, NULL) != 0) {
        LOG_ERROR("Failed to initialize condition variable");
        pthread_mutex_destroy(&ctx->mutex);
        return -1;
    }
    
    if (pthread_create(&ctx->thread, NULL, start_routine, ctx) != 0) {
        LOG_ERROR("Failed to create thread");
        pthread_mutex_destroy(&ctx->mutex);
        pthread_cond_destroy(&ctx->cond);
        return -1;
    }
    
    return 0;
}

void thread_destroy(thread_context_t *ctx) {
    if (!ctx) return;
    
    ctx->running = 0;
    thread_signal(ctx);
    
    if (pthread_join(ctx->thread, NULL) != 0) {
        LOG_ERROR("Failed to join thread");
    }
    
    pthread_mutex_destroy(&ctx->mutex);
    pthread_cond_destroy(&ctx->cond);
}

void thread_lock(thread_context_t *ctx) {
    if (ctx) {
        pthread_mutex_lock(&ctx->mutex);
    }
}

void thread_unlock(thread_context_t *ctx) {
    if (ctx) {
        pthread_mutex_unlock(&ctx->mutex);
    }
}

void thread_wait(thread_context_t *ctx) {
    if (ctx) {
        pthread_cond_wait(&ctx->cond, &ctx->mutex);
    }
}

void thread_signal(thread_context_t *ctx) {
    if (ctx) {
        pthread_cond_signal(&ctx->cond);
    }
}