#ifndef MEC_LOGGING_H
#define MEC_LOGGING_H

#include <stdarg.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

void log_init(const char *filename, log_level_t level);
void log_message(log_level_t level, const char *format, ...);
void log_cleanup(void);

#define LOG_DEBUG(...) log_message(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  log_message(LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __VA_ARGS__)

#endif // MEC_LOGGING_H
