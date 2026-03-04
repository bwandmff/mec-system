#include "mec_watchdog.h"
#include "mec_logging.h"
#include <stdlib.h>
#include <unistd.h>

static void* watchdog_thread(void *arg) {
    watchdog_t *wd = (watchdog_t *)arg;
    
    LOG_INFO("Watchdog started (timeout: %ds, check interval: %dms)", 
             wd->timeout_seconds, wd->check_interval_ms);
    
    while (atomic_load(&wd->running)) {
        usleep(wd->check_interval_ms * 1000);
        
        time_t now = time(NULL);
        
        // 检查帧处理超时
        if (wd->last_frame_time > 0) {
            int frame_diff = now - wd->last_frame_time;
            if (frame_diff > wd->timeout_seconds) {
                LOG_ERROR("Watchdog: No frame processed for %d seconds! Triggering recovery...", 
                          frame_diff);
                if (wd->on_timeout) {
                    wd->on_timeout(wd->callback_arg);
                }
                // 重置看门狗状态
                wd->last_frame_time = now;
            }
        }
        
        // 检查心跳超时
        if (wd->last_heartbeat > 0) {
            int hb_diff = now - wd->last_heartbeat;
            if (hb_diff > wd->timeout_seconds * 2) {
                LOG_WARN("Watchdog: No heartbeat for %d seconds", hb_diff);
            }
        }
    }
    
    LOG_INFO("Watchdog stopped");
    return NULL;
}

watchdog_t* watchdog_create(int timeout_seconds, int check_interval_ms) {
    if (timeout_seconds <= 0 || check_interval_ms <= 0) {
        return NULL;
    }
    
    watchdog_t *wd = malloc(sizeof(watchdog_t));
    if (!wd) return NULL;
    
    wd->timeout_seconds = timeout_seconds;
    wd->check_interval_ms = check_interval_ms;
    wd->last_heartbeat = 0;
    wd->last_frame_time = 0;
    wd->frame_count = 0;
    wd->on_timeout = NULL;
    wd->callback_arg = NULL;
    atomic_init(&wd->running, 0);
    
    return wd;
}

int watchdog_start(watchdog_t *wd) {
    if (!wd) return -1;
    
    atomic_store(&wd->running, 1);
    wd->last_heartbeat = time(NULL);
    wd->last_frame_time = time(NULL);
    
    pthread_create(&wd->thread, NULL, watchdog_thread, wd);
    return 0;
}

void watchdog_stop(watchdog_t *wd) {
    if (!wd) return;
    
    atomic_store(&wd->running, 0);
    pthread_join(wd->thread, NULL);
}

void watchdog_destroy(watchdog_t *wd) {
    if (wd) {
        free(wd);
    }
}

void watchdog_feed(watchdog_t *wd) {
    if (wd) {
        wd->last_heartbeat = time(NULL);
    }
}

void watchdog_record_frame(watchdog_t *wd) {
    if (wd) {
        wd->last_frame_time = time(NULL);
        atomic_fetch_add(&wd->frame_count, 1);
    }
}

void watchdog_set_callback(watchdog_t *wd, void (*callback)(void *), void *arg) {
    if (wd) {
        wd->on_timeout = callback;
        wd->callback_arg = arg;
    }
}
