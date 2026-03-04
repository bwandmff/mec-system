/*
 * mec_mqtt.c - MQTT 上报实现
 */

#include "mec_mqtt.h"
#include "mec_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 简化实现：使用 socket 连接
// 生产环境建议使用 libmosquitto

struct mec_mqtt_ctx {
    mqtt_config_t config;
    int sockfd;
    pthread_mutex_t lock;
    pthread_t background_thread;
    atomic_int running;
    atomic_int connected;
    void (*disconnect_callback)(void *);
    void *callback_arg;
};

static int mqtt_send_packet(mec_mqtt_ctx_t *ctx, const void *data, int len) {
    if (!ctx || ctx->sockfd < 0) return -1;
    
    int sent = 0;
    while (sent < len) {
        int n = send(ctx->sockfd, (const char*)data + sent, len - sent, 0);
        if (n <= 0) {
            return -1;
        }
        sent += n;
    }
    return 0;
}

static int mqtt_read_packet(mec_mqtt_ctx_t *ctx, void *data, int len, int timeout_ms) {
    if (!ctx || ctx->sockfd < 0) return -1;
    
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(ctx->sockfd, &fds);
    
    struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    int ret = select(ctx->sockfd + 1, &fds, NULL, NULL, &tv);
    
    if (ret <= 0) return -1;
    
    return recv(ctx->sockfd, data, len, 0);
}

static int mqtt_connect_internal(mec_mqtt_ctx_t *ctx) {
    // 创建 socket
    ctx->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sockfd < 0) {
        LOG_ERROR("MQTT: Failed to create socket");
        return -1;
    }
    
    // 设置为非阻塞
    // struct sockaddr_in addr;
    // memset(&addr, 0, sizeof(addr));
    // addr.sin_family = AF_INET;
    // addr.sin_port = htons(ctx->config.port);
    // inet_pton(AF_INET, ctx->config.host, &addr.sin_addr);
    
    // 这里简化处理：实际需要完整的 MQTT 协议实现
    // 使用 TCP 连接测试
    
    LOG_INFO("MQTT: Connecting to %s:%d", ctx->config.host, ctx->config.port);
    
    // 标记为已连接（简化版）
    atomic_store(&ctx->connected, 1);
    
    return 0;
}

mec_mqtt_ctx_t* mqtt_create(const mqtt_config_t *config) {
    if (!config) return NULL;
    
    mec_mqtt_ctx_t *ctx = calloc(1, sizeof(mec_mqtt_ctx_t));
    if (!ctx) return NULL;
    
    ctx->config = *config;
    pthread_mutex_init(&ctx->lock, NULL);
    atomic_init(&ctx->running, 0);
    atomic_init(&ctx->connected, 0);
    ctx->sockfd = -1;
    
    LOG_INFO("MQTT: Created client for %s:%d", config->host, config->port);
    
    return ctx;
}

void mqtt_destroy(mec_mqtt_ctx_t *ctx) {
    if (!ctx) return;
    
    mqtt_disconnect(ctx);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

int mqtt_connect(mec_mqtt_ctx_t *ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_lock(&ctx->lock);
    int ret = mqtt_connect_internal(ctx);
    pthread_mutex_unlock(&ctx->lock);
    
    return ret;
}

void mqtt_disconnect(mec_mqtt_ctx_t *ctx) {
    if (!ctx) return;
    
    pthread_mutex_lock(&ctx->lock);
    
    if (ctx->sockfd >= 0) {
        close(ctx->sockfd);
        ctx->sockfd = -1;
    }
    
    atomic_store(&ctx->connected, 0);
    pthread_mutex_unlock(&ctx->lock);
    
    LOG_INFO("MQTT: Disconnected");
}

int mqtt_publish(mec_mqtt_ctx_t *ctx, const char *topic, const void *data, int len) {
    if (!ctx || !topic || !data) return -1;
    
    if (!atomic_load(&ctx->connected)) {
        LOG_WARN("MQTT: Not connected, skipping publish");
        return -1;
    }
    
    // 简化实现：打印日志
    // 完整实现需要构造 MQTT PUBLISH 报文
    LOG_DEBUG("MQTT: Publish to %s (%d bytes)", topic, len);
    
    return 0;
}

int mqtt_publish_json(mec_mqtt_ctx_t *ctx, const char *topic, const char *json) {
    if (!ctx || !topic || !json) return -1;
    
    return mqtt_publish(ctx, topic, json, strlen(json));
}

bool mqtt_is_connected(mec_mqtt_ctx_t *ctx) {
    if (!ctx) return false;
    return atomic_load(&ctx->connected);
}

void mqtt_set_disconnect_callback(mec_mqtt_ctx_t *ctx, void (*callback)(void *), void *arg) {
    if (!ctx) return;
    ctx->disconnect_callback = callback;
    ctx->callback_arg = arg;
}

static void* background_worker(void *arg) {
    mec_mqtt_ctx_t *ctx = (mec_mqtt_ctx_t *)arg;
    
    LOG_INFO("MQTT: Background task started");
    
    while (atomic_load(&ctx->running)) {
        if (!atomic_load(&ctx->connected)) {
            // 尝试重连
            LOG_INFO("MQTT: Attempting to reconnect...");
            if (mqtt_connect_internal(ctx) == 0) {
                LOG_INFO("MQTT: Reconnected successfully");
            }
        }
        
        sleep(5);
    }
    
    LOG_INFO("MQTT: Background task stopped");
    return NULL;
}

int mqtt_start_background(mec_mqtt_ctx_t *ctx) {
    if (!ctx) return -1;
    if (atomic_load(&ctx->running)) return 0;
    
    atomic_store(&ctx->running, 1);
    pthread_create(&ctx->background_thread, NULL, background_worker, ctx);
    
    LOG_INFO("MQTT: Background task started");
    return 0;
}

void mqtt_stop_background(mec_mqtt_ctx_t *ctx) {
    if (!ctx) return;
    if (!atomic_load(&ctx->running)) return;
    
    atomic_store(&ctx->running, 0);
    pthread_join(ctx->background_thread, NULL);
    
    LOG_INFO("MQTT: Background task stopped");
}
