#include "mec_fusion.h"

fusion_processor_t* fusion_processor_create(const fusion_config_t *config) {
    if (!config) return NULL;
    
    fusion_processor_t *processor = mec_malloc(sizeof(fusion_processor_t));
    if (!processor) return NULL;
    
    processor->config = *config;
    processor->tracks = mec_malloc(100 * sizeof(fused_track_t));
    if (!processor->tracks) {
        mec_free(processor);
        return NULL;
    }
    
    processor->track_count = 0;
    processor->track_capacity = 100;
    processor->next_global_id = 1;
    processor->output_tracks = track_list_create(100);
    
    if (!processor->output_tracks) {
        mec_free(processor->tracks);
        mec_free(processor);
        return NULL;
    }
    
    LOG_INFO("Created fusion processor");
    return processor;
}

void fusion_processor_destroy(fusion_processor_t *processor) {
    if (!processor) return;
    
    fusion_processor_stop(processor);
    mec_free(processor->tracks);
    track_list_free(processor->output_tracks);
    mec_free(processor);
}

int fusion_processor_start(fusion_processor_t *processor) {
    if (!processor) return -1;
    
    if (thread_create(&processor->thread_ctx, fusion_processing_thread, processor) != 0) {
        LOG_ERROR("Failed to start fusion processing thread");
        return -1;
    }
    
    LOG_INFO("Started fusion processor");
    return 0;
}

void fusion_processor_stop(fusion_processor_t *processor) {
    if (!processor) return;
    
    thread_destroy(&processor->thread_ctx);
    LOG_INFO("Stopped fusion processor");
}

int fusion_processor_add_tracks(fusion_processor_t *processor, 
                               const track_list_t *tracks, 
                               int sensor_id) {
    if (!processor || !tracks) return -1;
    
    thread_lock(&processor->thread_ctx);
    
    // Process each incoming track
    for (int i = 0; i < tracks->count; i++) {
        const target_track_t *sensor_track = &tracks->tracks[i];
        
        // Find best matching fused track
        int best_match = -1;
        double best_distance = processor->config.association_threshold;
        
        for (int j = 0; j < processor->track_count; j++) {
            double distance = calculate_track_distance(&processor->tracks[j], sensor_track);
            if (distance < best_distance) {
                best_distance = distance;
                best_match = j;
            }
        }
        
        if (best_match >= 0) {
            // Update existing track
            update_fused_track(&processor->tracks[best_match], sensor_track);
            processor->tracks[best_match].sensor_mask |= (1 << sensor_id);
        } else {
            // Create new track
            if (processor->track_count < processor->track_capacity) {
                fused_track_t *new_track = &processor->tracks[processor->track_count];
                new_track->global_id = processor->next_global_id++;
                new_track->type = sensor_track->type;
                new_track->confidence = sensor_track->confidence;
                new_track->age = 0;
                new_track->sensor_mask = (1 << sensor_id);
                new_track->last_update = sensor_track->timestamp;
                
                // Initialize Kalman filter
                initialize_kalman_filter(&new_track->filter_state, sensor_track);
                
                processor->track_count++;
                LOG_DEBUG("Created new fused track %d", new_track->global_id);
            }
        }
    }
    
    thread_unlock(&processor->thread_ctx);
    return 0;
}

track_list_t* fusion_processor_get_tracks(fusion_processor_t *processor) {
    if (!processor) return NULL;
    return processor->output_tracks;
}

void* fusion_processing_thread(void *arg) {
    fusion_processor_t *processor = (fusion_processor_t*)arg;
    if (!processor) return NULL;
    
    while (processor->thread_ctx.running) {
        thread_lock(&processor->thread_ctx);
        
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        
        track_list_clear(processor->output_tracks);
        
        // Process all fused tracks
        for (int i = 0; i < processor->track_count; i++) {
            fused_track_t *track = &processor->tracks[i];
            
            // Calculate time since last update
            double dt = (current_time.tv_sec - track->last_update.tv_sec) + 
                       (current_time.tv_usec - track->last_update.tv_usec) / 1000000.0;
            
            // Predict track state
            predict_track_state(track, dt);
            
            // Age the track
            track->age++;
            
            // Check if track should be removed
            if (track->age > processor->config.max_track_age || 
                track->confidence < processor->config.confidence_threshold) {
                // Remove track by moving last track to this position
                if (i < processor->track_count - 1) {
                    processor->tracks[i] = processor->tracks[processor->track_count - 1];
                    i--; // Reprocess this index
                }
                processor->track_count--;
                continue;
            }
            
            // Convert to output format
            target_track_t output_track;
            output_track.id = track->global_id;
            output_track.type = track->type;
            output_track.position.latitude = track->filter_state.state[1];
            output_track.position.longitude = track->filter_state.state[0];
            output_track.position.altitude = 0.0;
            output_track.velocity = sqrt(track->filter_state.state[2] * track->filter_state.state[2] + 
                                        track->filter_state.state[3] * track->filter_state.state[3]);
            output_track.heading = atan2(track->filter_state.state[3], track->filter_state.state[2]) * 180.0 / M_PI;
            output_track.confidence = track->confidence;
            output_track.sensor_id = -1; // Fused track
            output_track.timestamp = current_time;
            
            track_list_add(processor->output_tracks, &output_track);
        }
        
        thread_unlock(&processor->thread_ctx);
        
        usleep(50000); // 50ms processing cycle
    }
    
    return NULL;
}

