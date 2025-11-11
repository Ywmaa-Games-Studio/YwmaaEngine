/**
 * @file yvar.h
 * @brief A file that contains the YVar system. YVars are global variables
 * that are dynamically created and set/used within the engine and/or
 * application, and are accessible from anywhere.
 * 
 */
#pragma once

#include "defines.h"

/**
 * @brief Initializes the YVar system. YVars are global variables
 * that are dynamically created and set/used within the engine and/or
 * application, and are accessible from anywhere. Like any other system,
 * this should be called twice, once to obtain the memory requirement
 * (where memory = 0), and a second time with an allocated block of
 * memory.
 * 
 * @param memory_requirement A pointer to hold the memory requirement for this system.
 * @param memory An allocated block of memory the size of memory_requirement.
 * @param config A pointer to config, if required.
 * @return b8 True on success; otherwise false.
 */
b8 yvar_init(u64* memory_requirement, void* memory, void* config);
/**
 * @brief Shuts down the YVar system.
 * 
 * @param state The system state.
 */
void yvar_shutdown(void* state);

/**
 * @brief Creates an integer variable type.
 * 
 * @param name The name of the variable.
 * @param value The value of the variable.
 * @return True on success; otherwise false.
 */
YAPI b8 yvar_create_int(const char* name, i32 value);

/**
 * @brief Attempts to obtain a variable value with the given
 * name. Returns false if not found.
 * 
 * @param name The name of the variable.
 * @param out_value A pointer to hold the variable.
 * @return True if the variable was found; otherwise false.
 */
YAPI b8 yvar_get_int(const char* name, i32* out_value);

/**
 * @brief Attempts to set the value of an existing variable with
 * the given name. Returns false if the variable was not found.
 * 
 * @param name The name of the variable.
 * @param value The value to be set.
 * @return True if found and set, otherwise false.
 */
YAPI b8 yvar_set_int(const char* name, i32 value);
