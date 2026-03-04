/*
 * mec_batch.c - 批量处理实现
 */

#include "mec_batch.h"
#include "mec_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdatomic.h>

#define DEFAULT_MAX_BATCH_SIZE 32
#define DEFAULT_MAX_WAIT_MS 100

typedef struct batch_item {
    void *data;
    struct batch_item *next;
} batch_item_t;

struct mec_batch_ctx {
    batch_config_t config;
    batch_item_t *head;
    batch_item_t *tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t thread;
    atomic_int running;
    batch_callback_t callback;
    void *user_data;
    struct timeval last_flush;
};

static void* batch_worker(void *arg) {
    mec_batch_ctx_t *ctx = (mec_batch_ctx_t *)arg;
    
    while (atomic_load(&ctx->running)) {
        pthread_mutex_lock(&ctx->lock);
        
        // 计算等待时间
        struct timeval now;
        gettimeofday(&now, 0);
        long elapsed_ms = (now.tv_sec - ctx->last_flush.tv_sec) * 1000 + 
                         (now.tv_usec - ctx->last_flush.tv_usec) / 1000;
        
        if (ctx->count == 0) {
            // 无数据，等待
            pthread_cond_wait(&ctx->cond, &ctx->lock);
        } else if (ctx->count >= ctx->config.max_batch_size || 
                   elapsed_ms >= ctx->config.max_wait_ms) {
            // 批次已满或超时，处理
            pthread_mutex_unlock(&ctx->lock);
            
            batch_process(ctx);
            gettimeofday(&ctx->last_flush, 0);
            continue;
        } else {
            // 等待更多数据
            struct timespec ts;
            ts.tv_sec = now.tv_sec + ctx->config.max_wait_ms / 1000;
            ts.tv_nsec = (now.tv_usec + (ctx->config.max_wait_ms % 1000) * 1000) * 1000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            pthread_cond_timedwait(&ctx->cond, &ctx->lock, &ts);
        }
        
        pthread_mutex_unlock(&ctx->lock);
    }
    
    return NULL;
}

mec_batch_ctx_t* batch_create(const batch_config_t *config) {
    mec_batch_ctx_t *ctx = calloc(1, sizeof(mec_batch_ctx_t));
    if (!ctx) return NULL;
    
    if (config) {
        ctx->config = *config;
    } else {
        ctx->config.max_batch_size = DEFAULT_MAX_BATCH_SIZE;
        ctx->config.max_wait_ms = DEFAULT_MAX_WAIT_MS;
        ctx->config.enable_adaptive = true;
    }
    
    pthread_mutex_init(&ctx->lock, NULL);
    pthread_cond_init(&ctx->cond, NULL);
    atomic_init(&ctx->running, 0);
    gettimeofday(&ctx->last_flush, 0);
    
    LOG_INFO("Batch processor created (max_size=%d, max_wait=%dms)", 
             ctx->config.max_batch_size, ctx->config.max_wait_ms);
    
    return ctx;
}

void batch_destroy(mec_batch_ctx_t *ctx) {
    if (!ctx) return;
    
    // 停止工作线程
    if (atomic_load(&ctx->running)) {
        atomic_store(&ctx->running, 0);
        pthread_cond_signal(&ctx->cond);
        pthread_join(ctx->thread, NULL);
    }
    
    // 清理剩余数据
    batch_flush(ctx);
    
    pthread_mutex_destroy(&ctx->lock);
    pthread_cond_destroy(&ctx->cond);
    free(ctx);
}

int batch_add(mec_batch_ctx_t *ctx, void *data) {
    if (!ctx || !data) return -1;
    
    batch_item_t *item = malloc(sizeof(batch_item_t));
    if (!item) return -1;
    
    item->data = data;
    item->next = NULL;
    
    pthread_mutex_lock(&ctx->lock);
    
    if (ctx->tail) {
        ctx->tail->next = item;
        ctx->tail = item;
    } else {
        ctx->head = ctx->tail = item;
    }
    ctx->count++;
    
    int should_process = (ctx->count >= ctx->config.max_batch_size);
    
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->lock);
    
    return should_process;
}

void batch_process(mec_batch_ctx_t *ctx) {
    if (!ctx || ctx->count == 0) return;
    
    // 取出所有数据
    pthread_mutex_lock(&ctx->lock);
    
    batch_item_t *items = ctx->head;
    int count = ctx->count;
    
    ctx->head = ctx->tail = NULL;
    ctx->count = 0;
    
    pthread_mutex_unlock(&ctx->lock);
    
    if (!ctx->callback) {
        // 无回调，释放内存
        while (items) {
            batch_item_t *next = items->next;
            free(items->data);
            free(items);
            items = next;
        }
        return;
    }
    
    // 转换为数组
    void **array = malloc(count * sizeof(void *));
    if (!array) {
        while (items) {
            batch_item_t *next = items->next;
            free(items->data);
            free(items);
            items = next;
        }
        return;
    }
    
    int i = 0;
    while (items) {
        array[i++] = items->data;
        batch_item_t *next = items->next;
        free(items);
        items = next;
    }
    
    // 调用回调
    ctx->callback(array, count, ctx->user_data);
    
    free(array);
}

void batch_flush(mec_batch_ctx_t *ctx) {
    batch_process(ctx);
}

int batch_size(mec_batch_ctx_t *ctx) {
    if (!ctx) return 0;
    pthread_mutex_lock(&ctx->lock);
    int size = ctx->count;
    pthread_mutex_unlock(&ctx->lock);
    return size;
}
