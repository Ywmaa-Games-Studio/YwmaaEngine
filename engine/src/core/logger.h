#pragma once

#include "defines.h"

#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#if KRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum E_LOG_LEVEL {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_TRACE = 4
} E_LOG_LEVEL;


/**
 * @brief Initializes logging system. Call twice; once with state = 0 to get required memory size,
 * then a second time passing allocated memory to state.
 * 
 * @param memory_requirement A pointer to hold the required memory size of internal state.
 * @param state 0 if just requesting memory requirement, otherwise allocated block of memory.
 * @return b8 True on success; otherwise false.
 */
b8 logging_init(u64* memory_requirement, void* state, void* config);
void logging_shutdown(void* state);

YAPI void log_output(E_LOG_LEVEL level, const char* message, ...);


#ifndef PRINT_ERROR
// Logs an error-level message.
#define PRINT_ERROR(...) log_output(LOG_LEVEL_ERROR, __VA_ARGS__);
#endif

#if LOG_WARNING_ENABLED == 1
// Logs a warning-level message.
#define PRINT_WARNING(...) log_output(LOG_LEVEL_WARNING, __VA_ARGS__);
#else
// Does nothing when LOG_WARNING_ENABLED != 1
#define PRINT_WARNING(...)
#endif

#if LOG_INFO_ENABLED == 1
// Logs a info-level message.
#define PRINT_INFO(...) log_output(LOG_LEVEL_INFO, __VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define PRINT_INFO(...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a debug-level message.
#define PRINT_DEBUG(...) log_output(LOG_LEVEL_DEBUG, __VA_ARGS__);
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#define PRINT_DEBUG(...)
#endif

#if LOG_TRACE_ENABLED == 1
/** 
 * @brief Logs a trace-level message. Should be used for verbose debugging purposes.
 * @param message The message to be logged.
 * @param ... Any formatted data that should be included in the log entry.
 */
#define PRINT_TRACE(...) log_output(LOG_LEVEL_TRACE, __VA_ARGS__);
#else
/** 
 * @brief Logs a trace-level message. Should be used for verbose debugging purposes.
 * Does nothing when LOG_TRACE_ENABLED != 1
 * @param message The message to be logged.
 * @param ... Any formatted data that should be included in the log entry.
 */
#define PRINT_TRACE(...)
#endif
