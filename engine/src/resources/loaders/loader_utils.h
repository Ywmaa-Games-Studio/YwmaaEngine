#pragma once

#include "defines.h"
#include "core/ymemory.h"
#include "resources/resource_types.h"

struct RESOURCE_LOADER;

b8 resource_unload(struct RESOURCE_LOADER* self, RESOURCE* resource, E_MEMORY_TAG tag);
