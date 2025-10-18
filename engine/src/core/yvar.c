#include "yvar.h"

#include "core/ymemory.h"
#include "core/logger.h"
#include "core/ystring.h"

#include "core/console.h"

typedef struct yvar_int_entry {
    const char* name;
    i32 value;
} yvar_int_entry;

#define YVAR_INT_MAX_COUNT 200

typedef struct YVAR_SYSTEM_STATE {
    // Integer yvars.
    yvar_int_entry ints[YVAR_INT_MAX_COUNT];
} YVAR_SYSTEM_STATE;

static YVAR_SYSTEM_STATE* state_ptr;

void yvar_register_console_commands(void);

b8 yvar_init(u64* memory_requirement, void* memory) {
    *memory_requirement = sizeof(YVAR_SYSTEM_STATE);

    if (!memory) {
        return true;
    }

    state_ptr = memory;

    yzero_memory(state_ptr, sizeof(YVAR_SYSTEM_STATE));

    yvar_register_console_commands();

    return true;
}
void yvar_shutdown(void* state) {
    if (state_ptr) {
        yzero_memory(state_ptr, sizeof(YVAR_SYSTEM_STATE));
    }
}

b8 yvar_create_int(const char* name, i32 value) {
    if (!state_ptr || !name) {
        return false;
    }

    for (u32 i = 0; i < YVAR_INT_MAX_COUNT; ++i) {
        yvar_int_entry* entry = &state_ptr->ints[i];
        if (entry->name && strings_equali(entry->name, name)) {
            PRINT_ERROR("A int yvar named '%s' already exists.", name);
            return false;
        }
    }

    for (u32 i = 0; i < YVAR_INT_MAX_COUNT; ++i) {
        yvar_int_entry* entry = &state_ptr->ints[i];
        if (!entry->name) {
            entry->name = string_duplicate(name);
            entry->value = value;
            return true;
        }
    }

    PRINT_ERROR("yvar_create_int could not find a free slot to store an entry in.");
    return false;
}

b8 yvar_get_int(const char* name, i32* out_value) {
    if (!state_ptr || !name) {
        return false;
    }

    for (u32 i = 0; i < YVAR_INT_MAX_COUNT; ++i) {
        yvar_int_entry* entry = &state_ptr->ints[i];
        if (entry->name && strings_equali(name, entry->name)) {
            *out_value = entry->value;
            return true;
        }
    }

    PRINT_ERROR("yvar_get_int could not find a yvar named '%s'.", name);
    return false;
}

b8 yvar_set_int(const char* name, i32 value) {
    if (!state_ptr || !name) {
        return false;
    }

    for (u32 i = 0; i < YVAR_INT_MAX_COUNT; ++i) {
        yvar_int_entry* entry = &state_ptr->ints[i];
        if (entry->name && strings_equali(name, entry->name)) {
            entry->value = value;
            return true;
        }
    }

    PRINT_ERROR("yvar_set_int could not find a yvar named '%s'.", name);
    return false;
}

void yvar_console_command_create_int(CONSOLE_COMMAND_CONTEXT context) {
    if (context.argument_count != 2) {
        PRINT_ERROR("yvar_console_command_create_int requires a context arg count of 2.");
        return;
    }

    const char* name = context.arguments[0].value;
    const char* val_str = context.arguments[1].value;
    i32 value = 0;
    if (!string_to_i32(val_str, &value)) {
        PRINT_ERROR("Failed to convert argument 1 to i32: '%s'.", val_str);
        return;
    }

    if (!yvar_create_int(name, value)) {
        PRINT_ERROR("Failed to create int yvar.");
    }
}

void yvar_console_command_print_int(CONSOLE_COMMAND_CONTEXT context) {
    if (context.argument_count != 1) {
        PRINT_ERROR("yvar_console_command_print_int requires a context arg count of 1.");
        return;
    }

    const char* name = context.arguments[0].value;
    i32 value = 0;
    if (!yvar_get_int(name, &value)) {
        PRINT_ERROR("Failed to find yvar called '%s'.", name);
        return;
    }

    char val_str[50] = {0};
    string_format(val_str, "%i", value);

    console_write_line(LOG_LEVEL_INFO, val_str);
}

void yvar_console_command_set_int(CONSOLE_COMMAND_CONTEXT context) {
    if (context.argument_count != 2) {
        PRINT_ERROR("yvar_console_command_set_int requires a context arg count of 2.");
        return;
    }

    const char* name = context.arguments[0].value;
    const char* val_str = context.arguments[1].value;
    i32 value = 0;
    if (!string_to_i32(val_str, &value)) {
        PRINT_ERROR("Failed to convert argument 1 to i32: '%s'.", val_str);
        return;
    }

    if (!yvar_set_int(name, value)) {
        PRINT_ERROR("Failed to set int yvar called '%s' because it doesn't exist.", name);
    }

    char out_str[500] = {0};
    string_format(out_str, "%s = %i", name, value);
    console_write_line(LOG_LEVEL_INFO, val_str);
}

void yvar_console_command_print_all(CONSOLE_COMMAND_CONTEXT context) {
    // Int yvars
    for (u32 i = 0; i < YVAR_INT_MAX_COUNT; ++i) {
        yvar_int_entry* entry = &state_ptr->ints[i];
        if (entry->name) {
            char val_str[500] = {0};
            string_format(val_str, "%s = %i", entry->name, entry->value);
            console_write_line(LOG_LEVEL_INFO, val_str);
        }
    }

    // TODO: Other variable types.
}

void yvar_register_console_commands(void) {
    console_register_command("yvar_create_int", 2, yvar_console_command_create_int);
    console_register_command("yvar_print_int", 1, yvar_console_command_print_int);
    console_register_command("yvar_set_int", 2, yvar_console_command_set_int);
    console_register_command("yvar_print_all", 0, yvar_console_command_print_all);
}
