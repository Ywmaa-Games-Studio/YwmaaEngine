#include "platform.h"

#if defined(YPLATFORM_LINUX) || defined(YPLATFORM_APPLE)

#include "core/ymemory.h"
#include "core/ystring.h"
#include "core/logger.h"
#include "data_structures/darray.h"

#include <dlfcn.h>

b8 platform_dynamic_library_load(const char *name, DYNAMIC_LIBRARY *out_library) {
    if (!out_library) {
        return false;
    }
    yzero_memory(out_library, sizeof(DYNAMIC_LIBRARY));
    if (!name) {
        return false;
    }

    char filename[260];  // NOTE: same as Windows, for now.
    yzero_memory(filename, sizeof(char) * 260);

    const char *extension = platform_dynamic_library_extension();
    const char *prefix = platform_dynamic_library_prefix();

    string_format(filename, "%s%s%s", prefix, name, extension);

    void* library = dlopen(filename, RTLD_NOW);  // "libtestbed_lib_loaded.dylib"

    // try ../lib/ path
    if (!library) {
        yzero_memory(filename, sizeof(char) * 260);
        string_format(filename, "../lib/%s%s%s", prefix, name, extension);
        library = dlopen(filename, RTLD_NOW);  // "libtestbed_lib_loaded.dylib"
    }
    if (!library) {
        PRINT_ERROR("Error opening library: %s", dlerror());
        return false;
    }

    out_library->name = string_duplicate(name);
    out_library->filename = string_duplicate(filename);

    out_library->internal_data_size = 8;
    out_library->internal_data = library;

    out_library->functions = darray_create(DYNAMIC_LIBRARY_FUNCTION);

    return true;
}

b8 platform_dynamic_library_unload(DYNAMIC_LIBRARY *library) {
    if (!library) {
        return false;
    }

    if (!library->internal_data) {
        return false;
    }

    i32 result = dlclose(library->internal_data);
    if (result != 0) {  // Opposite of Windows, 0 means success.
        return false;
    }

    library->internal_data = 0;

    if (library->name) {
        yfree((void *)library->name);
    }

    if (library->filename) {
        yfree((void *)library->filename);
    }

    if (library->functions) {
        u32 count = darray_length(library->functions);
        for (u32 i = 0; i < count; ++i) {
            DYNAMIC_LIBRARY_FUNCTION *f = &library->functions[i];
            if (f->name) {
                yfree((void *)f->name);
            }
        }

        darray_destroy(library->functions);
        library->functions = 0;
    }

    yzero_memory(library, sizeof(DYNAMIC_LIBRARY));

    return true;
}

b8 platform_dynamic_library_load_function(const char *name, DYNAMIC_LIBRARY *library) {
    if (!name || !library) {
        return false;
    }

    if (!library->internal_data) {
        return false;
    }

    void *f_addr = dlsym(library->internal_data, name);
    if (!f_addr) {
        return false;
    }

    DYNAMIC_LIBRARY_FUNCTION f = {0};
    f.pfn = f_addr;
    f.name = string_duplicate(name);
    darray_push(library->functions, f);

    return true;
}

#endif
