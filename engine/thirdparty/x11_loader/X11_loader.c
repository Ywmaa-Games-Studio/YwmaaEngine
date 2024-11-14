/* clang-format off */
#include "X11_loader.h"
#include <dlfcn.h>


static void* x11_loaded_module = NULL;
static void* libxkbcommon_loaded_module = NULL;
static void* libxcb_loaded_module = NULL;
static void* libx11xcb_loaded_module = NULL;
int x11_initialize(void)
{
	void* x11_module = dlopen("libX11.so.6", RTLD_LAZY | RTLD_LOCAL);
	if (!x11_module)
		x11_module = dlopen("libX11.so", RTLD_LAZY | RTLD_LOCAL);
	if (!x11_module)
		x11_module = dlopen("libX11-6.so", RTLD_LAZY | RTLD_LOCAL);
	if (!x11_module)
		return 1;

	loader_XOpenDisplay = (PFN_XOpenDisplay)dlsym(x11_module, "XOpenDisplay");
	
	void* libxkbcommon_module = dlopen("libxkbcommon.so.0", RTLD_LAZY | RTLD_LOCAL);
	if (!libxkbcommon_module)
		libxkbcommon_module = dlopen("libxkbcommon.so", RTLD_LAZY | RTLD_LOCAL);
	if (!libxkbcommon_module)
		libxkbcommon_module = dlopen("libxkbcommon-0.so", RTLD_LAZY | RTLD_LOCAL);
	if (!libxkbcommon_module)
		return 2;
	loader_XkbKeycodeToKeysym = (PFN_XkbKeycodeToKeysym)dlsym(libxkbcommon_module, "XkbKeycodeToKeysym");
	loader_XkbLookupKeySym = (PFN_XkbLookupKeySym)dlsym(libxkbcommon_module, "XkbLookupKeySym");

	void* libxcb_module = dlopen("libxcb.so.1", RTLD_LAZY | RTLD_LOCAL);
	if (!libxcb_module)
		libxcb_module = dlopen("libxcb.so", RTLD_LAZY | RTLD_LOCAL);
	if (!libxcb_module)
		libxcb_module = dlopen("libxcb-1.so", RTLD_LAZY | RTLD_LOCAL);
	if (!libxcb_module)
		return 3;
	loader_xcb_get_setup = (PFN_xcb_get_setup)dlsym(libxcb_module, "xcb_get_setup");
	loader_xcb_setup_roots_iterator = (PFN_xcb_setup_roots_iterator)dlsym(libxcb_module, "xcb_setup_roots_iterator");
	loader_xcb_screen_next = (PFN_xcb_screen_next)dlsym(libxcb_module, "xcb_screen_next");
	loader_xcb_generate_id = (PFN_xcb_generate_id)dlsym(libxcb_module, "xcb_generate_id");
	loader_xcb_create_window = (PFN_xcb_create_window)dlsym(libxcb_module, "xcb_create_window");
	loader_xcb_change_property = (PFN_xcb_change_property)dlsym(libxcb_module, "xcb_change_property");
	loader_xcb_intern_atom = (PFN_xcb_intern_atom)dlsym(libxcb_module, "xcb_intern_atom");
	loader_xcb_intern_atom_reply = (PFN_xcb_intern_atom_reply)dlsym(libxcb_module, "xcb_intern_atom_reply");
	loader_xcb_map_window = (PFN_xcb_map_window)dlsym(libxcb_module, "xcb_map_window");	
	loader_xcb_flush = (PFN_xcb_flush)dlsym(libxcb_module, "xcb_flush");
	loader_xcb_destroy_window = (PFN_xcb_destroy_window)dlsym(libxcb_module, "xcb_destroy_window");	
	loader_xcb_poll_for_event = (PFN_xcb_poll_for_event)dlsym(libxcb_module, "xcb_poll_for_event");	

	void* libx11xcb_module = dlopen("libX11-xcb-1.so", RTLD_LAZY | RTLD_LOCAL);
	if (!libx11xcb_module)
		libx11xcb_module = dlopen("libX11-xcb.so", RTLD_LAZY | RTLD_LOCAL);
	if (!libx11xcb_module)
		libx11xcb_module = dlopen("libX11-xcb.so.1", RTLD_LAZY | RTLD_LOCAL);
	if (!libx11xcb_module)
		return 4;
	loader_XGetXCBConnection = (PFN_XGetXCBConnection)dlsym(libx11xcb_module, "XGetXCBConnection");

	x11_loaded_module = x11_module;
	libxcb_loaded_module = libxcb_module;
	libxkbcommon_loaded_module = libxkbcommon_module;
	libx11xcb_loaded_module = libx11xcb_module;

	return 0;
}

void x11_finalize(void)
{
	if (x11_loaded_module)
		dlclose(x11_loaded_module);
	if (libxcb_loaded_module)
		dlclose(libxcb_loaded_module);
	if (libxkbcommon_loaded_module)
		dlclose(libxkbcommon_loaded_module);
	if (libx11xcb_loaded_module)
		dlclose(libx11xcb_loaded_module);

	x11_loaded_module = NULL;
	libxcb_loaded_module = NULL;
	libxkbcommon_loaded_module = NULL;
	libx11xcb_loaded_module = NULL;
}


/* X11_GENERATE_PROTOTYPES_C */
#ifdef __GNUC__
#ifdef X11_DEFAULT_VISIBILITY
#	pragma GCC visibility push(default)
#else
#	pragma GCC visibility push(hidden)
#endif
#endif

PFN_XOpenDisplay loader_XOpenDisplay;

PFN_XkbKeycodeToKeysym loader_XkbKeycodeToKeysym;
PFN_XkbLookupKeySym loader_XkbLookupKeySym;

PFN_XGetXCBConnection loader_XGetXCBConnection;

PFN_xcb_connection_has_error loader_xcb_connection_has_error;
PFN_xcb_get_setup loader_xcb_get_setup;
PFN_xcb_setup_roots_iterator loader_xcb_setup_roots_iterator;
PFN_xcb_screen_next loader_xcb_screen_next;
PFN_xcb_generate_id loader_xcb_generate_id;
PFN_xcb_create_window loader_xcb_create_window;
PFN_xcb_change_property loader_xcb_change_property;
PFN_xcb_intern_atom loader_xcb_intern_atom;
PFN_xcb_intern_atom_reply loader_xcb_intern_atom_reply;
PFN_xcb_map_window loader_xcb_map_window;
PFN_xcb_flush loader_xcb_flush;
PFN_xcb_destroy_window loader_xcb_destroy_window;
PFN_xcb_poll_for_event loader_xcb_poll_for_event;

#ifdef __GNUC__
#	pragma GCC visibility pop
#endif

/* clang-format on */
