/* WAYLANDSYM.h
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.15 -02:18
 * Last edited:
 *   2025.04.15 -02:18
 * Auto updated?
 *   Yes
 *
 * Description:
 *   wayland loader symbols
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

/* *INDENT-OFF* */ // clang-format off

#ifndef WAYLAND_MODULE
#define WAYLAND_MODULE(modname)
#endif

#ifndef WAYLAND_SYM
#define WAYLAND_SYM(rc,fn,params)
#endif

#ifndef WAYLAND_SYM_OPT
#define WAYLAND_SYM_OPT(rc,fn,params)
#endif

#ifndef WAYLAND_INTERFACE
#define WAYLAND_INTERFACE(iface)
#endif

WAYLAND_MODULE(WAYLAND_CLIENT)
WAYLAND_SYM(void, wl_proxy_marshal, (struct wl_proxy *, uint32_t, ...))
WAYLAND_SYM(struct wl_proxy *, wl_proxy_create, (struct wl_proxy *, const struct wl_interface *))
WAYLAND_SYM(void, wl_proxy_destroy, (struct wl_proxy *))
WAYLAND_SYM(int, wl_proxy_add_listener, (struct wl_proxy *, void (**)(void), void *))
WAYLAND_SYM(void, wl_proxy_set_user_data, (struct wl_proxy *, void *))
WAYLAND_SYM(void *, wl_proxy_get_user_data, (struct wl_proxy *))
WAYLAND_SYM(uint32_t, wl_proxy_get_version, (struct wl_proxy *))
WAYLAND_SYM(uint32_t, wl_proxy_get_id, (struct wl_proxy *))
WAYLAND_SYM(const char *, wl_proxy_get_class, (struct wl_proxy *))
WAYLAND_SYM(void, wl_proxy_set_queue, (struct wl_proxy *, struct wl_event_queue *))
WAYLAND_SYM(void *, wl_proxy_create_wrapper, (void *))
WAYLAND_SYM(void, wl_proxy_wrapper_destroy, (void *))
WAYLAND_SYM(struct wl_display *, wl_display_connect, (const char *))
WAYLAND_SYM(struct wl_display *, wl_display_connect_to_fd, (int))
WAYLAND_SYM(void, wl_display_disconnect, (struct wl_display *))
WAYLAND_SYM(int, wl_display_get_fd, (struct wl_display *))
WAYLAND_SYM(int, wl_display_dispatch, (struct wl_display *))
WAYLAND_SYM(int, wl_display_dispatch_queue, (struct wl_display *, struct wl_event_queue *))
WAYLAND_SYM(int, wl_display_dispatch_queue_pending, (struct wl_display *, struct wl_event_queue *))
WAYLAND_SYM(int, wl_display_dispatch_pending, (struct wl_display *))
WAYLAND_SYM(int, wl_display_prepare_read, (struct wl_display *))
WAYLAND_SYM(int, wl_display_prepare_read_queue, (struct wl_display *, struct wl_event_queue *))
WAYLAND_SYM(int, wl_display_read_events, (struct wl_display *))
WAYLAND_SYM(void, wl_display_cancel_read, (struct wl_display *))
WAYLAND_SYM(int, wl_display_get_error, (struct wl_display *))
WAYLAND_SYM(int, wl_display_flush, (struct wl_display *))
WAYLAND_SYM(int, wl_display_roundtrip, (struct wl_display *))
WAYLAND_SYM(struct wl_event_queue *, wl_display_create_queue, (struct wl_display *))
WAYLAND_SYM(void, wl_event_queue_destroy, (struct wl_event_queue *))
WAYLAND_SYM(void, wl_log_set_handler_client, (wl_log_func_t))
WAYLAND_SYM(void, wl_list_init, (struct wl_list *))
WAYLAND_SYM(void, wl_list_insert, (struct wl_list *, struct wl_list *) )
WAYLAND_SYM(void, wl_list_remove, (struct wl_list *))
WAYLAND_SYM(int, wl_list_length, (const struct wl_list *))
WAYLAND_SYM(int, wl_list_empty, (const struct wl_list *))
WAYLAND_SYM(void, wl_list_insert_list, (struct wl_list *, struct wl_list *))
WAYLAND_SYM(struct wl_proxy *, wl_proxy_marshal_constructor, (struct wl_proxy *, uint32_t opcode, const struct wl_interface *interface, ...))
WAYLAND_SYM(struct wl_proxy *, wl_proxy_marshal_constructor_versioned, (struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, uint32_t version, ...))
WAYLAND_SYM(void, wl_proxy_set_tag, (struct wl_proxy *, const char * const *))
WAYLAND_SYM(const char * const *, wl_proxy_get_tag, (struct wl_proxy *))

