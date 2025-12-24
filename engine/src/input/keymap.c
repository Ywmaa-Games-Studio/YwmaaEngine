#include "keymap.h"
#include "core/ymemory.h"

KEYMAP keymap_create(void) {
    KEYMAP map;
    yzero_memory(&map, sizeof(KEYMAP));

    map.overrides_all = false;

    for (u32 i = 0; i < KEYS_MAX_KEYS; ++i) {
        map.entries[i].bindings = 0;
        map.entries[i].key = i;
    }

    return map;
}

void keymap_binding_add(KEYMAP* map, E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data, PFN_keybind_callback callback) {
    if (map) {
        KEYMAP_ENTRY* entry = &map->entries[key];
        KEYMAP_BINDING* node = entry->bindings;
        KEYMAP_BINDING* previous = entry->bindings;
        while (node) {
            previous = node;
            node = node->next;
        }

        KEYMAP_BINDING* new_entry = yallocate_aligned(sizeof(KEYMAP_BINDING), 8, MEMORY_TAG_KEYMAP);
        new_entry->callback = callback;
        new_entry->modifiers = modifiers;
        new_entry->type = type;
        new_entry->next = 0;
        new_entry->user_data = user_data;

        if (previous) {
            previous->next = new_entry;
        } else {
            entry->bindings = new_entry;
        }
    }
}

void keymap_binding_remove(KEYMAP* map, E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, PFN_keybind_callback callback) {
    if (map) {
        KEYMAP_ENTRY* entry = &map->entries[key];
        KEYMAP_BINDING* node = entry->bindings;
        KEYMAP_BINDING* previous = entry->bindings;
        while (node) {
            if (node->callback == callback && node->modifiers == modifiers && node->type == type) {
                // Remove it
                previous->next = node->next;
                yfree(node);
                return;
            }
            previous = node;
            node = node->next;
        }
    }
}

void keymap_clear(KEYMAP* map) {
    if (map) {
        for (u32 i = 0; i < KEYS_MAX_KEYS; ++i) {
            KEYMAP_ENTRY* entry = &map->entries[i];
            KEYMAP_BINDING* node = entry->bindings;
            KEYMAP_BINDING* previous = entry->bindings;
            while (node) {
                // Remove all nodes
                previous->next = node->next;
                yfree(node);
                previous = node;
                node = node->next;
            }
        }
    }
}