int update_fused_track(fused_track_t *fused_track, const target_track_t *sensor_track) {
    if (!fused_track || !sensor_track) return -1;
    
    // Update Kalman filter with new measurement
    update_kalman_filter(&fused_track->filter_state, sensor_track);
    
    // Update track properties
    fused_track->confidence = (fused_track->confidence + sensor_track->confidence) / 2.0;
    fused_track->age = 0; // Reset age on update
    fused_track->last_update = sensor_track->timestamp;
    
    return 0;
}

int predict_track_state(fused_track_t *track, double dt) {
    if (!track) return -1;
    
    kalman_state_t *state = &track->filter_state;
    
    // Simple constant velocity model prediction
    // State: [x, y, vx, vy, ax, ay]
    state->state[0] += state->state[2] * dt + 0.5 * state->state[4] * dt * dt;
    state->state[1] += state->state[3] * dt + 0.5 * state->state[5] * dt * dt;
    state->state[2] += state->state[4] * dt;
    state->state[3] += state->state[5] * dt;
    
    // Update covariance matrix (simplified)
    for (int i = 0; i < 36; i++) {
        state->covariance[i] += 0.1 * dt; // Process noise
    }
    
    return 0;
}

int initialize_kalman_filter(kalman_state_t *state, const target_track_t *track) {
    if (!state || !track) return -1;
    
    // Initialize state vector
    state->state[0] = track->position.longitude;
    state->state[1] = track->position.latitude;
    state->state[2] = track->velocity * cos(track->heading * M_PI / 180.0);
    state->state[3] = track->velocity * sin(track->heading * M_PI / 180.0);
    state->state[4] = 0.0; // No initial acceleration
    state->state[5] = 0.0;
    
    // Initialize covariance matrix
    for (int i = 0; i < 36; i++) {
        state->covariance[i] = 0.0;
    }
    
    // Set diagonal values
    state->covariance[0] = 1.0;   // x position variance
    state->covariance[7] = 1.0;   // y position variance
    state->covariance[14] = 0.5;  // x velocity variance
    state->covariance[21] = 0.5;  // y velocity variance
    state->covariance[28] = 0.1;  // x acceleration variance
    state->covariance[35] = 0.1;  // y acceleration variance
    
    state->last_update = track->timestamp;
    state->initialized = 1;
    
    return 0;
}

int update_kalman_filter(kalman_state_t *state, const target_track_t *measurement) {
    if (!state || !measurement || !state->initialized) return -1;
    
    // Simplified Kalman filter update
    // In practice, would implement full matrix operations
    
    double innovation_x = measurement->position.longitude - state->state[0];
    double innovation_y = measurement->position.latitude - state->state[1];
    
    double gain = 0.3; // Simplified Kalman gain
    
    // Update state
    state->state[0] += gain * innovation_x;
    state->state[1] += gain * innovation_y;
    
    // Update velocity estimates
    double dt = (measurement->timestamp.tv_sec - state->last_update.tv_sec) + 
               (measurement->timestamp.tv_usec - state->last_update.tv_usec) / 1000000.0;
    
    if (dt > 0) {
        double vx = measurement->velocity * cos(measurement->heading * M_PI / 180.0);
        double vy = measurement->velocity * sin(measurement->heading * M_PI / 180.0);
        
        state->state[2] = (1.0 - gain) * state->state[2] + gain * vx;
        state->state[3] = (1.0 - gain) * state->state[3] + gain * vy;
    }
    
    // Update covariance (simplified)
    for (int i = 0; i < 36; i++) {
        state->covariance[i] *= (1.0 - gain);
    }
    
    state->last_update = measurement->timestamp;
    
    return 0;
}

double calculate_track_distance(const fused_track_t *track1, const target_track_t *track2) {
    if (!track1 || !track2) return 1e10;
    
    double dx = track1->filter_state.state[0] - track2->position.longitude;
    double dy = track1->filter_state.state[1] - track2->position.latitude;
    double dvx = track1->filter_state.state[2] - track2->velocity * cos(track2->heading * M_PI / 180.0);
    double dvy = track1->filter_state.state[3] - track2->velocity * sin(track2->heading * M_PI / 180.0);
    
    double position_distance = sqrt(dx * dx + dy * dy);
    double velocity_distance = sqrt(dvx * dvx + dvy * dvy);
    
    return position_distance + 0.1 * velocity_distance;
}