#if WAYLAND_CHECK_VERSION(1, 20, 0)
/* wayland-scanner 1.20 generates code that will call these, so these are
 * non-optional when we are compiling against Wayland 1.20. We don't
 * explicitly call them ourselves, though, so if we are only compiling
 * against Wayland 1.18, they're unnecessary. */
WAYLAND_SYM(struct wl_proxy*, wl_proxy_marshal_flags, (struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interfac, uint32_t version, uint32_t flags, ...))
WAYLAND_SYM(struct wl_proxy*, wl_proxy_marshal_array_flags, (struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, uint32_t version,  uint32_t flags, union wl_argument *args))
#endif

#if 0 // TODO RECONNECT: See waylandvideo.c for more information!
#if WAYLAND_CHECK_VERSION(broken, on, purpose)
WAYLAND_SYM(int, wl_display_reconnect, (struct wl_display*))
#endif
#endif // 0

WAYLAND_INTERFACE(wl_seat_interface)
WAYLAND_INTERFACE(wl_surface_interface)
WAYLAND_INTERFACE(wl_shm_pool_interface)
WAYLAND_INTERFACE(wl_buffer_interface)
WAYLAND_INTERFACE(wl_registry_interface)
WAYLAND_INTERFACE(wl_region_interface)
WAYLAND_INTERFACE(wl_pointer_interface)
WAYLAND_INTERFACE(wl_keyboard_interface)
WAYLAND_INTERFACE(wl_compositor_interface)
WAYLAND_INTERFACE(wl_output_interface)
WAYLAND_INTERFACE(wl_shm_interface)
WAYLAND_INTERFACE(wl_data_device_interface)
WAYLAND_INTERFACE(wl_data_source_interface)
WAYLAND_INTERFACE(wl_data_offer_interface)
WAYLAND_INTERFACE(wl_data_device_manager_interface)

WAYLAND_MODULE(WAYLAND_EGL)
WAYLAND_SYM(struct wl_egl_window *, wl_egl_window_create, (struct wl_surface *, int, int))
WAYLAND_SYM(void, wl_egl_window_destroy, (struct wl_egl_window *))
WAYLAND_SYM(void, wl_egl_window_resize, (struct wl_egl_window *, int, int, int, int))
WAYLAND_SYM(void, wl_egl_window_get_attached_size, (struct wl_egl_window *, int *, int *))

