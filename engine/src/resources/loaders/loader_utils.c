#include "loader_utils.h"

#include "core/ymemory.h"
#include "core/logger.h"
#include "core/ystring.h"

b8 resource_unload(struct RESOURCE_LOADER* self, RESOURCE* resource, E_MEMORY_TAG tag) {
    if (!self || !resource) {
        PRINT_WARNING("resource_unload called with nullptr for self or resource.");
        return false;
    }

    u32 path_length = string_length(resource->full_path);
    if (path_length) {
        yfree(resource->full_path);
    }

    if (resource->data) {
        yfree(resource->data);
        resource->data = 0;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }

    return true;
}
