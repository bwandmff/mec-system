/*
 * mec_batch.h - 批量处理接口
 */

#ifndef MEC_BATCH_H
#define MEC_BATCH_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 批处理配置
 */
typedef struct {
    int max_batch_size;      // 最大批次大小
    int max_wait_ms;        // 最大等待时间（毫秒）
    bool enable_adaptive;   // 自适应批处理
} batch_config_t;

/**
 * @brief 批处理上下文
 */
typedef struct mec_batch_ctx mec_batch_ctx_t;

/**
 * @brief 创建批处理上下文
 */
mec_batch_ctx_t* batch_create(const batch_config_t *config);

/**
 * @brief 销毁批处理上下文
 */
void batch_destroy(mec_batch_ctx_t *ctx);

/**
 * @brief 添加数据到批次
 * @return 0:继续累积, 1:批次已满需处理
 */
int batch_add(mec_batch_ctx_t *ctx, void *data);

/**
 * @brief 处理当前批次
 */
void batch_process(mec_batch_ctx_t *ctx);

/**
 * @brief 强制处理当前批次
 */
void batch_flush(mec_batch_ctx_t *ctx);

/**
 * @brief 获取当前批次大小
 */
int batch_size(mec_batch_ctx_t *ctx);

/**
 * @brief 批处理回调函数类型
 */
typedef void (*batch_callback_t)(void **items, int count, void *user_data);

#endif // MEC_BATCH_H