WAYLAND_MODULE(WAYLAND_CURSOR)
WAYLAND_SYM(struct wl_cursor_theme *, wl_cursor_theme_load, (const char *, int , struct wl_shm *))
WAYLAND_SYM(void, wl_cursor_theme_destroy, (struct wl_cursor_theme *))
WAYLAND_SYM(struct wl_cursor *, wl_cursor_theme_get_cursor, (struct wl_cursor_theme *, const char *))
WAYLAND_SYM(struct wl_buffer *, wl_cursor_image_get_buffer, (struct wl_cursor_image *))
WAYLAND_SYM(int, wl_cursor_frame, (struct wl_cursor *, uint32_t))
#ifdef HAVE_WAYLAND_XKB
WAYLAND_MODULE(WAYLAND_XKB)
WAYLAND_SYM(int, xkb_state_key_get_syms, (struct xkb_state *, xkb_keycode_t, const xkb_keysym_t **))
WAYLAND_SYM(int, xkb_keysym_to_utf8, (xkb_keysym_t, char *, size_t) )
WAYLAND_SYM(struct xkb_keymap *, xkb_keymap_new_from_string, (struct xkb_context *, const char *, enum xkb_keymap_format, enum xkb_keymap_compile_flags))
WAYLAND_SYM(struct xkb_state *, xkb_state_new, (struct xkb_keymap *) )
WAYLAND_SYM(int, xkb_keymap_key_repeats, (struct xkb_keymap *keymap, xkb_keycode_t key) )
WAYLAND_SYM(void, xkb_keymap_unref, (struct xkb_keymap *) )
WAYLAND_SYM(void, xkb_state_unref, (struct xkb_state *) )
WAYLAND_SYM(void, xkb_context_unref, (struct xkb_context *) )
WAYLAND_SYM(struct xkb_context *, xkb_context_new, (enum xkb_context_flags flags) )
WAYLAND_SYM(enum xkb_state_component, xkb_state_update_mask, (struct xkb_state *state,\
                      xkb_mod_mask_t depressed_mods,\
                      xkb_mod_mask_t latched_mods,\
                      xkb_mod_mask_t locked_mods,\
                      xkb_layout_index_t depressed_layout,\
                      xkb_layout_index_t latched_layout,\
                      xkb_layout_index_t locked_layout) )
WAYLAND_SYM(struct xkb_compose_table *, xkb_compose_table_new_from_locale, (struct xkb_context *,\
                      const char *locale, enum xkb_compose_compile_flags) )
WAYLAND_SYM(void, xkb_compose_state_reset, (struct xkb_compose_state *) )
WAYLAND_SYM(void, xkb_compose_table_unref, (struct xkb_compose_table *) )
WAYLAND_SYM(struct xkb_compose_state *, xkb_compose_state_new, (struct xkb_compose_table *, enum xkb_compose_state_flags) )
WAYLAND_SYM(void, xkb_compose_state_unref, (struct xkb_compose_state *) )
WAYLAND_SYM(enum xkb_compose_feed_result, xkb_compose_state_feed, (struct xkb_compose_state *, xkb_keysym_t) )
WAYLAND_SYM(enum xkb_compose_status, xkb_compose_state_get_status, (struct xkb_compose_state *) )
WAYLAND_SYM(xkb_keysym_t, xkb_compose_state_get_one_sym, (struct xkb_compose_state *) )
WAYLAND_SYM(void, xkb_keymap_key_for_each, (struct xkb_keymap *, xkb_keymap_key_iter_t, void*) )
WAYLAND_SYM(int, xkb_keymap_key_get_syms_by_level, (struct xkb_keymap *,
                                                        xkb_keycode_t,
                                                        xkb_layout_index_t,
                                                        xkb_layout_index_t,
                                                        const xkb_keysym_t **) )
WAYLAND_SYM(uint32_t, xkb_keysym_to_utf32, (xkb_keysym_t) )
WAYLAND_SYM(uint32_t, xkb_keymap_mod_get_index, (struct xkb_keymap *,
                                                      const char *) )
WAYLAND_SYM(const char *, xkb_keymap_layout_get_name, (struct xkb_keymap*, xkb_layout_index_t))
#endif
#ifdef HAVE_LIBDECOR_H
WAYLAND_MODULE(WAYLAND_LIBDECOR)
WAYLAND_SYM(void, libdecor_unref, (struct libdecor *))
WAYLAND_SYM(struct libdecor *, libdecor_new, (struct wl_display *, struct libdecor_interface *))
WAYLAND_SYM(struct libdecor_frame *, libdecor_decorate, (struct libdecor *,\
                                                             struct wl_surface *,\
                                                             struct libdecor_frame_interface *,\
                                                             void *))
