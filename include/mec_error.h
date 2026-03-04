/*
 * mec_error.h - 统一错误码定义
 */

#ifndef MEC_ERROR_H
#define MEC_ERROR_H

#include <stdint.h>

/**
 * @brief MEC 系统错误码
 */
typedef enum {
    /* 成功 */
    MEC_OK = 0,
    
    /* 通用错误 (1-99) */
    MEC_ERROR = -1,
    MEC_ERROR_INVALID_PARAM = -2,
    MEC_ERROR_NULL_PTR = -3,
    MEC_ERROR_NO_MEMORY = -4,
    MEC_ERROR_TIMEOUT = -5,
    MEC_ERROR_BUSY = -6,
    MEC_ERROR_NOT_INITIALIZED = -7,
    MEC_ERROR_ALREADY_INITIALIZED = -8,
    
    /* 队列错误 (100-199) */
    MEC_ERROR_QUEUE_FULL = -100,
    MEC_ERROR_QUEUE_EMPTY = -101,
    MEC_ERROR_QUEUE_CREATE = -102,
    
    /* 内存错误 (200-299) */
    MEC_ERROR_MEM_POOL_EXHAUSTED = -200,
    MEC_ERROR_MEM_ALLOC = -201,
    MEC_ERROR_MEM_FREE = -202,
    
    /* 传感器错误 (300-399) */
    MEC_ERROR_SENSOR_CONNECT = -300,
    MEC_ERROR_SENSOR_DISCONNECT = -301,
    MEC_ERROR_SENSOR_TIMEOUT = -302,
    MEC_ERROR_SENSOR_DATA = -303,
    
    /* 融合错误 (400-499) */
    MEC_ERROR_FUSION_CREATE = -400,
    MEC_ERROR_FUSION_PROCESS = -401,
    MEC_ERROR_FUSION_TRACK_NOT_FOUND = -402,
    
    /* 配置错误 (500-599) */
    MEC_ERROR_CONFIG_LOAD = -500,
    MEC_ERROR_CONFIG_SAVE = -501,
    MEC_ERROR_CONFIG_INVALID = -502,
    
    /* 网络错误 (600-699) */
    MEC_ERROR_NET_CONNECT = -600,
    MEC_ERROR_NET_SEND = -601,
    MEC_ERROR_NET_RECV = -602,
    
    /* V2X 协议错误 (700-799) */
    MEC_ERROR_V2X_ENCODE = -700,
    MEC_ERROR_V2X_DECODE = -701,
    
    /* 线程错误 (800-899) */
    MEC_ERROR_THREAD_CREATE = -800,
    MEC_ERROR_THREAD_JOIN = -801,
} mec_error_t;

/**
 * @brief 获取错误码描述
 */
const char* mec_error_string(mec_error_t err);

/**
 * @brief 判断是否为严重错误（需要停止系统）
 */
int mec_error_is_fatal(mec_error_t err);

#endif // MEC_ERROR_H
