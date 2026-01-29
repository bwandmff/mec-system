#ifndef MEC_COMMON_H
#define MEC_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

// Forward declarations
typedef struct track_list_t track_list_t;

// Common data structures
typedef struct {
    double latitude;
    double longitude;
    double altitude;
} wgs84_coord_t;

typedef struct {
    int x;
    int y;
} image_coord_t;

typedef enum {
    TARGET_VEHICLE = 0,
    TARGET_NON_VEHICLE = 1,
    TARGET_PEDESTRIAN = 2,
    TARGET_OBSTACLE = 3
} target_type_t;

typedef struct {
    int id;
    target_type_t type;
    wgs84_coord_t position;
    double velocity;
    double heading;
    double confidence;
    struct timeval timestamp;
    int sensor_id;
} target_track_t;

struct track_list_t {
    target_track_t *tracks;
    int count;
    int capacity;
    int ref_count;            // 引用计数
    pthread_mutex_t ref_lock; // 保护计数的锁
};

// 性能监控统计
typedef struct {
    double fps;               // 当前处理帧率
    double latency_ms;        // 平均处理时延
    size_t mem_used;          // 内存占用
} mec_metrics_t;

// Memory management
void* mec_malloc(size_t size);
void* mec_calloc(size_t nmemb, size_t size);
void* mec_realloc(void *ptr, size_t size);
void mec_free(void *ptr);

// Track list utilities
track_list_t* track_list_create(int initial_capacity);
void track_list_retain(track_list_t *list);  // 增加引用
void track_list_release(track_list_t *list); // 减少引用（计数为0时释放）
int track_list_add(track_list_t *list, const target_track_t *track);
void track_list_clear(track_list_t *list);

#include "mec_logging.h"
#include "mec_thread.h"
#include "mec_config.h"

#endif // MEC_COMMON_H
