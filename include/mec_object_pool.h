#ifndef MEC_OBJECT_POOL_H
#define MEC_OBJECT_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 对象池类型定义
 */
typedef enum {
    POOL_TYPE_TRACK_LIST,    // 航迹列表
    POOL_TYPE_TARGET_TRACK,   // 单个目标
    POOL_TYPE_MSG,            // 消息单元
    POOL_TYPE_QUEUE_NODE,     // 队列节点
    POOL_TYPE_MAX
} pool_type_t;

/**
 * @brief 对象池统计信息
 */
typedef struct {
    int total_objects;       // 总对象数
    int in_use;              // 使用中
    int available;           // 可用
    int alloc_count;         // 分配次数
    int free_count;          // 释放次数
} pool_stats_t;

/**
 * @brief 初始化对象池
 * @param type 对象类型
 * @param obj_size 对象大小
 * @param initial_count 初始对象数量
 */
int object_pool_init(pool_type_t type, size_t obj_size, int initial_count);

/**
 * @brief 从对象池分配对象
 */
void* object_pool_alloc(pool_type_t type);

/**
 * @brief 释放对象回对象池
 */
void object_pool_free(pool_type_t type, void *ptr);

/**
 * @brief 获取对象池统计信息
 */
void object_pool_get_stats(pool_type_t type, pool_stats_t *stats);

/**
 * @brief 打印所有对象池状态
 */
void object_pool_print_stats(void);

/**
 * @brief 清理所有对象池
 */
void object_pool_cleanup(void);

#endif // MEC_OBJECT_POOL_H
