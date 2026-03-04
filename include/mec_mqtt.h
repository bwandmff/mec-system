/*
 * mec_mqtt.h - MQTT 上报接口
 */

#ifndef MEC_MQTT_H
#define MEC_MQTT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief MQTT 配置
 */
typedef struct {
    char host[128];         // Broker 地址
    int port;               // Broker 端口
    char client_id[64];     // 客户端ID
    char username[64];      // 用户名
    char password[64];      // 密码
    char topic[128];        // 上报主题
    int qos;                // QoS 级别 (0,1,2)
    bool retain;             // Retain 消息
    int keepalive;          // Keep-alive 秒数
} mqtt_config_t;

/**
 * @brief MQTT 上下文
 */
typedef struct mec_mqtt_ctx mec_mqtt_ctx_t;

/**
 * @brief 创建 MQTT 客户端
 */
mec_mqtt_ctx_t* mqtt_create(const mqtt_config_t *config);

/**
 * @brief 销毁 MQTT 客户端
 */
void mqtt_destroy(mec_mqtt_ctx_t *ctx);

/**
 * @brief 连接到 Broker
 */
int mqtt_connect(mec_mqtt_ctx_t *ctx);

/**
 * @brief 断开连接
 */
void mqtt_disconnect(mec_mqtt_ctx_t *ctx);

/**
 * @brief 发布消息
 * @return 0:成功, -1:失败
 */
int mqtt_publish(mec_mqtt_ctx_t *ctx, const char *topic, const void *data, int len);

/**
 * @brief 发布 JSON 格式的传感器数据
 */
int mqtt_publish_json(mec_mqtt_ctx_t *ctx, const char *topic, const char *json);

/**
 * @brief 获取连接状态
 */
bool mqtt_is_connected(mec_mqtt_ctx_t *ctx);

/**
 * @brief 设置断开连接回调
 */
void mqtt_set_disconnect_callback(mec_mqtt_ctx_t *ctx, void (*callback)(void *), void *arg);

/**
 * @brief 启动后台任务（自动重连）
 */
int mqtt_start_background(mec_mqtt_ctx_t *ctx);

/**
 * @brief 停止后台任务
 */
void mqtt_stop_background(mec_mqtt_ctx_t *ctx);

#endif // MEC_MQTT_H
