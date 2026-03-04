/*
 * mec_error.c - 错误码实现
 */

#include "mec_error.h"
#include <string.h>

typedef struct {
    mec_error_t code;
    const char *name;
    const char *description;
} error_entry_t;

static const error_entry_t error_table[] = {
    /* 成功 */
    {MEC_OK, "MEC_OK", "Success"},
    
    /* 通用错误 */
    {MEC_ERROR, "MEC_ERROR", "Generic error"},
    {MEC_ERROR_INVALID_PARAM, "MEC_ERROR_INVALID_PARAM", "Invalid parameter"},
    {MEC_ERROR_NULL_PTR, "MEC_ERROR_NULL_PTR", "Null pointer"},
    {MEC_ERROR_NO_MEMORY, "MEC_ERROR_NO_MEMORY", "Out of memory"},
    {MEC_ERROR_TIMEOUT, "MEC_ERROR_TIMEOUT", "Operation timeout"},
    {MEC_ERROR_BUSY, "MEC_ERROR_BUSY", "Resource busy"},
    {MEC_ERROR_NOT_INITIALIZED, "MEC_ERROR_NOT_INITIALIZED", "Not initialized"},
    {MEC_ERROR_ALREADY_INITIALIZED, "MEC_ERROR_ALREADY_INITIALIZED", "Already initialized"},
    
    /* 队列错误 */
    {MEC_ERROR_QUEUE_FULL, "MEC_ERROR_QUEUE_FULL", "Queue is full"},
    {MEC_ERROR_QUEUE_EMPTY, "MEC_ERROR_QUEUE_EMPTY", "Queue is empty"},
    {MEC_ERROR_QUEUE_CREATE, "MEC_ERROR_QUEUE_CREATE", "Failed to create queue"},
    
    /* 内存错误 */
    {MEC_ERROR_MEM_POOL_EXHAUSTED, "MEC_ERROR_MEM_POOL_EXHAUSTED", "Memory pool exhausted"},
    {MEC_ERROR_MEM_ALLOC, "MEC_ERROR_MEM_ALLOC", "Memory allocation failed"},
    {MEC_ERROR_MEM_FREE, "MEC_ERROR_MEM_FREE", "Memory free failed"},
    
    /* 传感器错误 */
    {MEC_ERROR_SENSOR_CONNECT, "MEC_ERROR_SENSOR_CONNECT", "Sensor connection failed"},
    {MEC_ERROR_SENSOR_DISCONNECT, "MEC_ERROR_SENSOR_DISCONNECT", "Sensor disconnected"},
    {MEC_ERROR_SENSOR_TIMEOUT, "MEC_ERROR_SENSOR_TIMEOUT", "Sensor timeout"},
    {MEC_ERROR_SENSOR_DATA, "MEC_ERROR_SENSOR_DATA", "Invalid sensor data"},
    
    /* 融合错误 */
    {MEC_ERROR_FUSION_CREATE, "MEC_ERROR_FUSION_CREATE", "Failed to create fusion processor"},
    {MEC_ERROR_FUSION_PROCESS, "MEC_ERROR_FUSION_PROCESS", "Fusion processing failed"},
    {MEC_ERROR_FUSION_TRACK_NOT_FOUND, "MEC_ERROR_FUSION_TRACK_NOT_FOUND", "Track not found"},
    
    /* 配置错误 */
    {MEC_ERROR_CONFIG_LOAD, "MEC_ERROR_CONFIG_LOAD", "Failed to load configuration"},
    {MEC_ERROR_CONFIG_SAVE, "MEC_ERROR_CONFIG_SAVE", "Failed to save configuration"},
    {MEC_ERROR_CONFIG_INVALID, "MEC_ERROR_CONFIG_INVALID", "Invalid configuration"},
    
    /* 网络错误 */
    {MEC_ERROR_NET_CONNECT, "MEC_ERROR_NET_CONNECT", "Network connection failed"},
    {MEC_ERROR_NET_SEND, "MEC_ERROR_NET_SEND", "Network send failed"},
    {MEC_ERROR_NET_RECV, "MEC_ERROR_NET_RECV", "Network receive failed"},
    
    /* V2X 错误 */
    {MEC_ERROR_V2X_ENCODE, "MEC_ERROR_V2X_ENCODE", "V2X encoding failed"},
    {MEC_ERROR_V2X_DECODE, "MEC_ERROR_V2X_DECODE", "V2X decoding failed"},
    
    /* 线程错误 */
    {MEC_ERROR_THREAD_CREATE, "MEC_ERROR_THREAD_CREATE", "Failed to create thread"},
    {MEC_ERROR_THREAD_JOIN, "MEC_ERROR_THREAD_JOIN", "Failed to join thread"},
};

const char* mec_error_string(mec_error_t err) {
    for (size_t i = 0; i < sizeof(error_table) / sizeof(error_entry_t); i++) {
        if (error_table[i].code == err) {
            return error_table[i].name;
        }
    }
    return "UNKNOWN_ERROR";
}

int mec_error_is_fatal(mec_error_t err) {
    switch (err) {
        case MEC_ERROR_NO_MEMORY:
        case MEC_ERROR_MEM_POOL_EXHAUSTED:
        case MEC_ERROR:
        case MEC_ERROR_THREAD_CREATE:
            return 1;
        default:
            return 0;
    }
}
