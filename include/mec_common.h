#ifndef MEC_COMMON_H
#define MEC_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>

// Common data structures
typedef struct {
    double latitude;
    double longitude;
    double altitude;
} wgs84_coord_t;

typedef struct {
    int x;
    int y;
} image_coord_t;

typedef enum {
    TARGET_VEHICLE = 0,
    TARGET_NON_VEHICLE = 1,
    TARGET_PEDESTRIAN = 2,
    TARGET_OBSTACLE = 3
} target_type_t;

typedef struct {
    int id;
    target_type_t type;
    wgs84_coord_t position;
    double velocity;
    double heading;
    double confidence;
    struct timeval timestamp;
    int sensor_id;
} target_track_t;

typedef struct {
    target_track_t *tracks;
    int count;
    int capacity;
} track_list_t;

// Memory management
void* mec_malloc(size_t size);
void* mec_calloc(size_t nmemb, size_t size);
void* mec_realloc(void *ptr, size_t size);
void mec_free(void *ptr);

// Configuration management
typedef struct config_t config_t;
config_t* config_load(const char *filename);
void config_free(config_t *config);
const char* config_get_string(config_t *config, const char *key, const char *default_value);
int config_get_int(config_t *config, const char *key, int default_value);
double config_get_double(config_t *config, const char *key, double default_value);

// Logging
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} log_level_t;

void log_init(const char *filename, log_level_t level);
void log_message(log_level_t level, const char *format, ...);
void log_cleanup();

#define LOG_DEBUG(...) log_message(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __VA_ARGS__)

// Thread utilities
typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int running;
    void *data;
} thread_context_t;

int thread_create(thread_context_t *ctx, void *(*start_routine)(void*), void *arg);
void thread_destroy(thread_context_t *ctx);
void thread_lock(thread_context_t *ctx);
void thread_unlock(thread_context_t *ctx);
void thread_wait(thread_context_t *ctx);
void thread_signal(thread_context_t *ctx);

// Track list utilities
track_list_t* track_list_create(int initial_capacity);
void track_list_free(track_list_t *list);
int track_list_add(track_list_t *list, const target_track_t *track);
void track_list_clear(track_list_t *list);

#endif // MEC_COMMON_H