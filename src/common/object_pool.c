/*
 * mec_object_pool.c - 通用对象池实现
 */

#include "mec_object_pool.h"
#include "mec_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_POOLS 4

typedef struct object_pool_node {
    void *data;
    struct object_pool_node *next;
} object_pool_node_t;

typedef struct {
    pool_type_t type;
    size_t obj_size;
    object_pool_node_t *free_list;
    int total_count;
    int in_use_count;
    int alloc_count;
    int free_count;
    int initial_count;
    int expand_threshold;  // 扩展阈值
    pthread_mutex_t lock;
    bool initialized;
} object_pool_internal_t;

static object_pool_internal_t g_pools[POOL_TYPE_MAX];
static bool g_pools_initialized = false;

// 初始化各个对象池
static void init_pools(void) {
    if (g_pools_initialized) return;
    
    memset(g_pools, 0, sizeof(g_pools));
    
    // 预定义的池配置
    g_pools[POOL_TYPE_TRACK_LIST].obj_size = sizeof(void*);  // 指针大小
    g_pools[POOL_TYPE_TRACK_LIST].initial_count = 100;
    g_pools[POOL_TYPE_TRACK_LIST].expand_threshold = 20;
    
    g_pools[POOL_TYPE_TARGET_TRACK].obj_size = sizeof(void*);
    g_pools[POOL_TYPE_TARGET_TRACK].initial_count = 1000;
    g_pools[POOL_TYPE_TARGET_TRACK].expand_threshold = 100;
    
    g_pools[POOL_TYPE_MSG].obj_size = sizeof(void*);
    g_pools[POOL_TYPE_MSG].initial_count = 200;
    g_pools[POOL_TYPE_MSG].expand_threshold = 50;
    
    g_pools[POOL_TYPE_QUEUE_NODE].obj_size = sizeof(void*);
    g_pools[POOL_TYPE_QUEUE_NODE].initial_count = 500;
    g_pools[POOL_TYPE_QUEUE_NODE].expand_threshold = 100;
    
    // 初始化锁和预分配
    for (int i = 0; i < POOL_TYPE_MAX; i++) {
        g_pools[i].type = i;
        pthread_mutex_init(&g_pools[i].lock, NULL);
        
        // 预分配对象
        if (g_pools[i].initial_count > 0) {
            object_pool_init(i, g_pools[i].obj_size, g_pools[i].initial_count);
        }
    }
    
    g_pools_initialized = true;
    LOG_INFO("Object Pools: Initialized all pools");
}

int object_pool_init(pool_type_t type, size_t obj_size, int initial_count) {
    if (type >= POOL_TYPE_MAX || obj_size == 0 || initial_count <= 0) {
        return -1;
    }
    
    if (!g_pools_initialized) {
        init_pools();
    }
    
    object_pool_internal_t *pool = &g_pools[type];
    
    pthread_mutex_lock(&pool->lock);
    
    if (pool->initialized) {
        pthread_mutex_unlock(&pool->lock);
        return 0;
    }
    
    pool->obj_size = obj_size;
    pool->initial_count = initial_count;
    pool->expand_threshold = initial_count / 5;
    if (pool->expand_threshold < 10) pool->expand_threshold = 10;
    
    // 预分配
    for (int i = 0; i < initial_count; i++) {
        object_pool_node_t *node = malloc(sizeof(object_pool_node_t));
        if (!node) break;
        
        node->data = malloc(obj_size);
        if (!node->data) {
            free(node);
            break;
        }
        
        node->next = pool->free_list;
        pool->free_list = node;
        pool->total_count++;
    }
    
    pool->initialized = true;
    pthread_mutex_unlock(&pool->lock);
    
    LOG_INFO("Object Pool %d: Pre-allocated %d objects (size: %zu)", 
              type, pool->total_count, obj_size);
    
    return 0;
}

