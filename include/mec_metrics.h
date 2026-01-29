#ifndef MEC_METRICS_H
#define MEC_METRICS_H

#include "mec_common.h"

/**
 * @brief 性能监控模块
 */
typedef struct {
    long frame_count;
    struct timeval start_time;
    double total_latency_ms;
    pthread_mutex_t lock;
} mec_perf_stats_t;

void metrics_init();
void metrics_record_frame(double latency_ms);
void metrics_report(); // 输出当前的 FPS 和平均时延

#endif
