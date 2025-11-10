#pragma once

#include "defines.h"
#include "logger.h"

typedef b8 (*PFN_console_consumer_write)(void* inst, E_LOG_LEVEL level, const char* message);

typedef struct CONSOLE_COMMAND_ARGUMENT {
    const char* value;
} CONSOLE_COMMAND_ARGUMENT;

typedef struct CONSOLE_COMMAND_CONTEXT {
    u8 argument_count;
    CONSOLE_COMMAND_ARGUMENT* arguments;
} CONSOLE_COMMAND_CONTEXT;

typedef void (*PFN_console_command)(CONSOLE_COMMAND_CONTEXT context);

b8 console_init(u64* memory_requirement, void* memory, void* config);
void console_shutdown(void* state);

YAPI void console_register_consumer(void* inst, PFN_console_consumer_write callback);

void console_write_line(E_LOG_LEVEL level, const char* message);

YAPI b8 console_register_command(const char* command, u8 arg_count, PFN_console_command func);

YAPI b8 console_execute_command(const char* command);
