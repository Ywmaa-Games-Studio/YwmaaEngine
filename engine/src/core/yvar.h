#pragma once

#include "defines.h"

b8 yvar_init(u64* memory_requirement, void* memory);
void yvar_shutdown(void* state);

YAPI b8 yvar_create_int(const char* name, i32 value);
YAPI b8 yvar_get_int(const char* name, i32* out_value);
YAPI b8 yvar_set_int(const char* name, i32 value);
