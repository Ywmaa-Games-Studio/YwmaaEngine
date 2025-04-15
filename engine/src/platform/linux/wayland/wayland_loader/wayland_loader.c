/* WAYLAND LOADER.c
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.15 -02:06
 * Last edited:
 *   2025.04.15 -02:07
 * Auto updated?
 *   Yes
 *
 * Description:
 *   wayland loader
 * This file is part of the SDL library.
 * It is subject to the license terms in the LICENSE notice below
**/

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "defines.h"
// Linux platform layer - Wayland implementation
#if defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED) && !defined(YPLATFORM_ANDROID)

#include "core/asserts.h"
#include "core/logger.h"
#define WAYLAND_DYNAMIC_LOAD_IMPLEMENTATION
#ifdef WAYLAND_DYNAMIC_LOAD_IMPLEMENTATION
#define VIDEO_DRIVER_WAYLAND 1
#undef WAYLAND_DYNAMIC_LOAD_IMPLEMENTATION
#endif

#ifdef VIDEO_DRIVER_WAYLAND

#define DEBUG_DYNAMIC_WAYLAND 0

#include "wayland_loader.h"

#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC

#include <dlfcn.h>
#include <stdlib.h>
typedef struct
{
    void* lib;
    const char *libname;
} waylanddynlib;

void* LoadObject(const char *sofile)
{
    void *handle;
    const char *loaderror;

    handle = dlopen(sofile, RTLD_NOW | RTLD_LOCAL);
    loaderror = dlerror();
    if (!handle) {
        PRINT_ERROR("Failed loading %s: %s", sofile, loaderror);
        YASSERT_MSG(false,"Failed loading wayland");
        //SetError("Failed loading %s: %s", sofile, loaderror);
    }
    return (void *) handle;
}

void* LoadFunction(void *handle, const char *name){
    void *symbol = dlsym(handle, name);
    if (!symbol) {
        // prepend an underscore for platforms that need that.
/*         bool isstack;
        size_t len = strlen(name) + 1;
        char *_name = small_alloc(char, len + 1, &isstack);
        _name[0] = '_';
        memcpy(&_name[1], name, len);
        symbol = dlsym(handle, _name);
        small_free(_name, isstack); */
        if (!symbol) {
            return NULL;
        }
    }
    return symbol;
}
static waylanddynlib waylandlibs[] = {
    { NULL, VIDEO_DRIVER_WAYLAND_DYNAMIC },
#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC_EGL
    { NULL, VIDEO_DRIVER_WAYLAND_DYNAMIC_EGL },
#endif
#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC_CURSOR
    { NULL, VIDEO_DRIVER_WAYLAND_DYNAMIC_CURSOR },
#endif
#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC_XKBCOMMON
    { NULL, VIDEO_DRIVER_WAYLAND_DYNAMIC_XKBCOMMON },
#endif
#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC_LIBDECOR
    { NULL, VIDEO_DRIVER_WAYLAND_DYNAMIC_LIBDECOR },
#endif
    { NULL, NULL }
};

static void *WAYLAND_GetSym(const char *fnname, int *pHasModule, int required)
{
    void *fn = NULL;
    waylanddynlib *dynlib;
    for (dynlib = waylandlibs; dynlib->libname; dynlib++) {
        if (dynlib->lib) {
            fn = LoadFunction(dynlib->lib, fnname);
            if (fn) {
                break;
            }
        }
    }

#if DEBUG_DYNAMIC_WAYLAND
    if (fn) {
        PRINT_TRACE("WAYLAND: Found '%s' in %s (%p)", fnname, dynlib->libname, fn);
        //Log("WAYLAND: Found '%s' in %s (%p)", fnname, dynlib->libname, fn);
    } else {
        PRINT_TRACE("WAYLAND: Symbol '%s' NOT FOUND!", fnname);
        //Log("WAYLAND: Symbol '%s' NOT FOUND!", fnname);
    }
#endif

    if (!fn && required == 1) {
        *pHasModule = 0; // kill this module.
    }

    return fn;
}

#else

#include <wayland-egl.h>

