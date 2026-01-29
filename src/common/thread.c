#include "mec_common.h"
#include "mec_thread.h"
#include "mec_logging.h"
#include <errno.h>

int thread_create(thread_context_t *ctx, void *(*start_routine)(void*), void *arg) {
    if (!ctx) return -1;
    
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize mutex");
        return -1;
    }
    
    if (pthread_cond_init(&ctx->cond, NULL) != 0) {
        pthread_mutex_destroy(&ctx->mutex);
        LOG_ERROR("Failed to initialize condition variable");
        return -1;
    }
    
    ctx->running = 1;
    
    if (pthread_create(&ctx->thread, NULL, start_routine, arg) != 0) {
        ctx->running = 0;
        pthread_cond_destroy(&ctx->cond);
        pthread_mutex_destroy(&ctx->mutex);
        LOG_ERROR("Failed to create thread");
        return -1;
    }
    
    return 0;
}

void thread_destroy(thread_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->running) {
        // Simple cancellation - in a real system we'd want a safer stop
        thread_join(ctx);
    }
    
    pthread_cond_destroy(&ctx->cond);
    pthread_mutex_destroy(&ctx->mutex);
}

void thread_lock(thread_context_t *ctx) {
    if (ctx) pthread_mutex_lock(&ctx->mutex);
}

void thread_unlock(thread_context_t *ctx) {
    if (ctx) pthread_mutex_unlock(&ctx->mutex);
}

void thread_signal(thread_context_t *ctx) {
    if (ctx) pthread_cond_signal(&ctx->cond);
}

void thread_wait(thread_context_t *ctx) {
    if (ctx) pthread_cond_wait(&ctx->cond, &ctx->mutex);
}

void thread_join(thread_context_t *ctx) {
    if (!ctx || !ctx->running) return;
    
    if (pthread_join(ctx->thread, NULL) != 0) {
        LOG_ERROR("Failed to join thread");
    } else {
        ctx->running = 0;
    }
}
