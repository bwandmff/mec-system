#ifndef MEC_WATCHDOG_H
#define MEC_WATCHDOG_H

#include <stdatomic.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    pthread_t thread;
    atomic_int running;
    
    time_t last_heartbeat;      // 最后一次心跳时间
    int timeout_seconds;        // 超时阈值（秒）
    int check_interval_ms;      // 检查间隔（毫秒）
    
    atomic_int frame_count;     // 帧计数
    time_t last_frame_time;    // 最后处理帧的时间
    
    void (*on_timeout)(void *); // 超时回调函数
    void *callback_arg;         // 回调参数
    
} watchdog_t;

/**
 * @brief 创建看门狗
 * @param timeout_seconds 超时秒数
 * @param check_interval_ms 检查间隔（毫秒）
 */
watchdog_t* watchdog_create(int timeout_seconds, int check_interval_ms);

/**
 * @brief 启动看门狗
 */
int watchdog_start(watchdog_t *wd);

/**
 * @brief 停止看门狗
 */
void watchdog_stop(watchdog_t *wd);

/**
 * @brief 销毁看门狗
 */
void watchdog_destroy(watchdog_t *wd);

/**
 * @brief 喂狗 - 更新心跳/帧计数
 */
void watchdog_feed(watchdog_t *wd);

/**
 * @brief 记录处理了一帧
 */
void watchdog_record_frame(watchdog_t *wd);

/**
 * @brief 设置超时回调
 */
void watchdog_set_callback(watchdog_t *wd, void (*callback)(void *), void *arg);

#endif // MEC_WATCHDOG_H
