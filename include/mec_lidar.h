/*
 * mec_lidar.h - LiDAR 点云处理接口
 */

#ifndef MEC_LIDAR_H
#define MEC_LIDAR_H

#include "mec_common.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief LiDAR 点
 */
typedef struct {
    double x;           // X 坐标 (米)
    double y;           // Y 坐标 (米)
    double z;           // Z 坐标 (米)
    double intensity;   // 反射强度
    uint32_t timestamp; // 时间戳 (毫秒)
} lidar_point_t;

/**
 * @brief LiDAR 帧/点云
 */
typedef struct {
    uint64_t frame_id;           // 帧ID
    uint32_t point_count;       // 点数
    lidar_point_t *points;       // 点数组
    uint32_t timestamp;          // 接收时间戳
} lidar_frame_t;

/**
 * @brief LiDAR 配置
 */
typedef struct {
    char device_path[256];    // 设备路径 (文件或网络)
    int port;                 // 网络端口
    bool is_file;             // 是否为文件模式 (回放)
    int max_points;           // 最大点数
    double min_distance;     // 最小距离 (米)
    double max_distance;      // 最大距离 (米)
} lidar_config_t;

/**
 * @brief LiDAR 处理器
 */
typedef struct lidar_processor lidar_processor_t;

/**
 * @brief 创建 LiDAR 处理器
 */
lidar_processor_t* lidar_create(const lidar_config_t *config);

/**
 * @brief 销毁 LiDAR 处理器
 */
void lidar_destroy(lidar_processor_t *proc);

/**
 * @brief 启动 LiDAR
 */
int lidar_start(lidar_processor_t *proc);

/**
 * @brief 停止 LiDAR
 */
void lidar_stop(lidar_processor_t *proc);

/**
 * @brief 获取一帧点云
 * @return 0:成功, -1:超时
 */
int lidar_get_frame(lidar_processor_t *proc, lidar_frame_t *frame, int timeout_ms);

/**
 * @brief 释放帧内存
 */
void lidar_release_frame(lidar_frame_t *frame);

/**
 * @brief 简单的点云处理：地面过滤
 */
void lidar_filter_ground(lidar_frame_t *frame, double ground_height);

/**
 * @brief 点云降采样
 */
void lidar_downsample(lidar_frame_t *frame, double leaf_size);

/**
 * @brief 聚类分割 (简化版)
 */
int lidar_clustering(lidar_frame_t *frame, double cluster_distance, int min_points);

#endif // MEC_LIDAR_H