WAYLAND_SYM(void, libdecor_frame_unref, (struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_set_title, (struct libdecor_frame *, const char *))
WAYLAND_SYM(void, libdecor_frame_set_app_id, (struct libdecor_frame *, const char *))
WAYLAND_SYM(void, libdecor_frame_set_max_content_size, (struct libdecor_frame *frame,\
                                                            int content_width,\
                                                            int content_height))
WAYLAND_SYM(void, libdecor_frame_set_min_content_size, (struct libdecor_frame *frame,\
                                                            int content_width,\
                                                            int content_height))
WAYLAND_SYM(void, libdecor_frame_resize, (struct libdecor_frame *,\
                                              struct wl_seat *,\
                                              uint32_t,\
                                              enum libdecor_resize_edge))
WAYLAND_SYM(void, libdecor_frame_move, (struct libdecor_frame *,\
                                            struct wl_seat *,\
                                            uint32_t))
WAYLAND_SYM(void, libdecor_frame_commit, (struct libdecor_frame *,\
                                              struct libdecor_state *,\
                                              struct libdecor_configuration *))
WAYLAND_SYM(void, libdecor_frame_set_minimized, (struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_set_maximized, (struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_unset_maximized, (struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_set_fullscreen, (struct libdecor_frame *, struct wl_output *))
WAYLAND_SYM(void, libdecor_frame_unset_fullscreen, (struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_set_capabilities, (struct libdecor_frame *, \
                                                        enum libdecor_capabilities))
WAYLAND_SYM(void, libdecor_frame_unset_capabilities, (struct libdecor_frame *, \
                                                          enum libdecor_capabilities))
WAYLAND_SYM(bool, libdecor_frame_has_capability, (struct libdecor_frame *, \
                                                      enum libdecor_capabilities))
WAYLAND_SYM(void, libdecor_frame_set_visibility, (struct libdecor_frame *, bool))
WAYLAND_SYM(bool, libdecor_frame_is_visible, (struct libdecor_frame *))
WAYLAND_SYM(bool, libdecor_frame_is_floating, (struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_set_parent, (struct libdecor_frame *,\
                                                  struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_show_window_menu, (struct libdecor_frame *, struct wl_seat *, uint32_t, int, int))
WAYLAND_SYM(struct xdg_surface *, libdecor_frame_get_xdg_surface, (struct libdecor_frame *))
WAYLAND_SYM(struct xdg_toplevel *, libdecor_frame_get_xdg_toplevel, (struct libdecor_frame *))
WAYLAND_SYM(void, libdecor_frame_translate_coordinate, (struct libdecor_frame *, int, int, int *, int *))
WAYLAND_SYM(void, libdecor_frame_map, (struct libdecor_frame *))
WAYLAND_SYM(struct libdecor_state *, libdecor_state_new, (int, int))
WAYLAND_SYM(void, libdecor_state_free, (struct libdecor_state *))
WAYLAND_SYM(bool, libdecor_configuration_get_content_size, (struct libdecor_configuration *,\
                                                                struct libdecor_frame *,\
                                                                int *,\
                                                                int *))
WAYLAND_SYM(bool, libdecor_configuration_get_window_state, (struct libdecor_configuration *,\
                                                                enum libdecor_window_state *))
WAYLAND_SYM(int, libdecor_dispatch, (struct libdecor *, int))

#if defined(VIDEO_DRIVER_WAYLAND_DYNAMIC_LIBDECOR) || LIBDECOR_CHECK_VERSION(0, 2, 0)
// Only found in libdecor 0.1.1 or higher, so failure to load them is not fatal.
WAYLAND_SYM_OPT(void, libdecor_frame_get_min_content_size, (const struct libdecor_frame *,\
                                                            int *,\
                                                            int *))
WAYLAND_SYM_OPT(void, libdecor_frame_get_max_content_size, (const struct libdecor_frame *,\
                                                            int *,\
                                                            int *))
#endif

#if defined(VIDEO_DRIVER_WAYLAND_DYNAMIC_LIBDECOR) || LIBDECOR_CHECK_VERSION(0, 3, 0)
WAYLAND_SYM_OPT(enum libdecor_wm_capabilities, libdecor_frame_get_wm_capabilities, (struct libdecor_frame *))
#endif

#endif

#undef WAYLAND_MODULE
#undef WAYLAND_SYM
#undef WAYLAND_SYM_OPT
#undef WAYLAND_INTERFACE

/* *INDENT-ON* */ // clang-format on