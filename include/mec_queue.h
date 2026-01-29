#ifndef MEC_QUEUE_H
#define MEC_QUEUE_H

#include "mec_common.h"

/**
 * @brief 消息单元：封装单次传感器上报的完整数据包
 * 
 * 该结构体作为队列的基本存储单位，包含了传感器身份、目标列表和接收时间戳。
 */
typedef struct {
    int sensor_id;            // 传感器ID (1: Video, 2: Radar)
    track_list_t *tracks;     // 目标航迹列表数据（指针所有权由队列管理）
    struct timeval timestamp; // 数据接收到的系统时间戳
} mec_msg_t;

/**
 * @brief 线程安全循环队列句柄（不透明结构体，隐藏实现细节）
 */
typedef struct mec_queue_t mec_queue_t;

/**
 * @brief 创建一个新的消息队列
 * 
 * @param capacity 队列容量（最大允许积压的消息包数量）
 * @return 成功返回队列句柄，失败返回NULL（通常因内存不足）
 */
mec_queue_t* mec_queue_create(int capacity);

/**
 * @brief 销毁队列并释放相关资源
 * 
 * 会自动清理队列中尚未被弹出的残留消息内存。
 * @param queue 队列句柄
 */
void mec_queue_destroy(mec_queue_t *queue);

/**
 * @brief 生产者调用：向队列压入一条消息
 * 
 * 内部会对 tracks 进行深拷贝，确保多线程环境下的内存安全。
 * @param queue 队列句柄
 * @param msg 要压入的消息内容
 * @return 0:成功, -1:队列满（非阻塞模式）
 */
int mec_queue_push(mec_queue_t *queue, const mec_msg_t *msg);

/**
 * @brief 消费者调用：从队列弹出一条消息
 * 
 * 如果队列为空，调用线程将进入阻塞状态，直到有新数据或超时。
 * @param queue 队列句柄
 * @param out_msg 用于接收弹出的消息数据（调用者需负责释放 out_msg.tracks）
 * @param timeout_ms 超时时间（毫秒）。-1 表示无限等待，0 表示不等待（立即返回）
 * @return 0:成功弹出, -1:超时或出错
 */
int mec_queue_pop(mec_queue_t *queue, mec_msg_t *out_msg, int timeout_ms);

/**
 * @brief 获取当前队列中积压的消息数量
 * 
 * @param queue 队列句柄
 * @return 消息条数
 */
int mec_queue_size(mec_queue_t *queue);

#endif // MEC_QUEUE_H
