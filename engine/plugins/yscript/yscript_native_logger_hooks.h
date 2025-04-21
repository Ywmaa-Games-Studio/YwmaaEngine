#pragma once

#include "yscript.h"
#include "core/logger.h"
#include "stdio.h"

static yscript_value* print(yscript_context* context, yscript_value** args, u32 arg_count, E_LOG_LEVEL log_level) {

    for (u32 j = 0; j < arg_count; j++) {
        yscript_value* arg = args[j];
        if (arg->type->cast_to_string){
            RopeNode* result_text = arg->type->cast_to_string(arg->internal_data);
            char* flattened = rope_null_terminated_string(result_text);
            u64 length = rope_byte_length(result_text);
            switch (log_level)
            {
            case LOG_LEVEL_ERROR:
                PRINT_ERROR("%.*s", length, flattened );
                break;
            case LOG_LEVEL_INFO:
                PRINT_INFO("%.*s", length, flattened );
                break;
            case LOG_LEVEL_DEBUG:
                PRINT_DEBUG("%.*s", length, flattened );
                break;
            case LOG_LEVEL_WARNING:
                PRINT_WARNING("%.*s", length, flattened );
                break;
            case LOG_LEVEL_TRACE:
                PRINT_TRACE("%.*s", length, flattened );
                break;
            
            default:
                PRINT_ERROR("print_error: Unsupported Log Level");
                break;
            }
            yfree(flattened);
            rope_release(result_text);
            
        } else {
            PRINT_ERROR("print_error: Unsupported type");
        }
    }

    return NULL;
}

// Native function implementations
static yscript_value* native_print_error(yscript_context* context, yscript_value** args, u32 arg_count) {
    return print(context, args, arg_count, LOG_LEVEL_ERROR);
}

static yscript_value* native_print_warning(yscript_context* context, yscript_value** args, u32 arg_count) {
    return print(context, args, arg_count, LOG_LEVEL_WARNING);
}

static yscript_value* native_print_info(yscript_context* context, yscript_value** args, u32 arg_count) {
    return print(context, args, arg_count, LOG_LEVEL_INFO);
}

static yscript_value* native_print_debug(yscript_context* context, yscript_value** args, u32 arg_count) {
    return print(context, args, arg_count, LOG_LEVEL_DEBUG);
}

static yscript_value* native_print_trace(yscript_context* context, yscript_value** args, u32 arg_count) {
    return print(context, args, arg_count, LOG_LEVEL_TRACE);
}

static yscript_value* native_print(yscript_context* context, yscript_value** args, u32 arg_count) {
    return native_print_info(context, args, arg_count);
}
