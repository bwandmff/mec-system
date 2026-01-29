#include "mec_common.h"
#include "mec_queue.h"
#include "mec_logging.h"
#include <sys/time.h>

struct mec_queue_t {
    mec_msg_t *buffer;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

mec_queue_t* mec_queue_create(int capacity) {
    mec_queue_t *q = (mec_queue_t*)mec_malloc(sizeof(mec_queue_t));
    if (!q) return NULL;

    q->buffer = (mec_msg_t*)mec_malloc(sizeof(mec_msg_t) * capacity);
    if (!q->buffer) {
        mec_free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);

    LOG_INFO("MEC Queue: Initialized with capacity %d", capacity);
    return q;
}

void mec_queue_destroy(mec_queue_t *queue) {
    if (!queue) return;

    pthread_mutex_lock(&queue->lock);
    
    // Cleanup remaining items
    while (queue->count > 0) {
        mec_msg_t *msg = &queue->buffer[queue->tail];
        if (msg->tracks) {
            track_list_release(msg->tracks);
        }
        queue->tail = (queue->tail + 1) % queue->capacity;
        queue->count--;
    }

    pthread_mutex_unlock(&queue->lock);
    
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    
    mec_free(queue->buffer);
    mec_free(queue);
    
    LOG_INFO("MEC Queue: Destroyed");
}

int mec_queue_push(mec_queue_t *queue, const mec_msg_t *msg) {
    if (!queue || !msg) return -1;

    pthread_mutex_lock(&queue->lock);
    
    if (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->lock);
        LOG_WARN("MEC Queue: Push failed - buffer overflow!");
        return -1;
    }
    
    // Copy message to buffer
    queue->buffer[queue->head] = *msg;
    
    // Retain tracks if present since we're holding a reference in the queue
    if (msg->tracks) {
        track_list_retain(msg->tracks);
    }
    
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count++;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
    
    return 0;
}

int mec_queue_pop(mec_queue_t *queue, mec_msg_t *out_msg, int timeout_ms) {
    if (!queue || !out_msg) return -1;

    pthread_mutex_lock(&queue->lock);
    
    while (queue->count == 0) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        
        if (timeout_ms < 0) {
            pthread_cond_wait(&queue->not_empty, &queue->lock);
        } else {
            struct timespec ts;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            
            // Add timeout
            ts.tv_sec = tv.tv_sec + timeout_ms / 1000;
            ts.tv_nsec = (tv.tv_usec + (timeout_ms % 1000) * 1000) * 1000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            
            int rc = pthread_cond_timedwait(&queue->not_empty, &queue->lock, &ts);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&queue->lock);
                return -1;
            }
        }
    }
    
    *out_msg = queue->buffer[queue->tail];
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count--;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
    
    return 0;
}

int mec_queue_size(mec_queue_t *queue) {
    if (!queue) return 0;
    pthread_mutex_lock(&queue->lock);
    int size = queue->count;
    pthread_mutex_unlock(&queue->lock);
    return size;
}