#endif // VIDEO_DRIVER_WAYLAND_DYNAMIC

// Define all the function pointers and wrappers...
#define WAYLAND_MODULE(modname)         int WAYLAND_HAVE_##modname = 0;
#define WAYLAND_SYM(rc, fn, params)     DYNWAYLANDFN_##fn WAYLAND_##fn = NULL;
#define WAYLAND_SYM_OPT(rc, fn, params) DYNWAYLANDFN_##fn WAYLAND_##fn = NULL;
#define WAYLAND_INTERFACE(iface)        const struct wl_interface *WAYLAND_##iface = NULL;
#include "waylandsym.h"

static int wayland_load_refcount = 0;

void WAYLAND_UnloadSymbols(void)
{
    // Don't actually unload if more than one module is using the libs...
    if (wayland_load_refcount > 0) {
        if (--wayland_load_refcount == 0) {
#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC
            int i;
#endif

            // set all the function pointers to NULL.
#define WAYLAND_MODULE(modname)         WAYLAND_HAVE_##modname = 0;
#define WAYLAND_SYM(rc, fn, params)     WAYLAND_##fn = NULL;
#define WAYLAND_SYM_OPT(rc, fn, params) WAYLAND_##fn = NULL;
#define WAYLAND_INTERFACE(iface)        WAYLAND_##iface = NULL;
#include "waylandsym.h"

#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC
            for (i = 0; i < sizeof(waylandlibs)/sizeof(waylanddynlib); i++) {
                if (waylandlibs[i].lib) {
                    dlclose(waylandlibs[i].lib);
                    waylandlibs[i].lib = NULL;
                }
            }
#endif
        }
    }
}

// returns non-zero if all needed symbols were loaded.
int WAYLAND_LoadSymbols(void)
{
    int result = 1; // always succeed if not using Dynamic WAYLAND stuff.

    // deal with multiple modules (dga, wayland, etc) needing these symbols...
    if (wayland_load_refcount++ == 0) {
#ifdef VIDEO_DRIVER_WAYLAND_DYNAMIC
        int i;
        int *thismod = NULL;
        for (i = 0; i < sizeof(waylandlibs)/sizeof(waylanddynlib); i++) {
            if (waylandlibs[i].libname) {
                waylandlibs[i].lib = LoadObject(waylandlibs[i].libname);
            }
        }

#define WAYLAND_MODULE(modname) WAYLAND_HAVE_##modname = 1; // default yes
#include "waylandsym.h"

#define WAYLAND_MODULE(modname)         thismod = &WAYLAND_HAVE_##modname;
#define WAYLAND_SYM(rc, fn, params)     WAYLAND_##fn = (DYNWAYLANDFN_##fn)WAYLAND_GetSym(#fn, thismod, 1);
#define WAYLAND_SYM_OPT(rc, fn, params) WAYLAND_##fn = (DYNWAYLANDFN_##fn)WAYLAND_GetSym(#fn, thismod, 0);
#define WAYLAND_INTERFACE(iface)        WAYLAND_##iface = (struct wl_interface *)WAYLAND_GetSym(#iface, thismod, 1);
#include "waylandsym.h"

        if (WAYLAND_HAVE_WAYLAND_CLIENT &&
            WAYLAND_HAVE_WAYLAND_CURSOR) {
            // All required symbols loaded, only libdecor is optional.
        } else {
            // in case something got loaded...
            WAYLAND_UnloadSymbols();
            result = 0;
        }

#else // no dynamic WAYLAND

#define WAYLAND_MODULE(modname)         WAYLAND_HAVE_##modname = 1; // default yes
#define WAYLAND_SYM(rc, fn, params)     WAYLAND_##fn = fn;
#define WAYLAND_SYM_OPT(rc, fn, params) WAYLAND_##fn = fn;
#define WAYLAND_INTERFACE(iface)        WAYLAND_##iface = &iface;
#include "waylandsym.h"

#endif
    }

    return result;
}

#endif // VIDEO_DRIVER_WAYLAND

#endif // defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED) && !defined(YPLATFORM_ANDROID)
