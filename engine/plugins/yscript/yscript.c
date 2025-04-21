/* YSCRIPT.c
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.20 -19:36
 * Last edited:
 *   <l3152AMle>
 * Auto updated?
 *   Yes
 *
 * Description:
 *   YScript
**/

#include "yscript.h"

#include "core/logger.h"
#include "core/ystring.h"
#include "core/ymemory.h"

#include "systems/resource_system.h"

yscript_context* test_script;
b8 load_script_file(const char* path, char** file_read);
void unload_script_file(void);

void enter_script_context(void) {
    PRINT_DEBUG("Entering script context.");
    // Load file from disk
    // TODO: Should be able to be located anywhere.
    char* format_str = "scripts/%s.%s";
    char full_file_path[512];
    char* file_buffer;
    // TODO: try different extensions
    string_format(full_file_path, format_str, "test", "yscript");
    if (!load_script_file(full_file_path, &file_buffer)) {
        PRINT_ERROR("Failed to load script file: '%s'. Null pointer will be returned.", full_file_path);
    }
    PRINT_DEBUG("Script file loaded: '%s'.", full_file_path);
    test_script = yscript_context_create();
    b8 success = yscript_execute_string(test_script, file_buffer);
    //b8 success2 = yscript_execute_string(test_script, file_buffer);
    PRINT_DEBUG("Script executed: '%s'.", success ? "success" : "failure");
    yscript_context_destroy(test_script);
    unload_script_file();
    yscript_clear_types();
}

// Read the resource.
RESOURCE text_resource;
b8 load_script_file(const char* path, char** file_read) {
    

    if (!resource_system_load(path, RESOURCE_TYPE_TEXT, NULL, &text_resource)) {
        PRINT_ERROR("Unable to read script file: %s.", path);
        return false;
    }
    
    *file_read = (char*)text_resource.data;

    return true;
}

void unload_script_file(void) {
    // Release the resource.
    resource_system_unload(&text_resource);
}

