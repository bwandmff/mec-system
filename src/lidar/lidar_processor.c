/*
 * mec_lidar.c - LiDAR 点云处理实现
 */

#include "mec_lidar.h"
#include "mec_logging.h"
#include "mec_memory.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

struct lidar_processor {
    lidar_config_t config;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t reader_thread;
    atomic_int running;
    atomic_int has_frame;
    lidar_frame_t current_frame;
    FILE *file_fp;  // 文件回放模式
};

lidar_processor_t* lidar_create(const lidar_config_t *config) {
    if (!config) return NULL;
    
    lidar_processor_t *proc = calloc(1, sizeof(lidar_processor_t));
    if (!proc) return NULL;
    
    proc->config = *config;
    pthread_mutex_init(&proc->lock, NULL);
    pthread_cond_init(&proc->cond, NULL);
    atomic_init(&proc->running, 0);
    atomic_init(&proc->has_frame, 0);
    proc->file_fp = NULL;
    proc->current_frame.points = NULL;
    
    LOG_INFO("LiDAR: Created processor (max_points: %d)", config->max_points);
    
    return proc;
}

void lidar_destroy(lidar_processor_t *proc) {
    if (!proc) return;
    
    lidar_stop(proc);
    
    if (proc->current_frame.points) {
        free(proc->current_frame.points);
    }
    
    if (proc->file_fp) {
        fclose(proc->file_fp);
    }
    
    pthread_mutex_destroy(&proc->lock);
    pthread_cond_destroy(&proc->cond);
    free(proc);
    
    LOG_INFO("LiDAR: Destroyed processor");
}

int lidar_start(lidar_processor_t *proc) {
    if (!proc) return -1;
    if (atomic_load(&proc->running)) return 0;
    
    // 文件模式
    if (proc->config.is_file) {
        proc->file_fp = fopen(proc->config.device_path, "rb");
        if (!proc->file_fp) {
            LOG_ERROR("LiDAR: Failed to open file %s", proc->config.device_path);
            return -1;
        }
        LOG_INFO("LiDAR: Opened file %s for playback", proc->config.device_path);
    }
    
    atomic_store(&proc->running, 1);
    
    LOG_INFO("LiDAR: Started");
    return 0;
}

void lidar_stop(lidar_processor_t *proc) {
    if (!proc) return;
    if (!atomic_load(&proc->running)) return;
    
    atomic_store(&proc->running, 0);
    
    LOG_INFO("LiDAR: Stopped");
}

int lidar_get_frame(lidar_processor_t *proc, lidar_frame_t *frame, int timeout_ms) {
    if (!proc || !frame) return -1;
    
    // 简化实现：如果是文件模式，模拟读取
    if (proc->file_fp) {
        // 模拟生成点云数据
        static uint64_t frame_id = 0;
        
        frame->frame_id = frame_id++;
        frame->point_count = proc->config.max_points;
        frame->points = malloc(frame->point_count * sizeof(lidar_point_t));
        
        if (!frame->points) return -1;
        
        // 生成模拟点云（简化：随机分布的点）
        for (uint32_t i = 0; i < frame->point_count; i++) {
            double angle = (double)i / frame->point_count * 6.28318;
            double dist = 10.0 + (rand() % 100) / 10.0;
            
            frame->points[i].x = cos(angle) * dist;
            frame->points[i].y = sin(angle) * dist;
            frame->points[i].z = (rand() % 200) / 100.0 - 1.0;
            frame->points[i].intensity = 0.5 + (rand() % 50) / 100.0;
            frame->points[i].timestamp = (uint32_t)(frame_id * 100);
        }
        
        struct timeval tv;
        gettimeofday(&tv, 0);
        frame->timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        
        return 0;
    }
    
    // 实时模式：需要根据实际 LiDAR 协议实现
    LOG_WARN("LiDAR: Real-time mode not implemented, use file mode");
    return -1;
}

void lidar_release_frame(lidar_frame_t *frame) {
    if (!frame) return;
    
    if (frame->points) {
        free(frame->points);
        frame->points = NULL;
    }
    frame->point_count = 0;
}

void lidar_filter_ground(lidar_frame_t *frame, double ground_height) {
    if (!frame || !frame->points) return;
    
    uint32_t kept = 0;
    for (uint32_t i = 0; i < frame->point_count; i++) {
        if (frame->points[i].z > ground_height) {
            frame->points[kept++] = frame->points[i];
        }
    }
    
    frame->point_count = kept;
    LOG_DEBUG("LiDAR: Ground filter removed %d points", frame->point_count - kept);
}

void lidar_downsample(lidar_frame_t *frame, double leaf_size) {
    if (!frame || !frame->points || frame->point_count == 0) return;
    
    // 简化实现：随机降采样
    uint32_t step = (leaf_size < 1.0) ? 1 : (uint32_t)(leaf_size);
    uint32_t kept = 0;
    
    for (uint32_t i = 0; i < frame->point_count; i += step) {
        frame->points[kept++] = frame->points[i];
    }
    
    frame->point_count = kept;
    LOG_DEBUG("LiDAR: Downsampled to %d points", kept);
}

int lidar_clustering(lidar_frame_t *frame, double cluster_distance, int min_points) {
    if (!frame || !frame->points || frame->point_count == 0) return 0;
    
    // 简化实现：基于距离的聚类
    // 实际应该使用欧几里得聚类或 DBSCAN
    
    int cluster_count = 0;
    bool *visited = calloc(frame->point_count, sizeof(bool));
    
    if (!visited) return 0;
    
    for (uint32_t i = 0; i < frame->point_count; i++) {
        if (visited[i]) continue;
        
        // 简单聚类：找相邻点
        uint32_t cluster_size = 1;
        visited[i] = true;
        
        for (uint32_t j = i + 1; j < frame->point_count; j++) {
            if (visited[j]) continue;
            
            double dx = frame->points[i].x - frame->points[j].x;
            double dy = frame->points[i].y - frame->points[j].y;
            double dz = frame->points[i].z - frame->points[j].z;
            double dist = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (dist < cluster_distance) {
                visited[j] = true;
                cluster_size++;
            }
        }
        
        if (cluster_size >= min_points) {
            cluster_count++;
        }
    }
    
    free(visited);
    
    LOG_DEBUG("LiDAR: Found %d clusters", cluster_count);
    return cluster_count;
}
