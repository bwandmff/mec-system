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

static void mec_pool_init() {
    pthread_mutex_lock(&g_mem_pool.lock);
    if (!g_mem_pool.initialized) {
        // 初始分配 16 个块
        for (int i = 0; i < 16; i++) {
            mem_block_t *block = (mem_block_t *)malloc(sizeof(mem_block_t));
            if (block) {
                block->next = g_mem_pool.free_list;
                g_mem_pool.free_list = block;
            }
        }
        g_mem_pool.initialized = true;
        LOG_INFO("Memory Pool: Initialized with 16 blocks (Block size: %zu bytes)", POOL_BLOCK_SIZE);
    }
    pthread_mutex_unlock(&g_mem_pool.lock);
}

void* mec_malloc(size_t size) {
    // 如果申请的大小刚好符合我们的块大小，则从池中获取
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
        
        // 池耗尽，回退到普通 malloc 并警告
        LOG_WARN("Memory Pool: Pool exhausted, falling back to heap");
    }
    
    return malloc(size);
}

void* mec_calloc(size_t nmemb, size_t size) {
    void *ptr = mec_malloc(nmemb * size);
    if (ptr) memset(ptr, 0, nmemb * size);
    return ptr;
}

void* mec_realloc(void *ptr, size_t size) {
    // 简单实现：暂不支持池内 realloc，直接走普通 realloc
    // 在我们的系统中，尽量通过预分配 capacity 避免 realloc
    return realloc(ptr, size);
}

void mec_free(void *ptr) {
    if (!ptr) return;

    // 检查指针是否属于我们的池
    // 注意：这里的检查比较简化。在生产环境通常会维护一个地址范围
    // 为了极致性能，我们目前在 track_list 中显式管理。
    // 如果确定是池内存，将其回收到自由链表。
    
    // 我们采取一种更稳妥的做法：
    // 如果在我们的自由分配逻辑中确认为池内存（这里通过地址范围判断）
    // 此处仅做示例实现逻辑：
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
