#include "mec_common.h"
#include "mec_video.h"
#include "mec_radar.h"
#include "mec_fusion.h"
#include <signal.h>

static int running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        LOG_INFO("Received shutdown signal");
    }
}

int main(int argc, char *argv[]) {
    // Initialize logging
    log_init("/var/log/mec_system.log", LOG_INFO);
    LOG_INFO("MEC System starting...");
    
    // Install signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Load configuration
    config_t *config = config_load("/etc/mec/mec.conf");
    if (!config) {
        LOG_ERROR("Failed to load configuration");
        return 1;
    }
    
    // Create processors
    video_config_t video_cfg = {0};
    strncpy(video_cfg.rtsp_url, config_get_string(config, "video.rtsp_url", "rtsp://192.168.1.100:554/stream"), sizeof(video_cfg.rtsp_url) - 1);
    video_cfg.width = config_get_int(config, "video.width", 1920);
    video_cfg.height = config_get_int(config, "video.height", 1080);
    video_cfg.fps = config_get_int(config, "video.fps", 30);
    video_cfg.camera_id = config_get_int(config, "video.camera_id", 1);
    
    radar_config_t radar_cfg = {0};
    strncpy(radar_cfg.device_path, config_get_string(config, "radar.device_path", "/dev/ttyUSB0"), sizeof(radar_cfg.device_path) - 1);
    radar_cfg.baud_rate = config_get_int(config, "radar.baud_rate", 115200);
    radar_cfg.radar_id = config_get_int(config, "radar.radar_id", 2);
    radar_cfg.range_resolution = config_get_double(config, "radar.range_resolution", 0.1);
    radar_cfg.angle_resolution = config_get_double(config, "radar.angle_resolution", 1.0);
    radar_cfg.max_range = config_get_double(config, "radar.max_range", 200.0);
    
    fusion_config_t fusion_cfg = {0};
    fusion_cfg.association_threshold = config_get_double(config, "fusion.association_threshold", 5.0);
    fusion_cfg.position_weight = config_get_double(config, "fusion.position_weight", 1.0);
    fusion_cfg.velocity_weight = config_get_double(config, "fusion.velocity_weight", 0.1);
    fusion_cfg.confidence_threshold = config_get_double(config, "fusion.confidence_threshold", 0.3);
    fusion_cfg.max_track_age = config_get_int(config, "fusion.max_track_age", 50);
    
    // Create processor instances
    video_processor_t *video_proc = video_processor_create(&video_cfg);
    radar_processor_t *radar_proc = radar_processor_create(&radar_cfg);
    fusion_processor_t *fusion_proc = fusion_processor_create(&fusion_cfg);
    
    if (!video_proc || !radar_proc || !fusion_proc) {
        LOG_ERROR("Failed to create processors");
        goto cleanup;
    }
    
    // Start processors
    if (video_processor_start(video_proc) != 0 ||
        radar_processor_start(radar_proc) != 0 ||
        fusion_processor_start(fusion_proc) != 0) {
        LOG_ERROR("Failed to start processors");
        goto cleanup;
    }
    
    LOG_INFO("MEC System started successfully");
    
    // Main processing loop
    while (running) {
        // Get tracks from sensors
        track_list_t *video_tracks = video_processor_get_tracks(video_proc);
        track_list_t *radar_tracks = radar_processor_get_tracks(radar_proc);
        
        // Send tracks to fusion processor
        if (video_tracks && video_tracks->count > 0) {
            fusion_processor_add_tracks(fusion_proc, video_tracks, 1);
        }
        
        if (radar_tracks && radar_tracks->count > 0) {
            fusion_processor_add_tracks(fusion_proc, radar_tracks, 2);
        }
        
        // Get fused tracks
        track_list_t *fused_tracks = fusion_processor_get_tracks(fusion_proc);
        if (fused_tracks && fused_tracks->count > 0) {
            LOG_DEBUG("Processing %d fused tracks", fused_tracks->count);
            
            // Here you would send fused tracks to RSU or other downstream systems
            // For now, just log the information
            for (int i = 0; i < fused_tracks->count; i++) {
                target_track_t *track = &fused_tracks->tracks[i];
                LOG_DEBUG("Track %d: type=%d, pos=(%.6f,%.6f), vel=%.2f, heading=%.1f, conf=%.2f",
                         track->id, track->type, track->position.latitude, track->position.longitude,
                         track->velocity, track->heading, track->confidence);
            }
        }
        
        usleep(100000); // 100ms main loop
    }
    
cleanup:
    LOG_INFO("MEC System shutting down...");
    
    // Stop processors
    if (video_proc) {
        video_processor_stop(video_proc);
        video_processor_destroy(video_proc);
    }
    
    if (radar_proc) {
        radar_processor_stop(radar_proc);
        radar_processor_destroy(radar_proc);
    }
    
    if (fusion_proc) {
        fusion_processor_stop(fusion_proc);
        fusion_processor_destroy(fusion_proc);
    }
    
    // Cleanup
    config_free(config);
    log_cleanup();
    
    LOG_INFO("MEC System stopped");
    return 0;
}