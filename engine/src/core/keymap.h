#pragma once

#include "defines.h"
#include "input/input.h"

typedef enum E_KEYMAP_MODIFIER_BITS {
    KEYMAP_MODIFIER_NONE_BIT = 0x0,
    KEYMAP_MODIFIER_SHIFT_BIT = 0x1,
    KEYMAP_MODIFIER_CONTROL_BIT = 0x2,
    KEYMAP_MODIFIER_ALT_BIT = 0x4,
} E_KEYMAP_MODIFIER_BITS;

typedef u32 KEYMAP_MODIFIER;

typedef enum E_KEYMAP_ENTRY_BIND_TYPE {
    /** @brief An undefined mapping that can be overridden. */
    KEYMAP_BIND_TYPE_UNDEFINED = 0x0,
    /** @brief Callback is made when key is initially pressed. */
    KEYMAP_BIND_TYPE_PRESS = 0x1,
    /** @brief Callback is made when key is released. */
    KEYMAP_BIND_TYPE_RELEASE = 0x2,
    /** @brief Callback is made continuously while key is held. */
    KEYMAP_BIND_TYPE_HOLD = 0x4,
    /** @brief Used to disable a key binding on a lower-level map. */
    KEYMAP_BIND_TYPE_UNSET = 0x8
} E_KEYMAP_ENTRY_BIND_TYPE;

typedef void (*PFN_keybind_callback)(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data);

typedef struct KEYMAP_BINDING {
    E_KEYMAP_ENTRY_BIND_TYPE type;
    KEYMAP_MODIFIER modifiers;
    PFN_keybind_callback callback;
    void* user_data;
    struct KEYMAP_BINDING* next;
} KEYMAP_BINDING;

typedef struct KEYMAP_ENTRY {
    E_KEYS key;
    // Linked list of bindings.
    KEYMAP_BINDING* bindings;
} KEYMAP_ENTRY;

typedef struct KEYMAP {
    b8 overrides_all;
    KEYMAP_ENTRY entries[KEYS_MAX_KEYS];
} KEYMAP;

YAPI KEYMAP keymap_create(void);

YAPI void keymap_binding_add(KEYMAP* map, E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data, PFN_keybind_callback callback);
YAPI void keymap_binding_remove(KEYMAP* map, E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, PFN_keybind_callback callback);

// TODO: Keymaps will replace the existing
// checks for key states in that they will instead
// call callback functions instead.
// Maps will be added onto a stack, where bindings are
// replaced along the way. For example, if the base KEYMAP
// defines the "escape" key as an application quit, then
// another KEYMAP added on re-defines the key to nothing while
// adding a new binding for "a", then "a"'s binding will work,
// and "escape" will do nothing. If "escape" were left undefined
// in the second KEYMAP, the original mapping is left unchanged.
// Maps are pushed/popped as expected on a stack.
