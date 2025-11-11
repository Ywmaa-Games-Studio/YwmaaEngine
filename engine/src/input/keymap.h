/**
 * @file keymap.h
 * @brief This file contains the structures and functions required to
 * implement keymaps and keybindings, used to translate keyboard input
 * events to events and/or call bound functions automatically. Keymaps 
 * replace the need for checks of key states in that they will instead
 * invoke callback functions instead.
 * Maps will be added onto a stack, where bindings are
 * replaced along the way. For example, if the base keymap
 * defines the "escape" key as an application quit, then
 * another keymap added on re-defines the key to nothing while
 * adding a new binding for "a", then "a"'s binding will work,
 * and "escape" will do nothing. If "escape" were left undefined
 * in the second keymap, the original mapping is left unchanged.
 * Maps are pushed/popped as expected on a stack.
 * 
 */
#pragma once

#include "defines.h"
#include "input/input.h"

/**
 * @brief An enumeration of modifier keys required by
 * a keymap's keybinding to be activated. These may be
 * combined (ORed) together to require multiple modifiers.
 */
typedef enum E_KEYMAP_MODIFIER_BITS {
    /** @brief The default modifier, means that no modifiers are required. */
    KEYMAP_MODIFIER_NONE_BIT = 0x0,
    /** @brief A shift key must be pressed for the binding to fire. */
    KEYMAP_MODIFIER_SHIFT_BIT = 0x1,
    /** @brief A control(ctrl)/cmd key must be pressed for the binding to fire. */
    KEYMAP_MODIFIER_CONTROL_BIT = 0x2,
    /** @brief A alt/option key must be pressed for the binding to fire. */
    KEYMAP_MODIFIER_ALT_BIT = 0x4,
} E_KEYMAP_MODIFIER_BITS;

/** @brief A typedef for combined keymap modifiers. */
typedef u32 KEYMAP_MODIFIER;

/**
 * @brief A collection of keymap binding types, which
 * correspond to various types of key input events such
 * as press, release or hold.
 */
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

/** @brief A typedef of a keybinding callback to be made when a keybinding is activated. */
typedef void (*PFN_keybind_callback)(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data);

/**
 * @brief Represents an individual binding, containing the keybind type,
 * modifiers, callback, potential user data. Doubles as a linked-list
 * node, as keys can have multiple bindings associated with them on the
 * same map.
 */
typedef struct KEYMAP_BINDING {
    /** @brief The keybind type (i.e. press, release, hold). Default: KEYMAP_BIND_TYPE_UNDEFINED */
    E_KEYMAP_ENTRY_BIND_TYPE type;
    /** @brief Required modifiers, if any. Default: KEYMAP_MODIFIER_NONE_BIT */
    KEYMAP_MODIFIER modifiers;
    /** @brief A function pointer to be invoked when this binding is triggered. */
    PFN_keybind_callback callback;
    /** @brief User data, if supplied. Otherwise 0. */
    void* user_data;
    /** @brief A pointer to the next binding in the linked list, or 0 if the tail. */
    struct KEYMAP_BINDING* next;
} KEYMAP_BINDING;

/**
 * @brief An individual entry for a keymap, which contains the key
 * to be bound and a linked list of bindings.
 */
typedef struct KEYMAP_ENTRY {
    /** @brief The bound key. */
    E_KEYS key;
    /** @brief Linked list of bindings. Default: 0. */
    KEYMAP_BINDING* bindings;
} KEYMAP_ENTRY;

/**
 * @brief A keymap, which holds a list of keymap entries, each with
 * a list of bindings. These are held in an internal stack, and 
 * override entries of the keymaps below it in the stack.
 */
typedef struct KEYMAP {
    /** 
     * @brief Indicates that all entries are overridden, even ones
     * not defined (effectively blanking them out until this map is popped).
     * */
    b8 overrides_all;
    /** @brief An array of keymap entries, indexed by keycode for quick lookups. */
    KEYMAP_ENTRY entries[KEYS_MAX_KEYS];
} KEYMAP;

/**
 * @brief Creates and returns a new keymap.
 */
YAPI KEYMAP keymap_create(void);

/**
 * @brief Adds a binding to the keymap provided.
 * 
 * @param map A pointer to the keymap to add a binding to. Required.
 * @param key The key to bind to.
 * @param type The type of binding.
 * @param modifiers Required modifier keys, if any (OR them together).
 * @param user_data User data, if any. Optional.
 * @param callback The callback function pointer. Required.
 */
YAPI void keymap_binding_add(KEYMAP* map, E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data, PFN_keybind_callback callback);

/**
 * @brief Removes the binding from the given keymap that also matches
 * on key, bind type, modifiers and callback. If no match is found, nothing
 * is done.
 * 
 * @param map A pointer to the keymap to remove the binding from. Required.
 * @param key The key to bind to.
 * @param type The type of binding.
 * @param modifiers The modifiers required for the binding.
 * @param callback The callback to be made from the biding. Required.
 */
YAPI void keymap_binding_remove(KEYMAP* map, E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, PFN_keybind_callback callback);
