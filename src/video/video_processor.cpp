#include "mec_video.h"
#include <opencv2/opencv.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern "C" {

video_processor_t* video_processor_create(const video_config_t *config) {
    if (!config) return NULL;
    
    video_processor_t *processor = (video_processor_t*)mec_malloc(sizeof(video_processor_t));
    if (!processor) return NULL;
    
    processor->config = *config;
    processor->transform.calibrated = 0;
    processor->region_count = 0;
    processor->output_tracks = track_list_create(100);
    
    if (!processor->output_tracks) {
        mec_free(processor);
        return NULL;
    }
    
    LOG_INFO("Created video processor for camera %d", config->camera_id);
    return processor;
}

void video_processor_destroy(video_processor_t *processor) {
    if (!processor) return;
    
    video_processor_stop(processor);
    track_list_free(processor->output_tracks);
    mec_free(processor);
}

int video_processor_start(video_processor_t *processor) {
    if (!processor) return -1;
    
    if (thread_create(&processor->thread_ctx, video_processing_thread, processor) != 0) {
        LOG_ERROR("Failed to start video processing thread");
        return -1;
    }
    
    LOG_INFO("Started video processor for camera %d", processor->config.camera_id);
    return 0;
}

void video_processor_stop(video_processor_t *processor) {
    if (!processor) return;
    
    thread_destroy(&processor->thread_ctx);
    LOG_INFO("Stopped video processor for camera %d", processor->config.camera_id);
}

int video_processor_set_transform(video_processor_t *processor, const perspective_transform_t *transform) {
    if (!processor || !transform) return -1;
    
    processor->transform = *transform;
    LOG_INFO("Set perspective transform for camera %d", processor->config.camera_id);
    return 0;
}

int video_processor_add_region(video_processor_t *processor, const detection_region_t *region) {
    if (!processor || !region || processor->region_count >= 4) return -1;
    
    processor->regions[processor->region_count] = *region;
    processor->region_count++;
    LOG_INFO("Added detection region %d for camera %d", processor->region_count, processor->config.camera_id);
    return 0;
}

track_list_t* video_processor_get_tracks(video_processor_t *processor) {
    if (!processor) return NULL;
    return processor->output_tracks;
}

int transform_image_to_wgs84(const perspective_transform_t *transform, 
                           const image_coord_t *image_coord, 
                           wgs84_coord_t *wgs84_coord) {
    if (!transform || !image_coord || !wgs84_coord || !transform->calibrated) {
        return -1;
    }
    
    double x = image_coord->x;
    double y = image_coord->y;
    
    // Apply perspective transformation matrix
    double w = transform->matrix[6] * x + transform->matrix[7] * y + transform->matrix[8];
    if (fabs(w) < 1e-10) return -1;
    
    double world_x = (transform->matrix[0] * x + transform->matrix[1] * y + transform->matrix[2]) / w;
    double world_y = (transform->matrix[3] * x + transform->matrix[4] * y + transform->matrix[5]) / w;
    
    // Convert to WGS84 (simplified - in practice would need proper projection)
    wgs84_coord->latitude = world_y;
    wgs84_coord->longitude = world_x;
    wgs84_coord->altitude = 0.0;
    
    return 0;
}

void* video_processing_thread(void *arg) {
    video_processor_t *processor = (video_processor_t*)arg;
    if (!processor) return NULL;
    
    cv::VideoCapture cap;
    
    // Try to open RTSP stream
    if (!cap.open(processor->config.rtsp_url)) {
        LOG_ERROR("Failed to open RTSP stream: %s", processor->config.rtsp_url);
        return NULL;
    }
    
    cv::Mat frame;
    track_list_t *previous_tracks = track_list_create(100);
    
    while (processor->thread_ctx.running) {
        if (!cap.read(frame)) {
            LOG_WARN("Failed to read frame from camera %d", processor->config.camera_id);
            usleep(100000); // Wait 100ms before retry
            continue;
        }
        
        // Process frame
        thread_lock(&processor->thread_ctx);
        track_list_clear(processor->output_tracks);
        
        if (detect_targets(frame.data, frame.cols, frame.rows, processor->output_tracks) == 0) {
            track_targets(previous_tracks, processor->output_tracks);
            
            // Transform coordinates if calibrated
            if (processor->transform.calibrated) {
                for (int i = 0; i < processor->output_tracks->count; i++) {
                    image_coord_t img_coord = {
                        (int)(processor->output_tracks->tracks[i].position.longitude * frame.cols),
                        (int)(processor->output_tracks->tracks[i].position.latitude * frame.rows)
                    };
                    
                    wgs84_coord_t wgs84_coord;
                    if (transform_image_to_wgs84(&processor->transform, &img_coord, &wgs84_coord) == 0) {
                        processor->output_tracks->tracks[i].position = wgs84_coord;
                    }
                }
            }
        }
        
        // Copy current tracks to previous
        track_list_clear(previous_tracks);
        for (int i = 0; i < processor->output_tracks->count; i++) {
            track_list_add(previous_tracks, &processor->output_tracks->tracks[i]);
        }
        
        thread_unlock(&processor->thread_ctx);
        
        usleep(33333); // ~30 FPS
    }
    
    track_list_free(previous_tracks);
    cap.release();
    return NULL;
}

int detect_targets(const void *frame_data, int width, int height, track_list_t *tracks) {
    // Simplified target detection - in practice would use deep learning models
    // This is a placeholder implementation
    
    static int target_id = 0;
    target_track_t track;
    
    // Generate some dummy detections for demonstration
    for (int i = 0; i < 3; i++) {
        track.id = target_id++;
        track.type = TARGET_VEHICLE;
        track.position.latitude = 0.3 + i * 0.2;  // Normalized coordinates
        track.position.longitude = 0.4 + i * 0.1;
        track.position.altitude = 0.0;
        track.velocity = 10.0 + i * 5.0;
        track.heading = 45.0 + i * 30.0;
        track.confidence = 0.8 + i * 0.05;
        track.sensor_id = 1; // Video sensor
        gettimeofday(&track.timestamp, NULL);
        
        track_list_add(tracks, &track);
    }
    
    return 0;
}

int track_targets(track_list_t *previous_tracks, track_list_t *current_tracks) {
    // Simplified tracking - in practice would use Kalman filter or other algorithms
    // This is a placeholder that just maintains track IDs
    
    for (int i = 0; i < current_tracks->count; i++) {
        int matched = 0;
        for (int j = 0; j < previous_tracks->count; j++) {
            double dx = current_tracks->tracks[i].position.longitude - previous_tracks->tracks[j].position.longitude;
            double dy = current_tracks->tracks[i].position.latitude - previous_tracks->tracks[j].position.latitude;
            double distance = sqrt(dx*dx + dy*dy);
            
            if (distance < 0.1) { // Threshold for matching
                current_tracks->tracks[i].id = previous_tracks->tracks[j].id;
                matched = 1;
                break;
            }
        }
        
        if (!matched) {
            static int new_id = 1000;
            current_tracks->tracks[i].id = new_id++;
        }
    }
    
    return 0;
}

} // extern "C"