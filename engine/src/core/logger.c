#include "logger.h"
#include "asserts.h"
#include "platform/platform.h"

// TODO: temporary
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef struct LOGGER_SYSTEM_STATE {
    b8 initialized;
} LOGGER_SYSTEM_STATE;

static LOGGER_SYSTEM_STATE* state_ptr;

b8 init_logging(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(LOGGER_SYSTEM_STATE);
    if (state == 0) {
        return true;
    }

    state_ptr = state;
    state_ptr->initialized = true;

    // TODO: Remove this
    PRINT_ERROR("A test message: %f", 3.14f);
    PRINT_WARNING("A test message: %f", 3.14f);
    PRINT_INFO("A test message: %f", 3.14f);
    PRINT_DEBUG("A test message: %f", 3.14f);

    // TODO: create log file.
    return true;
}

void shutdown_logging(void* state) {
    // TODO: cleanup logging/write queued entries.
    state_ptr = 0;
}

void log_output(E_LOG_LEVEL level, const char* message, ...) {
    const char* level_strings[4] = {"[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: "};
    b8 is_error = level < LOG_LEVEL_WARNING;

    // Statically allocate memory for message instead of dynamically using malloc, for better performance.
    // Technically imposes a 32k character limit on a single log entry, but...
    // DON'T DO THAT!
    const i32 msg_length = 32000;
    char out_message[msg_length];
    memset(out_message, 0, sizeof(out_message));

    // Format original message.
    // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
    // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
    // which is the type GCC/Clang's va_start expects.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(out_message, msg_length, message, arg_ptr);
    va_end(arg_ptr);

    char out_message2[msg_length];
    sprintf(out_message2, "%s%s\n", level_strings[level], out_message);

    // Platform-specific output.
    if (is_error) {
        platform_console_write_error(out_message2, level);
    } else {
        platform_console_write(out_message2, level);
    }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_ERROR, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}