#include "mec_common.h"

static size_t total_allocated = 0;
static pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;

void* mec_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr) {
        pthread_mutex_lock(&memory_mutex);
        total_allocated += size;
        pthread_mutex_unlock(&memory_mutex);
    } else {
        LOG_ERROR("Memory allocation failed for %zu bytes", size);
    }
    return ptr;
}

void* mec_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (ptr) {
        pthread_mutex_lock(&memory_mutex);
        total_allocated += nmemb * size;
        pthread_mutex_unlock(&memory_mutex);
    } else {
        LOG_ERROR("Memory allocation failed for %zu elements of %zu bytes", nmemb, size);
    }
    return ptr;
}

void* mec_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        LOG_ERROR("Memory reallocation failed for %zu bytes", size);
    }
    return new_ptr;
}

void mec_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}