void* object_pool_alloc(pool_type_t type) {
    if (type >= POOL_TYPE_MAX) return NULL;
    
    if (!g_pools_initialized) {
        init_pools();
    }
    
    object_pool_internal_t *pool = &g_pools[type];
    void *ptr = NULL;
    
    pthread_mutex_lock(&pool->lock);
    
    if (pool->free_list) {
        // 从空闲列表取
        object_pool_node_t *node = pool->free_list;
        pool->free_list = node->next;
        ptr = node->data;
        free(node);  // 释放节点结构，数据保留
        
        pool->in_use_count++;
        pool->alloc_count++;
    } else {
        // 池已耗尽，动态扩展
        int expand_count = pool->expand_threshold;
        
        for (int i = 0; i < expand_count; i++) {
            object_pool_node_t *node = malloc(sizeof(object_pool_node_t));
            if (!node) break;
            
            node->data = malloc(pool->obj_size);
            if (!node->data) {
                free(node);
                break;
            }
            
            node->next = pool->free_list;
            pool->free_list = node;
            pool->total_count++;
        }
        
        // 再次尝试分配
        if (pool->free_list) {
            object_pool_node_t *node = pool->free_list;
            pool->free_list = node->next;
            ptr = node->data;
            free(node);
            
            pool->in_use_count++;
            pool->alloc_count++;
        }
    }
    
    pthread_mutex_unlock(&pool->lock);
    
    // 如果池扩展后仍然失败，返回系统 malloc
    if (!ptr) {
        ptr = malloc(pool->obj_size);
    }
    
    return ptr;
}

void object_pool_free(pool_type_t type, void *ptr) {
    if (type >= POOL_TYPE_MAX || !ptr) return;
    
    object_pool_internal_t *pool = &g_pools[type];
    
    pthread_mutex_lock(&pool->lock);
    
    // 放回空闲列表
    object_pool_node_t *node = malloc(sizeof(object_pool_node_t));
    if (node) {
        node->data = ptr;
        node->next = pool->free_list;
        pool->free_list = node;
        pool->in_use_count--;
        pool->free_count++;
    }
    
    pthread_mutex_unlock(&pool->lock);
}

void object_pool_get_stats(pool_type_t type, pool_stats_t *stats) {
    if (type >= POOL_TYPE_MAX || !stats) return;
    
    object_pool_internal_t *pool = &g_pools[type];
    
    pthread_mutex_lock(&pool->lock);
    stats->total_objects = pool->total_count;
    stats->in_use = pool->in_use_count;
    stats->available = pool->total_count - pool->in_use_count;
    stats->alloc_count = pool->alloc_count;
    stats->free_count = pool->free_count;
    pthread_mutex_unlock(&pool->lock);
}

void object_pool_print_stats(void) {
    if (!g_pools_initialized) init_pools();
    
    LOG_INFO("========== Object Pool Statistics ==========");
    
    for (int i = 0; i < POOL_TYPE_MAX; i++) {
        object_pool_internal_t *pool = &g_pools[i];
        
        pthread_mutex_lock(&pool->lock);
        LOG_INFO("Pool[%d]: Total=%d, InUse=%d, Available=%d, Alloc=%d, Free=%d",
                  i, pool->total_count, pool->in_use_count, 
                  pool->total_count - pool->in_use_count,
                  pool->alloc_count, pool->free_count);
        pthread_mutex_unlock(&pool->lock);
    }
    
    LOG_INFO("===========================================");
}

void object_pool_cleanup(void) {
    for (int i = 0; i < POOL_TYPE_MAX; i++) {
        object_pool_internal_t *pool = &g_pools[i];
        
        // 释放所有空闲节点
        object_pool_node_t *node = pool->free_list;
        while (node) {
            object_pool_node_t *next = node->next;
            if (node->data) free(node->data);
            free(node);
            node = next;
        }
        
        pthread_mutex_destroy(&pool->lock);
    }
    
    g_pools_initialized = false;
}
