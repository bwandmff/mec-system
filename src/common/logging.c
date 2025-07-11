#include "mec_common.h"
#include <stdarg.h>
#include <time.h>

static FILE *log_file = NULL;
static log_level_t current_log_level = LOG_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *log_level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void log_init(const char *filename, log_level_t level) {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file && log_file != stdout && log_file != stderr) {
        fclose(log_file);
    }
    
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            log_file = stderr;
            fprintf(stderr, "Failed to open log file %s, using stderr\n", filename);
        }
    } else {
        log_file = stdout;
    }
    
    current_log_level = level;
    pthread_mutex_unlock(&log_mutex);
}

void log_message(log_level_t level, const char *format, ...) {
    if (level < current_log_level || !log_file) {
        return;
    }
    
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_file, "[%s] %s: ", timestamp, log_level_strings[level]);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_cleanup() {
    pthread_mutex_lock(&log_mutex);
    if (log_file && log_file != stdout && log_file != stderr) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}