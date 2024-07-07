#pragma once

#include "defines.h"

#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1

#if YRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#endif

typedef enum LOG_LEVEL {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} LOG_LEVEL;


b8 init_logging();
void shutdown_logging();

YAPI void log_output(LOG_LEVEL level, const char* message, ...);


#ifndef PRINT_ERROR
// Logs an error-level message.
#define PRINT_ERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
// Logs a warning-level message.
#define PRINT_WARNING(message, ...) log_output(LOG_LEVEL_WARNING, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_WARN_ENABLED != 1
#define PRINT_WARNING(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
// Logs a info-level message.
#define PRINT_INFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define PRINT_INFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a debug-level message.
#define PRINT_DEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#define PRINT_DEBUG(message, ...)
#endif