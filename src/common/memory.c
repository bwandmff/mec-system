#include "mec_common.h"
#include <stdbool.h>

/**
 * @file memory.c
 * @brief 高性能内存池实现
 * 
 * 采用了“预分配 + 自由链表”的模式，专门优化目标航迹数组的分配。
 */

#define POOL_BLOCK_SIZE  (128 * sizeof(target_track_t)) // 每个块能容纳 128 条航迹
#define POOL_MAX_BLOCKS  64                              // 最大预分配 64 个块

typedef struct mem_block_t {
    uint8_t data[POOL_BLOCK_SIZE];
    struct mem_block_t *next;
} mem_block_t;

static struct {
    mem_block_t *free_list;
    mem_block_t *all_blocks;
    int blocks_in_use;
    pthread_mutex_t lock;
    bool initialized;
} g_mem_pool = { NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER, false };

// 静态内存池范围标记
static void *g_pool_start = NULL;
static void *g_pool_end = NULL;

static void mec_pool_init() {
    pthread_mutex_lock(&g_mem_pool.lock);
    if (!g_mem_pool.initialized) {
        // 一次性申请大块内存作为池，便于地址范围判断
        size_t total_size = POOL_MAX_BLOCKS * sizeof(mem_block_t);
        g_pool_start = malloc(total_size);
        if (g_pool_start) {
            g_pool_end = (uint8_t*)g_pool_start + total_size;
            
            // 链接链表
            mem_block_t *current = (mem_block_t *)g_pool_start;
            for (int i = 0; i < POOL_MAX_BLOCKS - 1; i++) {
                current[i].next = &current[i+1];
            }
            current[POOL_MAX_BLOCKS - 1].next = NULL;
            g_mem_pool.free_list = current;
            
            LOG_INFO("Memory Pool: Initialized with %d blocks (Block size: %zu bytes)", POOL_MAX_BLOCKS, POOL_BLOCK_SIZE);
        } else {
            LOG_ERROR("Memory Pool: Failed to allocate pool memory");
        }
        g_mem_pool.initialized = true;
    }
    pthread_mutex_unlock(&g_mem_pool.lock);
}

void* mec_malloc(size_t size) {
    if (size <= POOL_BLOCK_SIZE) {
        if (!g_mem_pool.initialized) mec_pool_init();

        pthread_mutex_lock(&g_mem_pool.lock);
        if (g_mem_pool.free_list) {
            mem_block_t *block = g_mem_pool.free_list;
            g_mem_pool.free_list = block->next;
            g_mem_pool.blocks_in_use++;
            pthread_mutex_unlock(&g_mem_pool.lock);
            return block->data;
        }
        pthread_mutex_unlock(&g_mem_pool.lock);
        
        LOG_WARN("Memory Pool: Pool exhausted, falling back to heap");
    }
    
    return malloc(size);
}

void mec_free(void *ptr) {
    if (!ptr) return;

    // 检查指针是否属于内存池范围
    if (g_mem_pool.initialized && ptr >= g_pool_start && ptr < g_pool_end) {
        // 计算块起始地址 (假设 data 是 mem_block_t 的第一个成员)
        mem_block_t *block = (mem_block_t *)ptr;
        
        pthread_mutex_lock(&g_mem_pool.lock);
        block->next = g_mem_pool.free_list;
        g_mem_pool.free_list = block;
        g_mem_pool.blocks_in_use--;
        pthread_mutex_unlock(&g_mem_pool.lock);
        return;
    }
    
    free(ptr);
}

// 专门为 track_list 优化的分配逻辑
target_track_t* mec_alloc_tracks(int count) {
    size_t size = count * sizeof(target_track_t);
    return (target_track_t*)mec_malloc(size);
}

void mec_free_tracks(target_track_t *ptr) {
    if (!ptr) return;
    
    pthread_mutex_lock(&g_mem_pool.lock);
    // 假设我们只回收符合 POOL_BLOCK_SIZE 的块进入池
    // 实际项目中会更复杂，这里作为第三阶段的性能原型
    pthread_mutex_unlock(&g_mem_pool.lock);
    
    free(ptr);
}

void* mec_calloc(size_t nmemb, size_t size) {
    void *ptr = mec_malloc(nmemb * size);
    if (ptr) memset(ptr, 0, nmemb * size);
    return ptr;
}

void* mec_realloc(void *ptr, size_t size) {
    // 简单实现：暂不支持池内 realloc，直接走普通 realloc
    // 如果 ptr 在池内，这种做法是危险的，需要迁移数据。
    // 但鉴于我们目前的使用场景（track_list扩容），通常是大块内存，
    // 大块内存我们走的是系统 malloc，所以这里暂且可以直接 realloc。
    // 如果 ptr 是小块池内内存，这里会崩溃。
    // 真正的修复应该判断 range。
    
    if (g_mem_pool.initialized && ptr >= g_pool_start && ptr < g_pool_end) {
        // 是池内内存，必须分配新块（或堆内存）并拷贝
        // 但池块大小固定，realloc 通常意味着变大，所以只能去堆
        void *new_ptr = malloc(size);
        if (new_ptr) {
            memcpy(new_ptr, ptr, POOL_BLOCK_SIZE); // 只能拷贝这么多了
            mec_free(ptr); // 归还旧块
        }
        return new_ptr;
    }
    
    return realloc(ptr, size);
}
