#include "mec_common.h"
#include "mec_video.h"
#include "mec_radar.h"
#include "mec_fusion.h"
#include "mec_simulator.h"
#include "mec_queue.h"
#include "mec_v2x.h"
#include <signal.h>

static int running = 1;
static int reload_config = 0;

// 信号处理函数：确保系统能够安全退出
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        LOG_INFO("Received shutdown signal, exiting...");
    } else if (sig == SIGHUP) {
        reload_config = 1;
        LOG_INFO("Received SIGHUP, reloading configuration...");
    }
}

int main(int argc, char *argv[]) {
    int sim_mode = 0;
    char *config_path = "/etc/mec/mec.conf";

    // 1. 命令行参数解析
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sim") == 0 || strcmp(argv[i], "-s") == 0) {
            sim_mode = 1;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            config_path = argv[++i];
        }
    }

    // 2. 初始化日志系统
    log_init("/var/log/mec_system.log", LOG_INFO);
    LOG_INFO("MEC System starting... (Mode: %s)", sim_mode ? "Simulation" : "Real Sensors");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 3. 加载配置文件
    config_t *config = config_load(config_path);
    if (!config) {
        LOG_WARN("Failed to load configuration from %s", config_path);
        if (!sim_mode) return 1;
    }
    
    // 4. 创建全局异步消息队列 (容量设为 50)
    mec_queue_t *msg_queue = mec_queue_create(50);
    if (!msg_queue) {
        LOG_ERROR("Failed to create message queue");
        return 1;
    }

    // 5. 初始化融合引擎配置
    fusion_config_t fusion_cfg = {0};
    if (config) {
        fusion_cfg.association_threshold = config_get_double(config, "fusion.association_threshold", 5.0);
        fusion_cfg.position_weight = config_get_double(config, "fusion.position_weight", 1.0);
        fusion_cfg.velocity_weight = config_get_double(config, "fusion.velocity_weight", 0.1);
        fusion_cfg.confidence_threshold = config_get_double(config, "fusion.confidence_threshold", 0.3);
        fusion_cfg.max_track_age = config_get_int(config, "fusion.max_track_age", 50);
    } else {
        fusion_cfg.association_threshold = 5.0;
        fusion_cfg.confidence_threshold = 0.3;
        fusion_cfg.max_track_age = 50;
    }

    fusion_processor_t *fusion_proc = fusion_processor_create(&fusion_cfg);
    video_processor_t *video_proc = NULL;
    radar_processor_t *radar_proc = NULL;
    mec_simulator_t *simulator = NULL;

    // 6. 启动数据源（模拟器或真实传感器）
    if (sim_mode) {
        simulator_config_t sim_cfg = {
            .playback_speed = 1.0,
            .loop = 1
        };
        const char *sim_data = (config) ? config_get_string(config, "sim.data_path", "config/scenario_test.txt") : "config/scenario_test.txt";
        strncpy(sim_cfg.data_path, sim_data, sizeof(sim_cfg.data_path) - 1);
        
        simulator = simulator_create(&sim_cfg);
        // 注意：这里需要给模拟器也适配队列，或者在主循环里手动入队（为了演示一致性，我们在主循环入队）
        if (!simulator || simulator_start(simulator) != 0) {
            LOG_ERROR("Failed to start simulator");
            goto cleanup;
        }
    } else {
        // 真实传感器模式：将队列句柄传入配置，实现生产者模式
        video_config_t video_cfg = {0};
        strncpy(video_cfg.rtsp_url, config_get_string(config, "video.rtsp_url", "rtsp://192.168.1.100:554/stream"), sizeof(video_cfg.rtsp_url) - 1);
        video_cfg.camera_id = 1;
        video_cfg.target_queue = msg_queue; // 绑定异步队列
        
        radar_config_t radar_cfg = {0};
        strncpy(radar_cfg.device_path, config_get_string(config, "radar.device_path", "/dev/ttyUSB0"), sizeof(radar_cfg.device_path) - 1);
        radar_cfg.radar_id = 2;
        radar_cfg.target_queue = msg_queue; // 绑定异步队列
        
        video_proc = video_processor_create(&video_cfg);
        radar_proc = radar_processor_create(&radar_cfg);
        
        if (video_processor_start(video_proc) != 0 || radar_processor_start(radar_proc) != 0) {
            LOG_ERROR("Failed to start sensor threads");
            goto cleanup;
        }
    }

    // 7. 启动融合处理器线程
    if (fusion_processor_start(fusion_proc) != 0) {
        LOG_ERROR("Failed to start fusion processor");
        goto cleanup;
    }
    
    LOG_INFO("MEC System Running in Asynchronous Mode (Queue: %d msgs limit)", 50);
    
    // 8. 核心消息循环 (消费者模式)
    while (running) {
        // --- 检查并处理配置重载 ---
        if (reload_config) {
            config_t *new_cfg = config_load(config_path);
            if (new_cfg) {
                // 更新融合参数（示例）
                fusion_cfg.association_threshold = config_get_double(new_cfg, "fusion.association_threshold", 5.0);
                fusion_cfg.confidence_threshold = config_get_double(new_cfg, "fusion.confidence_threshold", 0.3);
                LOG_INFO("Configuration reloaded (New Association Threshold: %.2f)", fusion_cfg.association_threshold);
                config_free(config);
                config = new_cfg;
            }
            reload_config = 0;
        }

        mec_msg_t incoming_msg;
        
        // 从队列中弹出数据，设置 500ms 超时，避免死等
        if (mec_queue_pop(msg_queue, &incoming_msg, 500) == 0) {
            // 拿到数据，立刻投喂给融合引擎
            fusion_processor_add_tracks(fusion_proc, incoming_msg.tracks, incoming_msg.sensor_id);
            
            // 重要：队列 pop 出来的 tracks 所有权转移给了主循环，处理完需手动释放
            track_list_free(incoming_msg.tracks);
            
            // 实时输出结果
            track_list_t *fused = fusion_processor_get_tracks(fusion_proc);
            if (fused && fused->count > 0) {
                printf("\r[LIVE] Fused Targets: %d | Last Source: %d   ", fused->count, incoming_msg.sensor_id);
                fflush(stdout);

                // --- 新增：V2X 标准消息编码 ---
                uint8_t v2x_buffer[2048];
                int v2x_len = sizeof(v2x_buffer);
                if (v2x_encode_rsm(fused, 0xABCD, v2x_buffer, &v2x_len) == 0) {
                    LOG_DEBUG("V2X: Encoded RSM packet (%d bytes) ready for broadcast", v2x_len);
                }
            }
        } else {
            // 队列空，打印心跳状态
            static time_t last_hb = 0;
            time_t now = time(NULL);
            if (now - last_hb >= 5) {
                LOG_INFO("System Heartbeat: [Queue Size: %d] [Active Tracks: %d]", 
                         mec_queue_size(msg_queue), fusion_proc->track_count);
                last_hb = now;
            }

            if (sim_mode) {
                // 模拟器特殊处理：主动拉取
                track_list_t *vt = simulator_get_video_tracks(simulator);
                if (vt && vt->count > 0) {
                    fusion_processor_add_tracks(fusion_proc, vt, 1);
                    track_list_clear(vt);
                }
            }
        }
    }
    
cleanup:
    LOG_INFO("MEC System shutting down...");
    if (simulator) simulator_destroy(simulator);
    if (video_proc) { video_processor_stop(video_proc); video_processor_destroy(video_proc); }
    if (radar_proc) { radar_processor_stop(radar_proc); radar_processor_destroy(radar_proc); }
    if (fusion_proc) { fusion_processor_stop(fusion_proc); fusion_processor_destroy(fusion_proc); }
    if (msg_queue) mec_queue_destroy(msg_queue);
    if (config) config_free(config);
    log_cleanup();
    return 0;
}
